#include "theme_music.h"
#include "led_map.h"
#include "can_frames.h"

// ============================================================================
// theme_music — AtmThemeMod=6，音乐随动 (优先级 3)
// ============================================================================
// 管辖 zone: 全部
// 信号来源:
//   canfd_0x1e9~0x1ed  → 19 个 PreDishes 区域
//   canfd_0x1ee         → 7 段高音扬声器散光灯

// ---- area → zone 映射表 ---------------------------------------------------
// TODO: 由硬件布局确定每个 PreDishesArea 编号对应的 zone
static const int g_area_to_zone[32] = {
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1,
};
// tweeter HL 段 → zone 映射
static const int g_hl_to_zone[6] = {
    Z_IP_H1, Z_IP_H2, Z_IP_H3, Z_IP_H4, Z_IP_H5, Z_IP_H6,
};

// ---- 辅助 ----------------------------------------------------------------
static void fill_z(int z, uint8_t r, uint8_t g, uint8_t b,
                   uint8_t bri, int allowed, LedOutput *out) {
    if (z < 0 || z >= ZONE_COUNT || !allowed || !out) return;
    zone_fill(out, z, r, g, b, bri, 0);
}

#define FILL_AREA(area_idx) do {               \
    int z = g_area_to_zone[a->VIU_AL_PreDishesArea##area_idx]; \
    fill_z(z,                                   \
        a->VIU_AL_ColorSet_R##area_idx,          \
        a->VIU_AL_ColorSet_G##area_idx,          \
        a->VIU_AL_ColorSet_B##area_idx,          \
        a->VIU_AL_BriSet##area_idx,              \
        allowed, out);                           \
} while(0)

#define FILL_HL(idx) do {                      \
    int z = g_hl_to_zone[idx];                  \
    fill_z(z,                                   \
        h->VIU_AL_HLColorSet_R##idx,             \
        h->VIU_AL_HLColorSet_G##idx,             \
        h->VIU_AL_HLColorSet_B##idx,             \
        h->VIU_AL_HLBriSet_##idx,                \
        allowed, out);                           \
} while(0)

// ---- update --------------------------------------------------------------
static void tm_init(Effect *e, TimeMs now) {
    e->start_ts = now;
    e->state    = S_ACTIVE;
}

static void tm_update(Effect *e, TimeMs now, int allowed, LedOutput *out) {
    if (!allowed || !out) return;
    (void)e; (void)now;

    // 0x1E9: 区域 1~4
    { const typeof(g_rx_frame.canfd_0x1e9) *a = &g_rx_frame.canfd_0x1e9;
      FILL_AREA(1); FILL_AREA(2); FILL_AREA(3); FILL_AREA(4); }

    // 0x1EA: 区域 5~8
    { const typeof(g_rx_frame.canfd_0x1ea) *a = &g_rx_frame.canfd_0x1ea;
      FILL_AREA(5); FILL_AREA(6); FILL_AREA(7); FILL_AREA(8); }

    // 0x1EB: 区域 9~12
    { const typeof(g_rx_frame.canfd_0x1eb) *a = &g_rx_frame.canfd_0x1eb;
      FILL_AREA(9); FILL_AREA(10); FILL_AREA(11); FILL_AREA(12); }

    // 0x1EC: 区域 13~16
    { const typeof(g_rx_frame.canfd_0x1ec) *a = &g_rx_frame.canfd_0x1ec;
      FILL_AREA(13); FILL_AREA(14); FILL_AREA(15); FILL_AREA(16); }

    // 0x1ED: 区域 17~19
    { const typeof(g_rx_frame.canfd_0x1ed) *a = &g_rx_frame.canfd_0x1ed;
      FILL_AREA(17); FILL_AREA(18); FILL_AREA(19); }

    // 0x1EE: 高音扬声器散光 6 段
    { const typeof(g_rx_frame.canfd_0x1ee) *h = &g_rx_frame.canfd_0x1ee;
      FILL_HL(1); FILL_HL(2); FILL_HL(3); FILL_HL(4);
      FILL_HL(5); FILL_HL(6); }
}

static void tm_deinit(Effect *e) { (void)e; }

Effect *theme_music_factory(void) {
    ZoneMask mask = zm_group_all_zones();
    Effect *e = effect_create_base("theme_music", 3, mask);
    if (!e) return NULL;
    e->init   = tm_init;
    e->update = tm_update;
    e->deinit = tm_deinit;
    return e;
}
