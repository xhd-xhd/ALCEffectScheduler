#ifndef LED_MAP_H
#define LED_MAP_H
#include "types.h"

// ============================================================================
// Zone 定义 — 车身内每个灯光区域的编号
// ============================================================================
// 新增 zone 时加在 ZONE_COUNT 前面，ZONE_COUNT 保持最后一个。
// ZoneMask 用位图表示"哪些 zone 被选中"，这里的编号就是位图中的 bit 位置。
//
// zone = 逻辑光照区域（仲裁的基本单位）
// 点光源（门灯、脚窝灯）一个 zone 只有 1 个像素
// 流水灯带（车顶灯带、氛围灯带）一个 zone 包含多个像素，内部用数字索引
enum {
    Z_FRONT_LEFT_DOOR,   // 0  左前门
    Z_FRONT_RIGHT_DOOR,  // 1  右前门
    Z_REAR_LEFT_DOOR,    // 2  左后门
    Z_REAR_RIGHT_DOOR,   // 3  右后门
    Z_FOOTWELL_LEFT,     // 4  左脚窝
    Z_FOOTWELL_RIGHT,    // 5  右脚窝
    Z_INSTRUMENT,        // 6  仪表台
    Z_ROOF_FRONT,        // 7  车顶前段 (40 像素)
    Z_ROOF_MID,          // 8  车顶中段 (40 像素) ← rl1 和 rl2 重叠争用
    Z_ROOF_REAR,         // 9  车顶后段 (20 像素)
    Z_AMBIENT_STRIP,     // 10 氛围灯带 (50 像素)
    ZONE_COUNT           // 11 计数标记，保持最后
};

// ============================================================================
// Zone 像素布局 — 描述每个 zone 在像素缓冲池中的位置和像素数
// ============================================================================
typedef struct {
    uint16_t offset;       // 在像素缓冲池中的起始位置
    uint16_t pixel_count;  // 这个 zone 有几个像素
} ZoneLayout;

// 每个 zone 的像素数量配置 —— 按实际硬件连线定义
// 新增 zone 时在这里补一行，然后更新 TOTAL_PIXELS
static const ZoneLayout g_zone_layout[ZONE_COUNT] = {
    [Z_FRONT_LEFT_DOOR]  = { 0,   1 },
    [Z_FRONT_RIGHT_DOOR] = { 1,   1 },
    [Z_REAR_LEFT_DOOR]   = { 2,   1 },
    [Z_REAR_RIGHT_DOOR]  = { 3,   1 },
    [Z_FOOTWELL_LEFT]    = { 4,   1 },
    [Z_FOOTWELL_RIGHT]   = { 5,   1 },
    [Z_INSTRUMENT]       = { 6,   1 },
    [Z_ROOF_FRONT]       = { 7,   40 },
    [Z_ROOF_MID]         = { 47,  40 },
    [Z_ROOF_REAR]        = { 87,  20 },
    [Z_AMBIENT_STRIP]    = { 107, 50 },
};

// 像素总和 = 1+1+1+1+1+1+1+40+40+20+50，必须和 g_zone_layout 保持同步
#define TOTAL_PIXELS 157

// ============================================================================
// 每帧输出的 LED 颜色值
// ============================================================================
typedef struct { uint8_t r, g, b; uint8_t brightness; } LedPixel;

// 扁平化像素缓冲池 —— 所有 zone 的像素连续存放
// zone_fill() 按 g_zone_layout 写入对应 zone 的像素
// 流水灯效可直接用 g_zone_layout[zone].offset + pixel_index 访问
typedef struct {
    LedPixel pixels[TOTAL_PIXELS];
} LedOutput;

// 便利函数: 设置一个 zone 内所有像素为相同颜色（点光源和灯带通用）
static inline void zone_fill(LedOutput *out, int zone,
                             uint8_t r, uint8_t g, uint8_t b, uint8_t br) {
    const ZoneLayout *zl = &g_zone_layout[zone];
    for (int i = 0; i < zl->pixel_count; i++) {
        out->pixels[zl->offset + i].r          = r;
        out->pixels[zl->offset + i].g          = g;
        out->pixels[zl->offset + i].b          = b;
        out->pixels[zl->offset + i].brightness = br;
    }
}

// ============================================================================
// Zone 分组 — 按物理布局把常用组合预定义好，效果工厂里直接用名字引用
// ============================================================================
static inline ZoneMask zm_group_all_doors(void) {
    ZoneMask m; zm_clear(&m);
    zm_set(&m, Z_FRONT_LEFT_DOOR);
    zm_set(&m, Z_FRONT_RIGHT_DOOR);
    zm_set(&m, Z_REAR_LEFT_DOOR);
    zm_set(&m, Z_REAR_RIGHT_DOOR);
    return m;
}

static inline ZoneMask zm_group_front_doors(void) {
    ZoneMask m; zm_clear(&m);
    zm_set(&m, Z_FRONT_LEFT_DOOR);
    zm_set(&m, Z_FRONT_RIGHT_DOOR);
    return m;
}

static inline ZoneMask zm_group_rear_doors(void) {
    ZoneMask m; zm_clear(&m);
    zm_set(&m, Z_REAR_LEFT_DOOR);
    zm_set(&m, Z_REAR_RIGHT_DOOR);
    return m;
}

static inline ZoneMask zm_group_roof_all(void) {
    ZoneMask m; zm_clear(&m);
    zm_set(&m, Z_ROOF_FRONT);
    zm_set(&m, Z_ROOF_MID);
    zm_set(&m, Z_ROOF_REAR);
    return m;
}

static inline ZoneMask zm_group_feet_wells(void) {
    ZoneMask m; zm_clear(&m);
    zm_set(&m, Z_FOOTWELL_LEFT);
    zm_set(&m, Z_FOOTWELL_RIGHT);
    return m;
}

static inline ZoneMask zm_group_all_zones(void) {
    ZoneMask m; zm_clear(&m);
    for (int i = 0; i < ZONE_COUNT; i++) zm_set(&m, i);
    return m;
}

#endif
