#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "hal.h"
#include "types.h"
#include "led_map.h"
#include "manager/effect_manager.h"
#include "can/parser.h"
#include "render/renderer.h"
#include "commands/processor.h"
#include "events/queue.h"
#include "effects/rear_alert.h"
#include "effects/welcome.h"
#include "effects/ambient.h"
#include "effects/running_light.h"

// ============================================================================
// 主循环 — 10ms 节拍的调度脉搏
// ============================================================================
//
// 数据流向（五层流水线）:
//
//   CAN FD 帧 (64 bytes)
//     │  hal_can_read()
//     ▼
//   can_parse_frame() ──→ Event 队列
//     │  event_to_command()  (边沿检测: 0→1 激活, 1→0 停用)
//     ▼
//   EffectCommand 队列
//     │  em_process_command()
//     ▼
//   EffectManager 调度
//     │  em_update_all()  (优先级排序 → 仲裁 → render)
//     ▼
//   LedOutput ──→ hal_led_send()  (输出到 LED 硬件)
//
// MCU 移植时只需:
//   1. 替换 src/hal/ 下的 PC 实现为硬件驱动
//   2. 替换 can/parser.c 为真实 CAN 信号解析
//   3. 替换 commands/processor.c 的 event_to_command 为实际信号映射
//   4. 开发 src/effects/ 下的灯效

static volatile int running = 1;
static void on_sigint(int _) { (void)_; running = 0; }

int main(void) {
    signal(SIGINT, on_sigint);

    // ---- 初始化 -------------------------------------------------------------
    hal_init();          // PC: clock_gettime + ANSI 终端; MCU: 硬件初始化
    can_parser_init();   // CAN 帧解析器
    event_queue_init();  // Event 环形缓冲
    commands_init();     // 命令队列 + 边沿检测状态
    em_init();           // EffectManager

    // 注册所有灯效工厂 —— 新增灯效在这里加一行
    em_register_factory("rear_alert",       rear_alert_factory);
    em_register_factory("welcome",          welcome_factory);
    em_register_factory("running_light_2",  running_light_2_factory);  // pri=6, 中段+后段, orange
    em_register_factory("running_light_1",  running_light_1_factory);  // pri=5, 前段+中段, cyan
    em_register_factory("ambient",          ambient_factory);

    printf("\033[2J\033[H");
    printf("ALC Effect Scheduler — starting 10ms loop (Ctrl+C to stop)\n");
    hal_delay_ms(500);

    // ---- 主循环: 10ms 一帧 --------------------------------------------------
    while (running) {
        uint32_t now = hal_millis();

        // 1. CAN 收包 → 事件队列
        uint8_t frame[64];
        while (hal_can_read(frame)) {
            Event events[8];
            int n = can_parse_frame(frame, events, 8);
            for (int i = 0; i < n; i++) {
                events[i].ts = now;
                event_queue_push(&events[i]);
            }
        }

        // 2. 事件 → 命令（含边沿检测）
        {
            Event evt;
            while (event_queue_pop(&evt)) {
                EffectCommand cmd;
                event_to_command(&evt, &cmd);
                if (cmd.type == CMD_ACTIVATE || cmd.type == CMD_DEACTIVATE)
                    command_queue_push(&cmd);
            }
        }

        // 3. 命令 → EffectManager 调度
        {
            EffectCommand cmd;
            while (command_queue_pop(&cmd))
                em_process_command(&cmd);
        }

        // 4. 效果更新 + 渲染输出
        LedOutput out;
        em_update_all(now, &out);
        commit_led_output(&out);   // PC: ANSI 仪表板; MCU: SPI 发送

        // 5. 维持 10ms 节拍
        uint32_t elapsed = hal_millis() - now;
        if (elapsed < 10)
            hal_delay_ms(10 - elapsed);
    }

    // ---- 清理 ----------------------------------------------------------------
    printf("\033[2J\033[H");
    em_shutdown();
    printf("ALC Effect Scheduler — stopped.\n");
    return 0;
}
