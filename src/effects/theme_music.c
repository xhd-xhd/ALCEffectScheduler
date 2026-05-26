#include "theme_music.h"
#include "led_map.h"
#include "can_frames.h"

// ============================================================================
// theme_music — AtmThemeMod=6，音乐随动 (优先级 3)
// ============================================================================
// 信号: canfd_0x1e9~0x1ed (19区) + canfd_0x1ee (6段施罗德HL)
// 每个 PreDishesArea 对应一组 zone，用 -1 哨兵结尾

// ---- area → zone 列表 (TODO: 实际映射) -----------------------------------
// 例: area1=左前门高位灯 (2 个 zone)
static const int area1_zones[]  = { Z_LF_UP_A, Z_LF_UP_B, -1 };
// 例: area2=左前门地图袋+照脚 (2 个 zone)
static const int area2_zones[]  = { Z_LF_MAP, Z_LF_FOOT, -1 };
// 例: area3=四个门全部照脚灯 (4 个 zone)
static const int area3_zones[]  = { Z_LF_FOOT, Z_LR_FOOT, Z_RF_FOOT, Z_RR_FOOT, -1 };
static const int area4_zones[]  = { -1 };
static const int area5_zones[]  = { -1 };
static const int area6_zones[]  = { -1 };
static const int area7_zones[]  = { -1 };
static const int area8_zones[]  = { -1 };
static const int area9_zones[]  = { -1 };
static const int area10_zones[] = { -1 };
static const int area11_zones[] = { -1 };
static const int area12_zones[] = { -1 };
static const int area13_zones[] = { -1 };
static const int area14_zones[] = { -1 };
static const int area15_zones[] = { -1 };
static const int area16_zones[] = { -1 };
static const int area17_zones[] = { -1 };
static const int area18_zones[] = { -1 };
static const int area19_zones[] = { -1 };

// PreDishesArea 值 → zone 列表指针
static const int *g_area_zones[32] = {
    [1]  = area1_zones,  [2]  = area2_zones,  [3]  = area3_zones,
    [4]  = area4_zones,  [5]  = area5_zones,  [6]  = area6_zones,
    [7]  = area7_zones,  [8]  = area8_zones,  [9]  = area9_zones,
    [10] = area10_zones, [11] = area11_zones, [12] = area12_zones,
    [13] = area13_zones, [14] = area14_zones, [15] = area15_zones,
    [16] = area16_zones, [17] = area17_zones, [18] = area18_zones,
    [19] = area19_zones,
};

// 施罗德 HL 6 段 — 每段 1 个 zone
static const int hl1_zones[] = { Z_IP_H1, -1 };
static const int hl2_zones[] = { Z_IP_H2, -1 };
static const int hl3_zones[] = { Z_IP_H3, -1 };
static const int hl4_zones[] = { Z_IP_H4, -1 };
static const int hl5_zones[] = { Z_IP_H5, -1 };
static const int hl6_zones[] = { Z_IP_H6, -1 };
static const int *g_hl_zones[6] = {
    hl1_zones, hl2_zones, hl3_zones, hl4_zones, hl5_zones, hl6_zones,
};

// ---- processor 调用: 根据 PreDishesMod==2 计算活跃 zone mask --------------
static void mask_add_list(ZoneMask *m, const int *zones) {
    if (!zones) return;
    for (int i = 0; zones[i] >= 0; i++) zm_set(m, zones[i]);
}

