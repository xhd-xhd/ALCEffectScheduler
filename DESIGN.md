# ALCEffectScheduler 设计文档

## 数据流

```
CAN FD 帧 (64 bytes)
  │  hal_can_read() → can_handle_frame() → g_rx_frame (CanFrame 位域)
  ▼
commands_tick(now)  读 g_rx_frame → 边沿检测 → EffectCommand 队列
  │  em_process_command()
  ▼
EffectManager 调度 (优先级排序 → zone 仲裁 → render)
  │  em_update_all()
  ▼
LedOutput → hal_led_send()
```

## 核心设计

### 1. Zone 模型 (include/led_map.h)

28 个逻辑灯区 + 4 个层级:

| 位置 | Zone | 层级 |
|------|------|------|
| 仪表流水灯×3 | Z_IP_FLOW_1/2/3 (12+6+9像素) | UPPER |
| 仪表高位灯×6 | Z_IP_H1~H6 (施罗德) | UPPER |
| 左前门×5 | Z_LF_FOOT/MAP/UP_A/UP_B/SPK | FOOT/MAP/UPPER/MID |
| 左后门×4 | Z_LR_FOOT/MAP/UP_A/UP_B | FOOT/MAP/UPPER |
| 右前门×5 | Z_RF_FOOT/MAP/UP_A/UP_B/SPK | FOOT/MAP/UPPER/MID |
| 右后门×4 | Z_RR_FOOT/MAP/UP_A/UP_B | FOOT/MAP/UPPER |
| 中控×1 | Z_CENTER | MAP |

层级: `LAYER_FOOT / MAP / MID / UPPER`，记录在 ZoneLayout.layer，通过 `zone_layer(z)` 查询。

TOTAL_PIXELS = 52, LedPixel = {r,g,b,l,f} 其中 f/10=渐变秒数。

### 2. CAN 帧解析 (include/can_frames.h)

位域 union 直接映射 DBC 信号:

```c
typedef struct {
    union { struct { uint32_t id; uint8_t payload[60]; }; uint8_t raw[64]; };
    union { struct { ... 位域信号 ... }; uint8_t _0x2cf_data[64]; } canfd_0x2cf;
    union { ... } canfd_0x1e9;  // MusicFollowCtrl1
    // ... canfd_0x1ea ~ canfd_0x1ee
} CanFrame;

extern CanFrame g_rx_frame;  // 全局接收帧
```

帧到达 → `can_handle_frame(frame)` → memcpy 进 g_rx_frame 对应 ID 的 union。
应用层直接 `g_rx_frame.canfd_0x2cf.VIU_AL_Welcome` 取值。

### 3. 命令处理器 (src/commands/processor.c)

`commands_tick(now)` 读取 g_rx_frame，每个信号独立边沿检测，互不感知:

| 信号 | 触发 | 命令 |
|------|------|------|
| PowerSt | 0→1/1→0 | ambient |
| AtmThemeMod | 值切换 | theme_off/theme_custom/theme_effect1~4/theme_music |
| NapMod | 0↔非0 | nap |
| StartMotion | 0↔非0 | start_motion |
| Welcome | 0↔非0 | welcome |
| DrivingMod | 0↔非0 | driving |
| FLAC/FRAC/RLAC/RRAC | 任一↔全零 | temperature |
| VoiceInteractIndcrSt | 2/4/7↔其他 | voice |
| DoorWarn 四门 | 任一↔全零 | door_warn |
| RearWarn 四门 | 任一↔全零 | rear_alert |
| TwetterLampEnable | !=1 / ==1 | tweeter_off |
| VideoAmbLiArea×6 | 任一↔全零 + 动态mask | video_ambient |

### 4. 效果优先级链 (低→高)

