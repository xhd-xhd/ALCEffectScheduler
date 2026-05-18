#include "theme_effect.h"
#include "led_map.h"
#include "can_frames.h"

// ============================================================================
// theme_effect1~4 — AtmThemeMod=2~5，预设主题 (优先级 2)
// ============================================================================
// 管辖 zone: 全部
// 颜色: 预设 R/G/B，4 个主题仅颜色不同
// 亮度: 实时读 0x2CF 帧的 4 层级 BriSet 信号

typedef struct {
    uint8_t r_upper, g_upper, b_upper;
    uint8_t r_mid,   g_mid,   b_mid;
    uint8_t r_map,   g_map,   b_map;
    uint8_t r_foot,  g_foot,  b_foot;
} ThemePreset;

static uint8_t layer_brightness(ZoneLayer layer) {
    switch (layer) {
    case LAYER_UPPER: return g_rx_frame.canfd_0x2cf.VIU_AL_HiLampBriSet;
    case LAYER_MID:   return g_rx_frame.canfd_0x2cf.VIU_AL_NeutLampBriSet;
    case LAYER_MAP:   return g_rx_frame.canfd_0x2cf.VIU_AL_CeilingLampBriSet;
    case LAYER_FOOT:  return g_rx_frame.canfd_0x2cf.VIU_AL_FootLampBriSet;
    default: return 0;
    }
}

static void te_init(Effect *e, TimeMs now) {
    e->start_ts = now;
    e->state    = S_ACTIVE;
}

static void te_update(Effect *e, TimeMs now, int allowed, LedOutput *out) {
    if (!allowed || !out) return;
    ThemePreset *p = (ThemePreset *)e->params;
    (void)now;

    for (int z = 0; z < ZONE_COUNT; z++) {
        if (!zm_test(&e->mask, z)) continue;
        ZoneLayer ly = zone_layer(z);
        uint8_t   br = layer_brightness(ly);

        switch (ly) {
        case LAYER_UPPER:
            zone_fill(out, z, p->r_upper, p->g_upper, p->b_upper, br, 0); break;
        case LAYER_MID:
            zone_fill(out, z, p->r_mid,   p->g_mid,   p->b_mid,   br, 0); break;
        case LAYER_MAP:
            zone_fill(out, z, p->r_map,   p->g_map,   p->b_map,   br, 0); break;
        case LAYER_FOOT:
            zone_fill(out, z, p->r_foot,  p->g_foot,  p->b_foot,  br, 0); break;
        }
    }
}

static void te_deinit(Effect *e) { (void)e; }

// ---- 预设颜色 (后续由产品定义调整) -----------------------------------------
//   upper=高位灯  mid=中音  map=地图袋  foot=照脚灯

static ThemePreset preset_aurora = {   // 极光: 青绿
    .r_upper=0,  .g_upper=200,.b_upper=180,
    .r_mid=0,    .g_mid=160,  .b_mid=140,
    .r_map=0,    .g_map=180,  .b_map=160,
    .r_foot=0,   .g_foot=140, .b_foot=120,
};
static ThemePreset preset_dawn = {     // 晨曦: 暖橙
    .r_upper=255,.g_upper=160,.b_upper=60,
    .r_mid=255,  .g_mid=130,  .b_mid=40,
    .r_map=255,  .g_map=150,  .b_map=50,
    .r_foot=255, .g_foot=110, .b_foot=30,
};
static ThemePreset preset_moonshadow = { // 月影: 蓝紫
    .r_upper=80, .g_upper=100,.b_upper=220,
    .r_mid=60,   .g_mid=80,   .b_mid=200,
    .r_map=70,   .g_map=90,   .b_map=210,
    .r_foot=50,  .g_foot=70,  .b_foot=180,
};
static ThemePreset preset_bonfire = {   // 篝火: 红橙
    .r_upper=255,.g_upper=100,.b_upper=20,
    .r_mid=255,  .g_mid=70,   .b_mid=10,
    .r_map=255,  .g_map=90,   .b_map=15,
    .r_foot=255, .g_foot=60,  .b_foot=5,
};

// ---- 工厂 ---------------------------------------------------------------
static Effect *te_factory(const char *name, ThemePreset *preset) {
    ZoneMask mask = zm_group_all_zones();
    Effect *e = effect_create_base(name, 2, mask);
    if (!e) return NULL;
    e->params = preset;
    e->init   = te_init;
    e->update = te_update;
    e->deinit = te_deinit;
    return e;
}

Effect *theme_effect1_factory(void) { return te_factory("theme_effect1", &preset_aurora); }
Effect *theme_effect2_factory(void) { return te_factory("theme_effect2", &preset_dawn); }
Effect *theme_effect3_factory(void) { return te_factory("theme_effect3", &preset_moonshadow); }
Effect *theme_effect4_factory(void) { return te_factory("theme_effect4", &preset_bonfire); }
