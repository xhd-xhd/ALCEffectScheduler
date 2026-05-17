# ALC Effect Scheduler — 设计文档

## 概述

ALC Effect Scheduler 是一个车载氛围灯调度引擎。它接收 CAN FD 帧（车身信号），
经过五层流水线处理，最终驱动车身内数十到上百颗 LED 的 RGB 颜色和亮度。

目标平台：32 位 MCU，10ms 调度周期，不允许动态内存分配。
当前在 PC 上通过 HAL 抽象层完整模拟运行。

---

## 五层数据流

```
CAN FD 帧 (64 bytes)
  │  hal_can_read()
  ▼
can_parse_frame() ──→ Event 队列        ← 语义化：「发生了什么事」
  │  event_to_command()  边沿检测
  ▼
EffectCommand 队列                       ← 调度指令：「谁该激活/停用」
  │  em_process_command()
  ▼
EffectManager 调度                       ← 优先级仲裁 + zone 归属
  │  em_update_all()
  ▼
LedOutput ──→ hal_led_send()             ← 最终的 RGB+亮度值
```

每层只关心自己的事：

### Layer 1: CAN 解析 (`src/can/parser.c`)

将 64 字节原始 CAN FD 帧解析为语义化的 Event 结构体。知道 CAN ID 和车身信号的
对应关系，但不知道这些信号会触发什么灯效。MCU 移植时只需改这一层的信号映射。

### Layer 2: 事件→命令 (`src/commands/processor.c`)

Event → EffectCommand，核心逻辑是**边沿检测**。采用计数方式：

- 门计数：任一扇门开 → 激活 welcome，所有门关 → 停用 welcome
- 雷达计数：任一侧有障碍物 → 激活 rear_alert，两侧都清空 → 停用 rear_alert

处理器不知道灯效怎么渲染，只知道"什么时候该让谁跑起来"。

### Layer 3: 命令队列

EffectCommand 环形缓冲，解耦 CAN 接收线程和效果调度线程。

### Layer 4: EffectManager (`src/manager/effect_manager.c`)

效果调度核心。所有效果注册时已按优先级排好序，运行时只遍历，不改结构。
按 zone 粒度做优先级仲裁，管理 IDLE / ACTIVE / SHADOWED 状态机。

### Layer 5: LED 输出

每帧产出一个 LedOutput（所有 zone 的 RGB+亮度值），交给 HAL 发送到 LED 驱动芯片。

---

## 三个核心设计

### 1. ZoneMask — 按 zone 粒度管理灯区

```
128 个 zone → 4 个 uint32_t，每位对应一个物理灯区
MAX_ZONES 可在编译时调整（-DMAX_ZONES=256）
```

ZoneMask 是一个多字位图。每个灯效在工厂注册时声明自己管辖哪些 zone。
所有位运算使用原生 uint32_t 指令，32 位 MCU 上一条指令完成。

配合 `led_map.h` 中的 zone 编号枚举和分组函数：

```c
// zone 枚举 — 按仲裁需求拆分，不是按像素数量拆分
enum {
    Z_FRONT_LEFT_DOOR,   // 0  单像素点光源
    Z_FRONT_RIGHT_DOOR,  // 1
    Z_REAR_LEFT_DOOR,    // 2
    Z_REAR_RIGHT_DOOR,   // 3
    Z_FOOTWELL_LEFT,     // 4
    Z_FOOTWELL_RIGHT,    // 5
    Z_INSTRUMENT,        // 6
    Z_ROOF_FRONT,        // 7  40 像素 — 可被效果 A 单独占有
    Z_ROOF_MID,          // 8  40 像素 — A 和 B 重叠争用
    Z_ROOF_REAR,         // 9  20 像素 — 可被效果 B 单独占有
    Z_AMBIENT_STRIP,     // 10 50 像素
    ZONE_COUNT           // 计数器，保持最后
};

// ZoneLayout — 描述每个 zone 在像素缓冲池中的位置和像素数
typedef struct {
    uint16_t offset;       // 在像素缓冲池中的起始位置
    uint16_t pixel_count;  // 这个 zone 有几个像素
} ZoneLayout;

static const ZoneLayout g_zone_layout[ZONE_COUNT] = {
    [Z_FRONT_LEFT_DOOR]  = { 0,   1 },
    [Z_FRONT_RIGHT_DOOR] = { 1,   1 },
    // ...
    [Z_ROOF_FRONT]       = { 7,   40 },
    [Z_ROOF_MID]         = { 47,  40 },  // ← rl1 和 rl2 重叠
    [Z_ROOF_REAR]        = { 87,  20 },
    [Z_AMBIENT_STRIP]    = { 107, 50 },
};

// 分组函数 — 按物理布局预定义，效果工厂里一行引用
ZoneMask mask = zm_group_all_doors();   // 四个车门
ZoneMask mask = zm_group_roof_all();    // 车顶三段
ZoneMask mask = zm_group_all_zones();   // 全部 zone
```