```
 3  theme_music, video_ambient    ← 氛围主题/光随影动
 2  theme_effect1~4               ← 预设主题
 2  theme_custom                  ← 自定义
 2  theme_off                     ← 全局关灯
 1  ambient                       ← 常驻背景 (PowerSt)
---
 6  welcome                       ← Welcome
 5  start_motion                  ← StartMotion
 4  nap                           ← NapMod
 7  driving                       ← DrivingMod
 8  voice, temperature            ← 语音/空调
 9  door_warn                     ← 开门预警
10  rear_alert                    ← 后方来车
11  tweeter_off                   ← 施罗德开关 (仅 IP_H1~H6)
```

### 5. Zone 仲裁

EffectManager 遍历已排序效果，高优先级先渲染。每个效果只渲染 mask 中未被占用 (occupied) 的 zone。
低优先级效果被占满 → SHADOWED (跳过渲染)。
**动态 mask**: video_ambient 的 mask 由 commands_tick 每帧通过 em_set_mask() 动态设置，
只有当前 active 的区域 zone 才被占用，disabled 区域释放给低优先级。

### 6. Effect 基类 (src/effects/effect.h)

```c
typedef struct Effect {
    const char *name;  int priority;  ZoneMask mask;  void *params;
    TimeMs start_ts;   EffectState state;
    void (*init)(struct Effect*, TimeMs now);
    void (*update)(struct Effect*, TimeMs now, int allowed, LedOutput *out);
    void (*deinit)(struct Effect*);
} Effect;
```

对象池 (32 个)，效果注册时创建，持续存在不销毁，运行时只在 IDLE/ACTIVE/SHADOWED 间切换。

## 效果清单

| 效果 | 文件 | 状态 |
|------|------|------|
| ambient | effects/ambient.c | 已完成 |
| welcome | effects/welcome.c | 已完成 |
| rear_alert | effects/rear_alert.c | 已完成 |
| theme_off | effects/theme_off.c | 已完成 |
| theme_custom | effects/theme_custom.c | 已完成 |
| theme_effect1~4 | effects/theme_effect.c | 已完成 (颜色可调) |
| theme_music | effects/theme_music.c | 已完成 (area映射TODO) |
| video_ambient | effects/video_ambient.c | 已完成 (zone列表可调) |
| tweeter_off | effects/tweeter_off.c | 已完成 |
| running_light_1/2 | effects/running_light.c | 旧demo，视需要保留 |
| footwell_pulse | effects/footwell_pulse.c | 旧demo，视需要保留 |
| nap/start_motion/driving/voice/temperature/door_warn | — | 仅有command，效果文件待后续 |

## PC 模拟器

- `hal_pc.c`: 终端 ANSI 仪表盘，28 zone 显示
- `can_sim.c`: 用 CanFrame 构造真实 0x2CF 帧，13 步演示场景
- Windows 上 clock_gettime/nanosleep 待适配

## 移植到 MCU

1. 替换 `hal_pc.c` → 硬件定时器 + CAN 中断 + SPI LED 驱动
2. 中断里 `can_handle_frame(rx_buf)` 更新 g_rx_frame
3. `commands_tick()` + `em_process_command()` + `em_update_all()` 在 10ms 循环中

---

# 效果过渡设计 (Effect Transition Design)

## 1. 问题分析

### 1.1 当前缺失

效果切换之间没有过渡管理:

- 高优先级效果 A 释放 zone → 低优先级效果 B 接管 → B 直接以满状态写入 LED，太突兀
- 状态切换路径: `A: ACTIVE → deinit() → IDLE` (瞬间消失) + `B: SHADOWED → ACTIVE → update(allowed=1)` (每帧执行正常内容，突然出现)
- 两个事件发生在同一帧里，B 的效果直接以 100% 强度出现在 LED 上，没有任何过渡

### 1.2 真实场景

```
A(high) kills → B(low) appears: 如警报结束→氛围灯恢复，应柔和渐变
A(low) 被 B(high) 抢占:      如开门预警突然触发→压住氛围灯，安全场景需瞬间切换
主题切换:                      theme_custom → theme_music，希望先黑场再亮
部分压制恢复:                  B 的 7 个 zone 正常跑，3 个 zone 被 A 压住，
                              A 死 → 3 个 zone 快速追平其余 7 个 zone 状态
```

