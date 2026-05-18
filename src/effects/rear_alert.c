#include <stdlib.h>
#include <string.h>
#include "rear_alert.h"
#include "led_map.h"

// ============================================================================
// rear_alert — 后雷达告警 (优先级 10，最高)
// ============================================================================
// 管辖 zone: 左后门 + 右后门全部（8 个点光源）
// 效果: 红色闪烁，500ms 亮 / 500ms 灭

static void rear_init(Effect *e, TimeMs now) {
    e->start_ts = now;
    e->state    = S_ACTIVE;
}

static void rear_update(Effect *e, TimeMs now, int allowed, LedOutput *out) {
    if (!allowed || !out) return;
    int     phase = (int)((now - e->start_ts) / 500) % 2;
    uint8_t br    = phase ? 255 : 0;

    for (int i = 0; i < ZONE_COUNT; i++) {
        if (zm_test(&e->mask, i))
            zone_fill(out, i, 255, 0, 0, br, 0);
    }
}

static void rear_deinit(Effect *e) { (void)e; }

Effect *rear_alert_factory(void) {
    ZoneMask mask = zm_group_door_lr();
    ZoneMask t    = zm_group_door_rr();
    zm_or(&mask, &t);

    Effect *e = effect_create_base("rear_alert", 10, mask);
    if (!e) return NULL;
    e->init   = rear_init;
    e->update = rear_update;
    e->deinit = rear_deinit;
    return e;
}
