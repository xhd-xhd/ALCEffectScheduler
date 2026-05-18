#ifndef LED_MAP_H
#define LED_MAP_H
#include "types.h"

// ============================================================================
// Zone 层级 — 按车身高度分层
// ============================================================================
typedef enum {
    LAYER_FOOT,   // 照脚灯
    LAYER_MAP,    // 地图袋
    LAYER_MID,    // 中音 / 扬声器
    LAYER_UPPER,  // 上装 (高位灯 / 顶灯)
} ZoneLayer;

// ============================================================================
// Zone 定义 — 28 个逻辑灯区
// ============================================================================
enum {
    // 流水灯 (仪表 3 段 PCB) — 3 个多像素 zone
    Z_IP_FLOW_1,   // 0   12 像素
    Z_IP_FLOW_2,   // 1    6 像素
    Z_IP_FLOW_3,   // 2    9 像素

    // 仪表高位灯 (6)
    Z_IP_H1,       // 3
    Z_IP_H2,       // 4
    Z_IP_H3,       // 5
    Z_IP_H4,       // 6
    Z_IP_H5,       // 7
    Z_IP_H6,       // 8

    // 左前门 (5)
    Z_LF_FOOT,     // 9
    Z_LF_MAP,      // 10
    Z_LF_UP_A,     // 11
    Z_LF_UP_B,     // 12
    Z_LF_SPK,      // 13

    // 左后门 (4)
    Z_LR_FOOT,     // 14
    Z_LR_MAP,      // 15
    Z_LR_UP_A,     // 16
    Z_LR_UP_B,     // 17

    // 右前门 (5)
    Z_RF_FOOT,     // 18
    Z_RF_MAP,      // 19
    Z_RF_UP_A,     // 20
    Z_RF_UP_B,     // 21
    Z_RF_SPK,      // 22

    // 右后门 (4)
    Z_RR_FOOT,     // 23
    Z_RR_MAP,      // 24
    Z_RR_UP_A,     // 25
    Z_RR_UP_B,     // 26

    // 中控 (1)
    Z_CENTER,      // 27

    ZONE_COUNT     // 28
};

// ============================================================================
// Zone 像素布局 — offset 在扁平缓冲池中的位置
// ============================================================================
typedef struct {
    uint16_t  offset;
    uint16_t  pixel_count;
    ZoneLayer layer;
} ZoneLayout;

static const ZoneLayout g_zone_layout[ZONE_COUNT] = {
    // 流水灯 (27 像素: 12+6+9)
    [Z_IP_FLOW_1]   = { 0,  12, LAYER_UPPER },
    [Z_IP_FLOW_2]   = { 12,  6, LAYER_UPPER },
    [Z_IP_FLOW_3]   = { 18,  9, LAYER_UPPER },

    // 仪表高位灯 (6)
    [Z_IP_H1]       = { 27,  1, LAYER_UPPER },
    [Z_IP_H2]       = { 28,  1, LAYER_UPPER },
    [Z_IP_H3]       = { 29,  1, LAYER_UPPER },
    [Z_IP_H4]       = { 30,  1, LAYER_UPPER },
    [Z_IP_H5]       = { 31,  1, LAYER_UPPER },
    [Z_IP_H6]       = { 32,  1, LAYER_UPPER },

    // 左前门
    [Z_LF_FOOT]     = { 33,  1, LAYER_FOOT  },
    [Z_LF_MAP]      = { 34,  1, LAYER_MAP   },
    [Z_LF_UP_A]     = { 35,  1, LAYER_UPPER },
    [Z_LF_UP_B]     = { 36,  1, LAYER_UPPER },
    [Z_LF_SPK]      = { 37,  1, LAYER_MID   },

    // 左后门
    [Z_LR_FOOT]     = { 38,  1, LAYER_FOOT  },
    [Z_LR_MAP]      = { 39,  1, LAYER_MAP   },
    [Z_LR_UP_A]     = { 40,  1, LAYER_UPPER },
    [Z_LR_UP_B]     = { 41,  1, LAYER_UPPER },

    // 右前门
    [Z_RF_FOOT]     = { 42,  1, LAYER_FOOT  },
    [Z_RF_MAP]      = { 43,  1, LAYER_MAP   },
    [Z_RF_UP_A]     = { 44,  1, LAYER_UPPER },
    [Z_RF_UP_B]     = { 45,  1, LAYER_UPPER },
    [Z_RF_SPK]      = { 46,  1, LAYER_MID   },

    // 右后门
    [Z_RR_FOOT]     = { 47,  1, LAYER_FOOT  },
    [Z_RR_MAP]      = { 48,  1, LAYER_MAP   },
    [Z_RR_UP_A]     = { 49,  1, LAYER_UPPER },
    [Z_RR_UP_B]     = { 50,  1, LAYER_UPPER },

    // 中控
    [Z_CENTER]      = { 51,  1, LAYER_MAP   },
};