## 2. 不需要中间过渡缓冲层

### 2.1 关键认知

`LedPixel.f` 已经是硬件级渐变参数: `f/10 = 秒数`，硬件自动在 `f*100ms` 内从 LED 当前实际颜色平滑过渡到目标色。

```
A 最后一帧写: rgb=蓝, f=0
B 第一帧写:   rgb=暖白, f=20
→ 硬件自动在 2 秒内从蓝色渐变到暖白色，无需软件缓冲层
```

### 2.2 不用额外缓冲的原因

- 硬件已经在做 cross-fade，软件再加一层只会重复
- 效果只需在接管 zone 时写合理的 `f` 值即可实现大多数过渡
- 少数需要"先灭后亮"的场景，由效果自身在头几帧写低亮度来达成

## 3. 过渡策略归属: 效果自治

### 3.1 决策

过渡策略**不集中管理**，写死在每个效果自身的 `init()` / `update()` 里。

原因:
- 需要特殊处理的场景少
- 每个效果过渡方式不同（呼吸、流水、常亮）
- 集中管理会变成配置噩梦
- 不改 EffectManager 框架，不改 EffectCommand 结构

### 3.2 谁负责过渡: 接管方原则

**谁接管 zone，谁决定怎么出现在 LED 上:**

| 方向 | 谁处理入场 | 谁处理退场 |
|------|-----------|-----------|
| B(高) 抢占 A(低) | B 处理 | A 不管 |
| A(低) 从 B(高) 恢复 | A 处理 | B 不管 |
| 效果首次启动 | 自己处理入场 | - |

被压制的效果不需要做退场动作 — 接管方会直接覆盖 LED。

## 4. 三种过渡形态

### 形态 1: 交叉渐变 (A→B 直接过渡)

```
A色 ╲
     ╲___B色
```

B 接管时写目标色 + 大 `f` (如 f=20，2s 渐变)，硬件自动从 A 色过渡到 B 色。

适用场景: ambient 恢复、大多数常规效果切换。

### 形态 2: 先灭后亮 (A→黑→B)

```
A色 ╲
      ╲___黑___╱ B色
```

B 接管后前 N 帧逐步提升亮度:

```
progress 0→1 (过渡期内):
  animation_rgb = 正常动画计算(now - start_ts)  // 动画相位一直在走
  output_rgb    = animation_rgb × progress       // 亮度从 0 爬到正常
  f = 0  // 不用硬件渐变，自己每帧推亮度

过渡结束后:
  output_rgb = animation_rgb  // 无感衔接
  f = 正常值
```

适用场景: 主题切换、警报结束后需要"冷却期"的场景。

### 形态 3: 瞬间硬切

```
A色──B色
```

B 接管时写目标色 + `f=0`，瞬间到位。

适用场景: 安全类效果抢占（door_warn、rear_alert 触发时压住氛围灯）。

## 5. 对称信息模型

### 5.1 需求

不管谁入场谁退场，效果都需要知道"上一个/当前在这个 zone 上跑的是谁":

| 方向 | 需要知道 | 用途 |
|------|---------|------|
| 被压制时 | 谁在压我 | 决定内部行为（时间冻结/继续） |
| 恢复时 | 谁刚走了 | 决定恢复过渡策略 |
| 抢占时 | 我踩了谁 | 决定入场过渡策略 |

### 5.2 Effect 结构体新增字段

