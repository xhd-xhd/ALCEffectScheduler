#include "theme_off.h"
#include "led_map.h"

// ============================================================================
// theme_off — AtmThemeMod=0，全局关灯 (优先级 2)
// ============================================================================
// 管辖 zone: 全部
// 效果: 所有 zone 填 0，覆盖 ambient。高优先级效果(rear_alert 等)仍可通过仲裁穿透。

static void to_init(Effect *e, TimeMs now) {
    e->start_ts = now;
    e->state    = S_ACTIVE;
}

static void to_update(Effect *e, TimeMs now, int allowed, LedOutput *out) {
    if (!allowed || !out) return;
    for (int z = 0; z < ZONE_COUNT; z++) {
        if (zm_test(&e->mask, z))
            zone_fill(out, z, 0, 0, 0, 0, 0);
    }
}

static void to_deinit(Effect *e) { (void)e; }

Effect *theme_off_factory(void) {
    ZoneMask mask = zm_group_all_zones();
    Effect *e = effect_create_base("theme_off", 2, mask);
    if (!e) return NULL;
    e->init   = to_init;
    e->update = to_update;
    e->deinit = to_deinit;
    return e;
}
