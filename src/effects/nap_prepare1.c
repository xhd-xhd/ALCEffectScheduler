#include "nap_prepare1.h"
#include "led_map.h"

// ============================================================================
// nap_prepare1 — 小憩模式 Preparing1 (优先级 4)
// ============================================================================
// mask: 全 zone 静态. TODO: 具体灯效

static void np1_init(Effect *e, TimeMs now) { e->start_ts = now; e->state = S_ACTIVE; }
static void np1_update(Effect *e, TimeMs now, int allowed, LedOutput *out) { (void)e; (void)now; (void)allowed; (void)out; }
static void np1_deinit(Effect *e) { (void)e; }

Effect *nap_prepare1_factory(void) {
    Effect *e = effect_create_base("nap_prepare1", 4, zm_group_all_zones());
    if (!e) return NULL;
    e->init = np1_init; e->update = np1_update; e->deinit = np1_deinit;
    return e;
}
