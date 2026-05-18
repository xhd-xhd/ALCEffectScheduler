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
