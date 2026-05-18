#include "theme_custom.h"
#include "led_map.h"
#include "can_frames.h"

// ============================================================================
// theme_custom — AtmThemeMod=1，用户自定义模式 (优先级 2)
// ============================================================================
// 管辖 zone: 全部
// 信号来源: 0x2CF 帧的 4 个层级颜色+亮度
//   CeilingLamp → LAYER_MAP
//   HiLamp      → LAYER_UPPER
//   NeutLamp    → LAYER_MID
//   FootLamp    → LAYER_FOOT

static void tc_init(Effect *e, TimeMs now) {
    e->start_ts = now;
    e->state    = S_ACTIVE;
}

static void tc_update(Effect *e, TimeMs now, int allowed, LedOutput *out) {
    if (!allowed || !out) return;
    (void)now;

    for (int z = 0; z < ZONE_COUNT; z++) {
        if (!zm_test(&e->mask, z)) continue;

        switch (zone_layer(z)) {
        case LAYER_UPPER:
            zone_fill(out, z,
                g_rx_frame.canfd_0x2cf.VIU_AL_HiLampColorSet_Red,
                g_rx_frame.canfd_0x2cf.VIU_AL_HiLampColorSet_Green,
                g_rx_frame.canfd_0x2cf.VIU_AL_HiLampColorSet_Blue,
                g_rx_frame.canfd_0x2cf.VIU_AL_HiLampBriSet, 0);
            break;
        case LAYER_MID:
            zone_fill(out, z,
                g_rx_frame.canfd_0x2cf.VIU_AL_NeutLampColorSet_Red,
                g_rx_frame.canfd_0x2cf.VIU_AL_NeutLampColorSet_Green,
                g_rx_frame.canfd_0x2cf.VIU_AL_NeutLampColorSet_Blue,
                g_rx_frame.canfd_0x2cf.VIU_AL_NeutLampBriSet, 0);
            break;
        case LAYER_MAP:
            zone_fill(out, z,
                g_rx_frame.canfd_0x2cf.VIU_AL_CeilingLampColorSet_Red,
                g_rx_frame.canfd_0x2cf.VIU_AL_CeilingLampColorSet_Green,
                g_rx_frame.canfd_0x2cf.VIU_AL_CeilingLampColorSet_Blue,
                g_rx_frame.canfd_0x2cf.VIU_AL_CeilingLampBriSet, 0);
            break;
        case LAYER_FOOT:
            zone_fill(out, z,
                g_rx_frame.canfd_0x2cf.VIU_AL_FootLampColorSet_Red,
                g_rx_frame.canfd_0x2cf.VIU_AL_FootLampColorSet_Green,
                g_rx_frame.canfd_0x2cf.VIU_AL_FootLampColorSet_Blue,
                g_rx_frame.canfd_0x2cf.VIU_AL_FootLampBriSet, 0);
            break;
        }
    }
}

static void tc_deinit(Effect *e) { (void)e; }

Effect *theme_custom_factory(void) {
    ZoneMask mask = zm_group_all_zones();
    Effect *e = effect_create_base("theme_custom", 2, mask);
    if (!e) return NULL;
    e->init   = tc_init;
    e->update = tc_update;
    e->deinit = tc_deinit;
    return e;
}
