#include "tweeter_off.h"
#include "led_map.h"

// ============================================================================
// tweeter_off — 施罗德使能关闭时熄灭 6 颗 IP_H LED (优先级 11, 最高)
// ============================================================================
// 管辖 zone: Z_IP_H1~H6
// TwetterLampEnable != 1 → 激活，填 0，任何效果都抢不走这 6 颗。
// TwetterLampEnable == 1   → 停用，正常显示。

static void tw_init(Effect *e, TimeMs now) {
    e->start_ts = now;
    e->state    = S_ACTIVE;
}

static void tw_update(Effect *e, TimeMs now, int allowed, LedOutput *out) {
    if (!allowed || !out) return;
    (void)now;
    if (zm_test(&e->mask, Z_IP_H1)) zone_fill(out, Z_IP_H1, 0, 0, 0, 0, 0);
    if (zm_test(&e->mask, Z_IP_H2)) zone_fill(out, Z_IP_H2, 0, 0, 0, 0, 0);
    if (zm_test(&e->mask, Z_IP_H3)) zone_fill(out, Z_IP_H3, 0, 0, 0, 0, 0);
    if (zm_test(&e->mask, Z_IP_H4)) zone_fill(out, Z_IP_H4, 0, 0, 0, 0, 0);
    if (zm_test(&e->mask, Z_IP_H5)) zone_fill(out, Z_IP_H5, 0, 0, 0, 0, 0);
    if (zm_test(&e->mask, Z_IP_H6)) zone_fill(out, Z_IP_H6, 0, 0, 0, 0, 0);
}

static void tw_deinit(Effect *e) { (void)e; }

Effect *tweeter_off_factory(void) {
    ZoneMask mask; zm_clear(&mask);
    zm_set(&mask, Z_IP_H1); zm_set(&mask, Z_IP_H2);
    zm_set(&mask, Z_IP_H3); zm_set(&mask, Z_IP_H4);
    zm_set(&mask, Z_IP_H5); zm_set(&mask, Z_IP_H6);

    Effect *e = effect_create_base("tweeter_off", 11, mask);
    if (!e) return NULL;
    e->init   = tw_init;
    e->update = tw_update;
    e->deinit = tw_deinit;
    return e;
}