// 查某个区域 (1~19) 的 Mod 值
static uint8_t get_mod(int area_idx) {
    switch (area_idx) {
    case 1:  return g_rx_frame.canfd_0x1e9.VIU_AL_PreDishesMod1;
    case 2:  return g_rx_frame.canfd_0x1e9.VIU_AL_PreDishesMod2;
    case 3:  return g_rx_frame.canfd_0x1e9.VIU_AL_PreDishesMod3;
    case 4:  return g_rx_frame.canfd_0x1e9.VIU_AL_PreDishesMod4;
    case 5:  return g_rx_frame.canfd_0x1ea.VIU_AL_PreDishesMod5;
    case 6:  return g_rx_frame.canfd_0x1ea.VIU_AL_PreDishesMod6;
    case 7:  return g_rx_frame.canfd_0x1ea.VIU_AL_PreDishesMod7;
    case 8:  return g_rx_frame.canfd_0x1ea.VIU_AL_PreDishesMod8;
    case 9:  return g_rx_frame.canfd_0x1eb.VIU_AL_PreDishesMod9;
    case 10: return g_rx_frame.canfd_0x1eb.VIU_AL_PreDishesMod10;
    case 11: return g_rx_frame.canfd_0x1eb.VIU_AL_PreDishesMod11;
    case 12: return g_rx_frame.canfd_0x1eb.VIU_AL_PreDishesMod12;
    case 13: return g_rx_frame.canfd_0x1ec.VIU_AL_PreDishesMod13;
    case 14: return g_rx_frame.canfd_0x1ec.VIU_AL_PreDishesMod14;
    case 15: return g_rx_frame.canfd_0x1ec.VIU_AL_PreDishesMod15;
    case 16: return g_rx_frame.canfd_0x1ec.VIU_AL_PreDishesMod16;
    case 17: return g_rx_frame.canfd_0x1ed.VIU_AL_PreDishesMod17;
    case 18: return g_rx_frame.canfd_0x1ed.VIU_AL_PreDishesMod18;
    case 19: return g_rx_frame.canfd_0x1ed.VIU_AL_PreDishesMod19;
    default: return 0;
    }
}

ZoneMask theme_music_active_mask(void) {
    ZoneMask m; zm_clear(&m);

    // 19 个区域: Mod==2 → 加入 mask
    for (int a = 1; a <= 19; a++) {
        if (get_mod(a) == 2)
            mask_add_list(&m, g_area_zones[a]);
    }

    // HL: HLPreDishesMod==2 → 全部加入
    if (g_rx_frame.canfd_0x1ee.VIU_AL_HLPreDishesMod == 2) {
        for (int i = 0; i < 6; i++)
            mask_add_list(&m, g_hl_zones[i]);
    }

    return m;
}

// ---- 哨兵遍历填 zone -----------------------------------------------------
static void fill_list(const int *zones,
                      uint8_t r, uint8_t g, uint8_t b, uint8_t bri,
                      int allowed, LedOutput *out) {
    if (!allowed || !out || !zones) return;
    for (int i = 0; zones[i] >= 0; i++)
        zone_fill(out, zones[i], r, g, b, bri, 0);
}