```c
typedef struct Effect {
    const char *name;
    int         priority;
    ZoneMask    mask;
    void       *params;
    TimeMs      start_ts;       // 效果本身时钟（init 时设，不动）
    TimeMs      recovery_ts;    // 上次从 SHADOWED 恢复的时刻（0=不在过渡期）
    uint8_t     prev_allowed;   // 上一帧的 allowed 值
    const char *shadowed_by;    // 当前/刚才压我的效果名
    const char *displacing;     // 我踩了谁（init 或恢复第一帧时有效）
    EffectState state;

    void (*init)  (struct Effect*, TimeMs now);
    void (*update)(struct Effect*, TimeMs now, int allowed, LedOutput *out);
    void (*deinit)(struct Effect*);
} Effect;
```

### 5.3 信息填充: em_update_all 加 per-zone owner 追踪

```c
// 每帧维护每个 zone 当前被哪个 effect 占用
static int zone_owner[ZONE_COUNT];  // 存 effects[] 索引，-1 为空

// 仲裁时:
for (each effect by priority) {
    if (S_IDLE) continue;

    render_mask = mask - occupied;

    if (全被占 → SHADOWED) {
        // 取 mask 中第一个被占 zone 的 owner
        e->shadowed_by = get_first_owner_in_mask(e->mask, zone_owner);
    }
    else (有可用 zone → ACTIVE) {
        // 取与 occupied 重叠的 zone 的原 owner → 知道踩了谁
        e->displacing = get_first_overlap_owner(e->mask, occupied, zone_owner);

        // 标记占用
        for (zone in render_mask)
            zone_owner[zone] = effect_index;
        zm_or(&occupied, &render_mask);
    }
}
```

`shadowed_by` 和 `displacing` 存的是同一个关系的两端——效果总能看到"上一状态是谁"。

## 6. 双时间戳设计

### 6.1 两个时间戳各管各的

| 时间戳 | 用途 | 设置时机 |
|--------|------|---------|
| `start_ts` | 动画相位基准 (`now - start_ts`) | init 时设，之后不动 |
| `recovery_ts` | 恢复过渡进度 (`now - recovery_ts`) | allowed 0→1 时设 |

### 6.2 动画相位在压制期间继续走

`allowed=0` 时 `update()` 仍被调用，拿到的是同一个全局 `now` (`hal_millis()`):

```
时间线:
0s —— A 启动 (start_ts=0)
1s —— A 被 B 压制 (allowed=0, update 继续跑，动画相位计算走，不输出)
3s —— A 恢复 (allowed: 0→1, now=3, 动画相位 = 3s，天然正确)

→ A 不需要"追上"什么，它本来就没停
```

关键原因: 部分压制场景 — B 的 7 个 zone 正常跑，3 个 zone 被 A 压住。
如果 A 冻结时间，恢复后 3 个 zone 和 7 个 zone 动画相位不同步，视觉撕裂。

### 6.3 过渡期计算逻辑

```c
static void my_update(Effect *e, TimeMs now, int allowed, LedOutput *out) {
    int recovered = allowed && !e->prev_allowed;
    if (recovered)
        e->recovery_ts = now;

    // 计算正常动画颜色（相位始终基于 start_ts，不受压制影响）
    LedColor anim = calc_animation(e, now - e->start_ts);

    int in_transition = (now - e->recovery_ts) < TRANSITION_MS;
    if (in_transition) {
        float t = (float)(now - e->recovery_ts) / TRANSITION_MS;
        write_zones_with_blend(out, e->mask, anim, t);
        // f=0，自己每帧推亮度
    } else {
        write_zones_direct(out, e->mask, anim);
        // f=正常值
    }

    e->prev_allowed = allowed;
}
```

过渡期结束 → 无感衔接回正常动画。

## 7. 效果入口逻辑

### 7.1 init (首次启动 / 被命令激活)

```c
static void rear_alert_init(Effect *e, TimeMs now) {
    e->start_ts = now;
    e->state = S_ACTIVE;

    if (e->displacing && strcmp(e->displacing, "ambient") == 0) {
        // 踩了氛围灯，瞬间全亮（安全场景，形态3）
        use_cut_transition();
    } else if (e->displacing) {
        // 踩了其他效果，瞬间切换
        use_cut_transition();
    } else {
        // 首次启动，没有踩谁
        use_default_entry();
    }
}
```