核心位运算（`include/types.h`）：

| 函数 | 用途 |
|------|------|
| `zm_clear` / `zm_set` / `zm_set_range` | 构建 mask |
| `zm_test` | 测试某个 zone 是否在 mask 中 |
| `zm_intersects` | 两份 mask 是否有重叠 |
| `zm_andnot` | 从 mask 中移除另一份 mask 的位 |
| `zm_is_empty` | 判断 mask 是否全空 |
| `zm_or` | 合并 mask |

#### Zone vs Pixel — 两层寻址

**zone = 逻辑光照区域，是仲裁和所有权的基本单位。** ZoneMask 和 Effect.mask 工作在 zone 层。

**pixel = zone 内部的单个 LED。** 点光源的 zone 只有 1 个像素，流水灯带的 zone 可以有几十上百个像素。

两层寻址各用各的方式：
- **枚举** 定位 zone：`zm_set(&mask, Z_ROOF_STRIP)`，枚举只写到 zone 级别
- **数字索引** 遍历 zone 内的像素：`out->pixels[zl->offset + i]`，不需要枚举

```c
// 点光源 — zone_fill 设置整个 zone 的所有像素（这里只有 1 个）
zone_fill(out, Z_FRONT_LEFT_DOOR, 0, 100, 255, bright);

// 流水灯带 — 直接按像素索引逐个写入不同颜色
const ZoneLayout *zl = &g_zone_layout[Z_ROOF_STRIP];
for (int i = 0; i < zl->pixel_count; i++) {
    LedPixel *p = &out->pixels[zl->offset + i];
    p->r = wave_r(i, tick);
    p->g = wave_g(i, tick);
    p->b = wave_b(i, tick);
    p->brightness = calc_brightness(i, tick);
}
```

`LedOutput` 是一个扁平化的像素缓冲池，所有 zone 的像素连续存放：

```c
typedef struct {
    LedPixel pixels[TOTAL_PIXELS];  // 157 = 1+1+...+100+50
} LedOutput;
```

`zone_fill()` 是便利函数，为点光源效果简化代码——一次调用填满一个 zone 的所有像素。流水灯效则直接用 offset + index 访问每个像素。

### 2. Effect — 灯效对象

```c
typedef struct Effect {
    const char *name;      // 效果名，对应命令里的 effect_name
    int         priority;  // 优先级，越大越高（1~255）
    ZoneMask    mask;      // 管辖哪些 zone，工厂里设定后不动
    TimeMs      start_ts;  // 启动时刻，动画的时钟原点
    EffectState state;     // IDLE / ACTIVE / SHADOWED

    void (*init)  (struct Effect*, TimeMs now);
    void (*update)(struct Effect*, TimeMs now, int allowed, LedOutput *out);
    void (*deinit)(struct Effect*);
} Effect;
```

虚方法表风格，三种方法的行为约定：

- **init** — 效果首次激活时调用一次。设置 `start_ts = now`，状态切到 ACTIVE。
- **update** — 每帧调用。`allowed=1` 时正常输出颜色到 `out`（遍历 `e->mask` 写对应 zone）。
  `allowed=0` 时不写输出，但动画时钟继续推进，保证恢复后相位连续。