// ---- FILL_AREA 宏: 从 canfd_0x1e9~0x1ee 命名字段取值 -------------------
#define FILL_AREA(area_idx) do {                                    \
    const int *z = g_area_zones[a->VIU_AL_PreDishesArea##area_idx]; \
    fill_list(z,                                                     \
        a->VIU_AL_ColorSet_R##area_idx,                              \
        a->VIU_AL_ColorSet_G##area_idx,                              \
        a->VIU_AL_ColorSet_B##area_idx,                              \
        a->VIU_AL_BriSet##area_idx, allowed, out);                   \
} while(0)

// ---- update --------------------------------------------------------------
static void tm_init(Effect *e, TimeMs now) {
    e->start_ts = now;
    e->state    = S_ACTIVE;
}

static void tm_update(Effect *e, TimeMs now, int allowed, LedOutput *out) {
    if (!allowed || !out) return;
    (void)e; (void)now;
    const CanFrame *f = &g_rx_frame;

    // 只填 PreDishesMod==2 的区域
    { const typeof(f->canfd_0x1e9) *a = &f->canfd_0x1e9;
      if (a->VIU_AL_PreDishesMod1 == 2) FILL_AREA(1);
      if (a->VIU_AL_PreDishesMod2 == 2) FILL_AREA(2);
      if (a->VIU_AL_PreDishesMod3 == 2) FILL_AREA(3);
      if (a->VIU_AL_PreDishesMod4 == 2) FILL_AREA(4); }
    { const typeof(f->canfd_0x1ea) *a = &f->canfd_0x1ea;
      if (a->VIU_AL_PreDishesMod5  == 2) FILL_AREA(5);
      if (a->VIU_AL_PreDishesMod6  == 2) FILL_AREA(6);
      if (a->VIU_AL_PreDishesMod7  == 2) FILL_AREA(7);
      if (a->VIU_AL_PreDishesMod8  == 2) FILL_AREA(8); }
    { const typeof(f->canfd_0x1eb) *a = &f->canfd_0x1eb;
      if (a->VIU_AL_PreDishesMod9  == 2) FILL_AREA(9);
      if (a->VIU_AL_PreDishesMod10 == 2) FILL_AREA(10);
      if (a->VIU_AL_PreDishesMod11 == 2) FILL_AREA(11);
      if (a->VIU_AL_PreDishesMod12 == 2) FILL_AREA(12); }
    { const typeof(f->canfd_0x1ec) *a = &f->canfd_0x1ec;
      if (a->VIU_AL_PreDishesMod13 == 2) FILL_AREA(13);
      if (a->VIU_AL_PreDishesMod14 == 2) FILL_AREA(14);
      if (a->VIU_AL_PreDishesMod15 == 2) FILL_AREA(15);
      if (a->VIU_AL_PreDishesMod16 == 2) FILL_AREA(16); }
    { const typeof(f->canfd_0x1ed) *a = &f->canfd_0x1ed;
      if (a->VIU_AL_PreDishesMod17 == 2) FILL_AREA(17);
      if (a->VIU_AL_PreDishesMod18 == 2) FILL_AREA(18);
      if (a->VIU_AL_PreDishesMod19 == 2) FILL_AREA(19); }

    // HL: HLPreDishesMod==2 全亮
    if (f->canfd_0x1ee.VIU_AL_HLPreDishesMod == 2) {
      const typeof(f->canfd_0x1ee) *h = &f->canfd_0x1ee;
      fill_list(g_hl_zones[0], h->VIU_AL_HLColorSet_R1, h->VIU_AL_HLColorSet_G1,
                h->VIU_AL_HLColorSet_B1, h->VIU_AL_HLBriSet_1, allowed, out);
      fill_list(g_hl_zones[1], h->VIU_AL_HLColorSet_R2, h->VIU_AL_HLColorSet_G2,
                h->VIU_AL_HLColorSet_B2, h->VIU_AL_HLBriSet_2, allowed, out);
      fill_list(g_hl_zones[2], h->VIU_AL_HLColorSet_R3, h->VIU_AL_HLColorSet_G3,
                h->VIU_AL_HLColorSet_B3, h->VIU_AL_HLBriSet_3, allowed, out);
      fill_list(g_hl_zones[3], h->VIU_AL_HLColorSet_R4, h->VIU_AL_HLColorSet_G4,
                h->VIU_AL_HLColorSet_B4, h->VIU_AL_HLBriSet_4, allowed, out);
      fill_list(g_hl_zones[4], h->VIU_AL_HLColorSet_R5, h->VIU_AL_HLColorSet_G5,
                h->VIU_AL_HLColorSet_B5, h->VIU_AL_HLBriSet_5, allowed, out);
      fill_list(g_hl_zones[5], h->VIU_AL_HLColorSet_R6, h->VIU_AL_HLColorSet_G6,
                h->VIU_AL_HLColorSet_B6, h->VIU_AL_HLBriSet_6, allowed, out);
    }
}

static void tm_deinit(Effect *e) { (void)e; }

Effect *theme_music_factory(void) {
    ZoneMask mask; zm_clear(&mask);  // 运行时由 commands_tick 动态设置
    Effect *e = effect_create_base("theme_music", 3, mask);
    if (!e) return NULL;
    e->init   = tm_init;
    e->update = tm_update;
    e->deinit = tm_deinit;
    return e;
}
