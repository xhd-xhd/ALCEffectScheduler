#include <stdlib.h>
#include <string.h>
#include "ambient.h"
#include "led_map.h"

// ============================================================================
// ambient — 背景氛围灯 (优先级 1，最低)
// ============================================================================
// 管辖 zone: 全部 9 个 zone
// 效果: 暖白色低亮度常亮，任何其他灯效激活时被影子化

static void ambient_init(Effect *e, TimeMs now) {
    e->start_ts = now;
    e->state    = S_ACTIVE;
}

static void ambient_update(Effect *e, TimeMs now, int allowed, LedOutput *out) {
    (void)now;
    if (!allowed || !out) return;
    for (int i = 0; i < MAX_ZONES; i++) {
        if (zm_test(&e->mask, i))
            zone_fill(out, i, 255, 180, 60, 40);
    }
}

static void ambient_deinit(Effect *e) { (void)e; }

Effect *ambient_factory(void) {
    ZoneMask mask = zm_group_all_zones();

    Effect *e = effect_create_base("ambient", 1, mask);
    if (!e) return NULL;
    e->init   = ambient_init;
    e->update = ambient_update;
    e->deinit = ambient_deinit;
    return e;
}