- **deinit** — 效果停用时调用一次，状态切回 IDLE。收尾清理用。

#### 对象池

```c
#define EFFECT_POOL_SIZE 32
static Effect g_pool[32];   // 32 个槽位，注册时分配
static int    g_used[32];   // 使用标记
```

效果在注册时从池中分配，整个运行期间不归还。shutdown 时统一回收。
替代 malloc/free，满足 MCU 无动态内存的限制。

写一个新灯效只需：实现 init/update/deinit + 一个工厂函数，然后在 main.c 加一行注册。

### 3. EffectManager — 调度核心

#### 注册阶段

`em_register_factory()` 立即调用工厂创建效果，按优先级降序插入 effects[] 数组：

```
注册后的 effects[]:
  [0] rear_alert       pri=10  mask={2,3}
  [1] welcome          pri=7   mask={0,1,2,3}
  [2] running_light_2  pri=6   mask={8,9}      ← 重叠在 Z_ROOF_MID
  [3] running_light_1  pri=5   mask={7,8}
  [4] ambient          pri=1   mask={0..10}
```

之后遍历就是从高到低，不需要任何排序。

#### 运行时状态机

```
    ┌──────────┐  CMD_ACTIVATE  ┌──────────┐
    │   IDLE   │ ─────────────→ │  ACTIVE  │
    └──────────┘                └──────────┘
         ↑                           │  ↑
         │  CMD_DEACTIVATE           │  │ 部分 zone 被高优先级
         │                           │  │ 效果占用，导致自己
         │                           ▼  │ 全部 zone 都被占
         │                      ┌───────────┐
         └──────────────────────│ SHADOWED  │
                                └───────────┘
                                     │  所有被占 zone 释放
                                     └────→ 回到 ACTIVE
```

#### 每帧调度（按 zone 粒度仲裁）

```
occupied = {}   // 已被更高优先级效果渲染的 zone 集合

for each effect in effects[] (高→低):
    if state == IDLE → skip

    render_mask = effect.mask - occupied   // 该效果还能渲染的 zone

    if render_mask 全空:
        state = SHADOWED
        update(allowed=0)    // 效果自己时钟继续走，不写输出
    else:
        state = ACTIVE
        临时把 e.mask 替换为 render_mask
        update(allowed=1)    // 效果照常遍历 mask，只写未被占的 zone
        恢复 e.mask
        occupied |= render_mask
```

**按 zone 粒度仲裁的含义**（以车顶灯带三段为例）：

```
车顶布局:
  ┌──────────────┬──────────────┬────────┐
  │ Roof-Front   │  Roof-Mid    │Roof-Rear│
  │   (40 px)    │   (40 px)    │ (20 px) │
  └──────────────┴──────────────┴────────┘
       rl1 要          rl2 也要
       (cyan)       两者争用  (orange)

效果注册:
  rl1: pri=5, mask={Z_ROOF_FRONT, Z_ROOF_MID}
  rl2: pri=6, mask={Z_ROOF_MID, Z_ROOF_REAR}
```

rl1 和 rl2 同时激活时：
- rl2 先渲染（pri=6 > 5）：拿走 Z_ROOF_MID + Z_ROOF_REAR。occupied = {mid, rear}
- rl1 渲染：render_mask = {front, mid} - {mid, rear} = {front}
- 结果：车顶前段流光溢彩(cyan)，中段+后段橙色跑马灯(orange)

rl1 没有被整体影子化，只是让出了重叠的 Z_ROOF_MID。只有当 render_mask 全空时
（比如某个 pri=10 的效果 mask 完全覆盖 rl1），rl1 才进入 SHADOWED。

---

## 硬件抽象层 (HAL)

四个接口，PC 和 MCU 各自实现：