### 7.2 update (被压制中 / 刚恢复 / 正常运行)

```c
static void ambient_update(Effect *e, TimeMs now, int allowed, LedOutput *out) {
    // 1. 感知边沿
    int recovered = allowed && !e->prev_allowed;

    if (recovered) {
        e->recovery_ts = now;
    }

    // 2. 计算正常动画状态（时间一直在走）
    LedColor anim = calc_anim(e, now - e->start_ts);

    // 3. 过渡中?
    if (in_transition(e)) {
        float t = (float)(now - e->recovery_ts) / TRANSITION_MS;
        LedColor blended = lerp_color(BLACK, anim, t);
        write_zones(out, e->mask, blended, /*f=*/0);
    } else if (allowed) {
        // 正常跑
        write_zones(out, e->mask, anim, /*f=*/normal);
    }

    // 4. 记状态
    e->prev_allowed = allowed;
}
```

### 7.3 各效果过渡策略总览

| 效果 | 作为抢占方(displacing非空) | 作为恢复方(shadowed_by非空) | 首次启动 |
|------|--------------------------|---------------------------|---------|
| ambient | 不抢占别人 | 形态1, f=20, 大渐变恢复 | 形态1 |
| welcome | 形态1, f=10 | 不恢复（优先级高，一次性） | 形态1 |
| rear_alert | 形态3, f=0, 瞬间切入 | 不恢复（一次性） | 形态3 |
| door_warn | 形态3, f=0, 瞬间切入 | 不恢复（一次性） | 形态3 |
| theme_xxx | 形态2, 先黑后亮, 500ms | 形态2, 先黑后亮, 500ms | 形态2 |
| tweeter_off | 形态3, f=0, 瞬间 | 形态3, f=0, 瞬间 | 形态3 |
| nap 系列 | 形态1, f=30, 大渐变 | 形态2, f=30, 缓慢唤醒 | 形态1 |

## 8. 待确认 / 疑虑

### 8.1 displacing 时效性

`displacing` 只在抢占或恢复发生的第一帧有意义。如果效果不立即消费就永远丢失。
考虑: 是否把 `displacing` 作为 `init()` 的参数传入，而不作为 struct 字段？
(不过这样要改 Effect 的 init 接口签名。)

### 8.2 多层压制的 shadowed_by

B 压 A，C 又压 B。当 C 释放时，B 的 `shadowed_by` 应该还是 C。
B 恢复时只需要知道"上次直接压我的是谁"(LED 上显示的就是 C 的颜色)。

### 8.3 zone_owner 粒度

一个效果可能被多个效果分区压制。`shadowed_by` 取 mask 中第一个被占 zone 的 owner。
大多数情况一个效果只被一个效果压制，够用。

### 8.4 过渡被打断

A 在过渡期间又被 C 抢占 → A 再次 SHADOWED。C 释放后 A 恢复，`recovery_ts` 重新计时。
从头开始入场过渡即可。

### 8.5 displacing 与 shadowed_by 存字符串还是指针

字符串指针更安全（效果不销毁，指针不会野）。`strcmp` 在嵌入式上开销小（少数场景才用）。

## 9. 可能的后续优化（非过渡相关）

1. **`em_update_all` 里临时 swap mask 的做法**: `saved = e->mask; e->mask = render_mask; ... e->mask = saved;`
   — 对效果有 surprise，传 `render_mask` 参数更清晰。

2. **Effect 的 `name` 用枚举替代字符串**: `shadowed_by` / `displacing` 比较时用整数而非 `strcmp`，嵌入式友好。

3. **command 队列满时静默丢弃**: 当前 `push_cmd` 满时无告警，可加入丢帧计数。

4. **theme 名称映射收拢**: `processor.c` 和 theme 效果文件各有一份映射表，考虑统一。
