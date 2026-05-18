#include "parser.h"
#include "can_frames.h"
#include <string.h>

// 全局接收帧 — 所有 CAN ID 的最新数据都在这里，应用层直接读
CanFrame g_rx_frame;

void can_parser_init(void) {
    memset(&g_rx_frame, 0, sizeof(CanFrame));
}

// 收到一帧 → 根据 ID 填入 g_rx_frame 对应 union
int can_handle_frame(const uint8_t frame[64]) {
    uint32_t can_id = (uint32_t)frame[0] | ((uint32_t)frame[1] << 8)
                    | ((uint32_t)frame[2] << 16) | ((uint32_t)frame[3] << 24);

    switch (can_id) {
    case 0x2CF:
        memcpy(g_rx_frame.canfd_0x2cf._0x2cf_data, frame + 4, 60);
        g_rx_frame.id = can_id;
        return 1;
    case 0x1E9:
        memcpy(g_rx_frame.canfd_0x1e9._0x1e9_data, frame + 4, 60);
        g_rx_frame.id = can_id;
        return 1;
    case 0x1EA:
        memcpy(g_rx_frame.canfd_0x1ea._0x1ea_data, frame + 4, 60);
        g_rx_frame.id = can_id;
        return 1;
    case 0x1EB:
        memcpy(g_rx_frame.canfd_0x1eb._0x1eb_data, frame + 4, 60);
        g_rx_frame.id = can_id;
        return 1;
    case 0x1EC:
        memcpy(g_rx_frame.canfd_0x1ec._0x1ec_data, frame + 4, 60);
        g_rx_frame.id = can_id;
        return 1;
    case 0x1ED:
        memcpy(g_rx_frame.canfd_0x1ed._0x1ed_data, frame + 4, 60);
        g_rx_frame.id = can_id;
        return 1;
    case 0x1EE:
        memcpy(g_rx_frame.canfd_0x1ee._0x1ee_data, frame + 4, 60);
        g_rx_frame.id = can_id;
        return 1;
    default:
        return 0;
    }
}

// 兼容旧接口 — 收到帧后直接产生 Event
int can_parse_frame(const uint8_t frame[64], Event *out_events, int max_out) {
    if (max_out < 1) return 0;
    if (!can_handle_frame(frame)) return 0;

    memset(out_events, 0, sizeof(Event));

    switch (g_rx_frame.id) {
    case 0x2CF:
        out_events[0].type = EVT_ALCTRL;
        return 1;
    case 0x1E9: case 0x1EA: case 0x1EB: case 0x1EC: case 0x1ED:
        out_events[0].type = EVT_MUSIC_FOLLOW;
        return 1;
    case 0x1EE:
        out_events[0].type = EVT_MUSIC_TWEETER;
        return 1;
    default:
        return 0;
    }
}