```c
void     hal_init(void);                     // 平台初始化
uint32_t hal_millis(void);                   // 毫秒时间戳（32位，约49天回绕）
void     hal_delay_ms(uint32_t ms);          // 毫秒级延时
int      hal_can_read(uint8_t frame[64]);    // 读一帧 CAN FD，无数据返回 0
void     hal_led_send(const LedOutput *out); // 将所有像素数据发送到 LED 驱动
```

- PC 端：`src/hal/hal_pc.c` — `clock_gettime` / `nanosleep` / ANSI 终端模拟 LED
- MCU 端：替换为硬件定时器 / CAN 中断 / SPI 或 LIN 驱动

所有上层代码依赖这四个接口，不依赖任何 PC 或 MCU 特定 API。

---

## 目录结构

```
include/
  types.h             ZoneMask 位运算、TimeMs、Event、EffectCommand
  led_map.h           zone 编号枚举、分组函数、LedPixel/LedOutput
  hal.h               HAL 接口声明

src/
  main.c              主循环（10ms 节拍），五层流水线串联
  can/
    parser.c          CAN FD 帧 → Event
  commands/
    processor.c       Event → EffectCommand（边沿检测+计数逻辑）
  events/
    queue.c           Event 环形缓冲
  manager/
    effect_manager.c  效果注册、命令处理、每帧调度+仲裁
  render/
    renderer.c        LED 输出提交
  effects/
    effect.h/c        效果基类、对象池、虚方法表
    welcome.c         迎宾灯效 (pri=7, 四门蓝色呼吸)
    rear_alert.c      后雷达告警 (pri=10, 后门红色闪烁)
    running_light.c   流水灯效 (两个工厂: rl1 cyan 前中段 pri=5, rl2 orange 中后段 pri=6)
    ambient.c         背景氛围灯 (pri=1, 全 zone 暖白)
  hal/
    hal_pc.c          PC 端 HAL 实现 + ANSI 终端仪表板
    can_sim.c/h       CAN 场景注入器
```

---

## 新增灯效步骤

1. 在 `src/effects/` 下创建 `my_effect.c`（和对应的 `.h`）
2. 实现 `my_init / my_update / my_deinit` 三个函数
3. 写 `my_effect_factory()`：调用 `effect_create_base(name, priority, mask)` + 挂上三个函数
4. 在 `main.c` 中加一行 `em_register_factory("my_effect", my_effect_factory)`
5. 如果需要新的 CAN 信号触发，在 `processor.c` 的 `event_to_command()` 里加对应的 case
6. 如果是新的多像素 zone，在 `led_map.h` 的 enum 和 `g_zone_layout[]` 中各加一行，更新 `TOTAL_PIXELS`
7. 如果是新的点光源 zone，步骤同上但 pixel_count=1

调度器、队列、仲裁逻辑不需要任何修改。

**点光源 vs 流水灯带** 的区别只在 `update()` 里怎么写：
- 点光源用 `zone_fill(out, zone, r, g, b, br)` 一行搞定
- 流水灯带通过 `g_zone_layout[zone].offset + pixel_index` 逐个像素写入不同颜色

---

## 关键技术决策

| 决策 | 原因 |
|------|------|
| TimeMs 用 uint32_t | 32 位 MCU 原生字长，49 天回绕对车载设备可接受 |
| 对象池替代 malloc | MCU 无堆或禁止动态分配 |
| 效果注册时创建、按优先级排序插入 | 避免每帧 qsort，减少运行时开销 |
| 按 zone 粒度仲裁 | 高优先级效果只占用自己需要的 zone，低优先级效果其余 zone 继续渲染 |
| 被遮挡效果时钟不冻结 | 恢复正常渲染时动画相位连续，无跳帧感 |
| 边沿检测用计数方式 | 多扇门、多路雷达时激活/停用逻辑自动正确 |
| ZoneMask 编译时可配 | -DMAX_ZONES=N，适应不同车型的 LED 数量 |
| zone 和 pixel 两层寻址 | zone 用枚举（仲裁/所有权的单位），pixel 用数字索引（渲染细节）。流水灯带的 100 颗 LED 只占用 1 个 zone 枚举值，不需要逐颗枚举 |
