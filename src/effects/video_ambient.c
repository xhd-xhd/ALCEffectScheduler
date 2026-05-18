#include "video_ambient.h"
#include "led_map.h"
#include "can_frames.h"

// ============================================================================
// video_ambient — 光随影动 (优先级 3)
// ============================================================================
// mask: 动态 — commands_tick 每帧根据 area flag 计算后通过 em_set_mask 更新

static const int g_video_fl_zones[] = {
    Z_LF_FOOT, Z_LF_MAP, Z_LF_UP_A, Z_LF_UP_B,
};
static const int g_video_fr_zones[] = {
    Z_RF_FOOT, Z_RF_MAP, Z_RF_UP_A, Z_RF_UP_B,
};
static const int g_video_ml_zones[] = {
    Z_IP_H1, Z_IP_H2, Z_IP_H3,  // TODO: 实际映射待确认
};
static const int g_video_mr_zones[] = {
    Z_IP_H4, Z_IP_H5, Z_IP_H6,  // TODO: 实际映射待确认
};
static const int g_video_rl_zones[] = {
    Z_LR_FOOT, Z_LR_MAP, Z_LR_UP_A, Z_LR_UP_B,
};
static const int g_video_rr_zones[] = {
    Z_RR_FOOT, Z_RR_MAP, Z_RR_UP_A, Z_RR_UP_B,
};

#define VA_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

// processor 调用: 根据当前 area flag 计算活跃 zone mask
ZoneMask video_ambient_active_mask(void) {
    ZoneMask m; zm_clear(&m);
    if (g_rx_frame.canfd_0x2cf.VIU_AL_VideoAmbLiAreaFL)
        for (int i = 0; i < (int)VA_LEN(g_video_fl_zones); i++) zm_set(&m, g_video_fl_zones[i]);
    if (g_rx_frame.canfd_0x2cf.VIU_AL_VideoAmbLiAreaFR)
        for (int i = 0; i < (int)VA_LEN(g_video_fr_zones); i++) zm_set(&m, g_video_fr_zones[i]);
    if (g_rx_frame.canfd_0x2cf.VIU_AL_VideoAmbLiAreaML)
        for (int i = 0; i < (int)VA_LEN(g_video_ml_zones); i++) zm_set(&m, g_video_ml_zones[i]);
    if (g_rx_frame.canfd_0x2cf.VIU_AL_VideoAmbLiAreaMR)
        for (int i = 0; i < (int)VA_LEN(g_video_mr_zones); i++) zm_set(&m, g_video_mr_zones[i]);
    if (g_rx_frame.canfd_0x2cf.VIU_AL_VideoAmbLiAreaRL)
        for (int i = 0; i < (int)VA_LEN(g_video_rl_zones); i++) zm_set(&m, g_video_rl_zones[i]);
    if (g_rx_frame.canfd_0x2cf.VIU_AL_VideoAmbLiAreaRR)
        for (int i = 0; i < (int)VA_LEN(g_video_rr_zones); i++) zm_set(&m, g_video_rr_zones[i]);
    return m;
}

static void va_fill_zones(const int *zones, int count,
                          uint8_t r, uint8_t g, uint8_t b,
                          int allowed, LedOutput *out) {
    if (!allowed || !out) return;
    for (int i = 0; i < count; i++)
        zone_fill(out, zones[i], r, g, b, 50, 0);
}

static void va_init(Effect *e, TimeMs now) {
    e->start_ts = now;
    e->state    = S_ACTIVE;
}

static void va_update(Effect *e, TimeMs now, int allowed, LedOutput *out) {
    if (!allowed || !out) return;
    (void)e; (void)now;

    uint8_t r = g_rx_frame.canfd_0x2cf.VIU_AL_VideoAmbLiColorSet_Red;
    uint8_t g = g_rx_frame.canfd_0x2cf.VIU_AL_VideoAmbLiColorSet_Green;
    uint8_t b = g_rx_frame.canfd_0x2cf.VIU_AL_VideoAmbLiColorSet_Blue;

    if (g_rx_frame.canfd_0x2cf.VIU_AL_VideoAmbLiAreaFL)
        va_fill_zones(g_video_fl_zones, VA_LEN(g_video_fl_zones), r, g, b, allowed, out);
    if (g_rx_frame.canfd_0x2cf.VIU_AL_VideoAmbLiAreaFR)
        va_fill_zones(g_video_fr_zones, VA_LEN(g_video_fr_zones), r, g, b, allowed, out);
    if (g_rx_frame.canfd_0x2cf.VIU_AL_VideoAmbLiAreaML)
        va_fill_zones(g_video_ml_zones, VA_LEN(g_video_ml_zones), r, g, b, allowed, out);
    if (g_rx_frame.canfd_0x2cf.VIU_AL_VideoAmbLiAreaMR)
        va_fill_zones(g_video_mr_zones, VA_LEN(g_video_mr_zones), r, g, b, allowed, out);
    if (g_rx_frame.canfd_0x2cf.VIU_AL_VideoAmbLiAreaRL)
        va_fill_zones(g_video_rl_zones, VA_LEN(g_video_rl_zones), r, g, b, allowed, out);
    if (g_rx_frame.canfd_0x2cf.VIU_AL_VideoAmbLiAreaRR)
        va_fill_zones(g_video_rr_zones, VA_LEN(g_video_rr_zones), r, g, b, allowed, out);
}

static void va_deinit(Effect *e) { (void)e; }

// 工厂: mask 初始为空，运行时由 commands_tick -> em_set_mask 动态设置
Effect *video_ambient_factory(void) {
    ZoneMask mask; zm_clear(&mask);
    Effect *e = effect_create_base("video_ambient", 3, mask);
    if (!e) return NULL;
    e->init   = va_init;
    e->update = va_update;
    e->deinit = va_deinit;
    return e;
}
