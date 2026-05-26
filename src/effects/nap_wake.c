#include "nap_wake.h"
#include "led_map.h"

// ============================================================================
// nap_wake — 小憩模式 WakingUp (优先级 4)
// ============================================================================
// TODO: 具体灯效

static void nw_init(Effect *e, TimeMs now) { e->start_ts = now; e->state = S_ACTIVE; }
static void nw_update(Effect *e, TimeMs now, int allowed, LedOutput *out) { (void)e; (void)now; (void)allowed; (void)out; }
static void nw_deinit(Effect *e) { (void)e; }

Effect *nap_wake_factory(void) {
    Effect *e = effect_create_base("nap_wake", 4, zm_group_all_zones());
    if (!e) return NULL;
    e->init = nw_init; e->update = nw_update; e->deinit = nw_deinit;
    return e;
}
