#include "nap_sleep.h"
#include "led_map.h"

// ============================================================================
// nap_sleep — 小憩模式 Sleeping (优先级 4)
// ============================================================================
// TODO: 具体灯效

static void ns_init(Effect *e, TimeMs now) { e->start_ts = now; e->state = S_ACTIVE; }
static void ns_update(Effect *e, TimeMs now, int allowed, LedOutput *out) { (void)e; (void)now; (void)allowed; (void)out; }
static void ns_deinit(Effect *e) { (void)e; }

Effect *nap_sleep_factory(void) {
    Effect *e = effect_create_base("nap_sleep", 4, zm_group_all_zones());
    if (!e) return NULL;
    e->init = ns_init; e->update = ns_update; e->deinit = ns_deinit;
    return e;
}
