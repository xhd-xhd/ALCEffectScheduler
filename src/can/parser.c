#include "parser.h"
#include <string.h>

// ============================================================================
// CAN FD 帧解析器 — 64 字节原始帧 → Event
// ============================================================================
// 帧格式（与 can_sim 约定一致）:
//   bytes 0..3  → CAN ID (uint32 LE)
//   bytes 4..63 → payload (60 bytes)
//
// MCU 移植时替换为你实际的 CAN 信号定义。
// PC 模拟器使用以下 ID:
//   0x100 — 车门状态
//   0x200 — 后雷达距离
//   0x300 — 车速（预留）
//   0x400 — 氛围灯模式

static uint32_t read_u32le(const uint8_t *b) {
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8)
         | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}

void can_parser_init(void) {}

int can_parse_frame(const uint8_t frame[64], Event *out_events, int max_out) {
    if (max_out < 1) return 0;
    uint32_t can_id = read_u32le(frame);
    const uint8_t *d = frame + 4; // payload 从 byte 4 开始

    memset(out_events, 0, sizeof(Event));
    out_events[0].ts = 0; // 由主循环填入实际时间戳

    switch (can_id) {
    case 0x100: // 车门状态: d[0]=门编号, d[1]=开/关
        out_events[0].type = EVT_DOOR;
        out_events[0].data.door.door_index = d[0];
        out_events[0].data.door.opened     = d[1];
        return 1;
    case 0x200: { // 后雷达: d[0]=左右, d[1..4]=float 距离(米)
        out_events[0].type = EVT_REAR_ALERT;
        out_events[0].data.rear.side = d[0];
        float dist;
        memcpy(&dist, d + 1, 4);
        out_events[0].data.rear.distance = dist;
        return 1;
    }
    case 0x300: { // 车速: d[0..1]=uint16 kph*10
        out_events[0].type = EVT_SPEED;
        uint16_t kph_x10;
        memcpy(&kph_x10, d, 2);
        out_events[0].data.speed.kph = kph_x10 / 10.0f;
        return 1;
    }
    case 0x400: // 氛围灯模式: d[0]=模式 ID
        out_events[0].type = EVT_MODE;
        out_events[0].data.mode.mode_id = d[0];
        out_events[0].data.mode.params   = NULL;
        return 1;
    default:
        return 0; // 未知 CAN ID，忽略
    }
}