#define TOTAL_PIXELS 52

// ============================================================================
// LED 输出结构 — 扁平化像素缓冲池
// ============================================================================
// f/10 = 从当前实际颜色过渡到目标颜色的秒数（硬件 slew）
typedef struct { uint8_t r, g, b; uint8_t l; uint8_t f; } LedPixel;

typedef struct {
    LedPixel pixels[TOTAL_PIXELS];
} LedOutput;

// ---- 便利函数 --------------------------------------------------------------

// 设置一个 zone 内所有像素为相同颜色
static inline void zone_fill(LedOutput *out, int zone,
                             uint8_t r, uint8_t g, uint8_t b, uint8_t l, uint8_t f) {
    const ZoneLayout *zl = &g_zone_layout[zone];
    for (int i = 0; i < zl->pixel_count; i++) {
        out->pixels[zl->offset + i].r = r;
        out->pixels[zl->offset + i].g = g;
        out->pixels[zl->offset + i].b = b;
        out->pixels[zl->offset + i].l = l;
        out->pixels[zl->offset + i].f = f;
    }
}

// 查一个 zone 的层级
static inline ZoneLayer zone_layer(int z) {
    return g_zone_layout[z].layer;
}

// ============================================================================
// Zone 分组 — 按物理位置 / 层级预定义，效果里直接用名字引用
// ============================================================================

// ---- 按位置 ----------------------------------------------------------------

static inline ZoneMask zm_group_flow_all(void) {
    ZoneMask m; zm_clear(&m);
    zm_set(&m, Z_IP_FLOW_1);
    zm_set(&m, Z_IP_FLOW_2);
    zm_set(&m, Z_IP_FLOW_3);
    return m;
}

static inline ZoneMask zm_group_ip_all(void) {
    ZoneMask m; zm_clear(&m);
    for (int i = Z_IP_H1; i <= Z_IP_H6; i++) zm_set(&m, i);
    return m;
}

static inline ZoneMask zm_group_door_lf(void) {
    ZoneMask m; zm_clear(&m);
    zm_set_range(&m, Z_LF_FOOT, 5);
    return m;
}

static inline ZoneMask zm_group_door_lr(void) {
    ZoneMask m; zm_clear(&m);
    zm_set_range(&m, Z_LR_FOOT, 4);
    return m;
}

static inline ZoneMask zm_group_door_rf(void) {
    ZoneMask m; zm_clear(&m);
    zm_set_range(&m, Z_RF_FOOT, 5);
    return m;
}

static inline ZoneMask zm_group_door_rr(void) {
    ZoneMask m; zm_clear(&m);
    zm_set_range(&m, Z_RR_FOOT, 4);
    return m;
}

static inline ZoneMask zm_group_all_doors(void) {
    ZoneMask m, t; zm_clear(&m);
    t = zm_group_door_lf(); zm_or(&m, &t);
    t = zm_group_door_lr(); zm_or(&m, &t);
    t = zm_group_door_rf(); zm_or(&m, &t);
    t = zm_group_door_rr(); zm_or(&m, &t);
    return m;
}

// ---- 按层级 ----------------------------------------------------------------

static inline ZoneMask zm_group_layer(ZoneLayer layer) {
    ZoneMask m; zm_clear(&m);
    for (int z = 0; z < ZONE_COUNT; z++)
        if (g_zone_layout[z].layer == layer) zm_set(&m, z);
    return m;
}

static inline ZoneMask zm_group_all_zones(void) {
    ZoneMask m; zm_clear(&m);
    for (int z = 0; z < ZONE_COUNT; z++) zm_set(&m, z);
    return m;
}

#endif
