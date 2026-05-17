#ifndef HAL_H
#define HAL_H
#include <stdint.h>
#include "led_map.h"

// ============================================================================
// 硬件抽象层 — PC 和 MCU 各自实现这四个接口
// ============================================================================
// PC 端: src/hal/hal_pc.c  （clock_gettime / nanosleep / ANSI 终端模拟 LED）
// MCU 端: 替换为硬件定时器 / CAN 中断 / SPI 驱动

void     hal_init(void);                     // 平台初始化
uint32_t hal_millis(void);                   // 系统启动后的毫秒计数，32 位
void     hal_delay_ms(uint32_t ms);          // 毫秒级延时
int      hal_can_read(uint8_t frame[64]);    // 读一帧 CAN FD（64字节），无数据返回 0
void     hal_led_send(const LedOutput *out); // 将所有 zone 的像素数据发送到 LED 驱动芯片

#endif
