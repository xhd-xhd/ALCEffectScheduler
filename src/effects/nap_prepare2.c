#include "nap_prepare2.h"
#include "led_map.h"

// ============================================================================
// nap_prepare2 — 小憩模式 Preparing2 (优先级 4)
// ============================================================================
// TODO: 具体灯效

static void np2_init(Effect *e, TimeMs now) { e->start_ts = now; e->state = S_ACTIVE; }
static void np2_update(Effect *e, TimeMs now, int allowed, LedOutput *out) { (void)e; (void)now; (void)allowed; (void)out; }
static void np2_deinit(Effect *e) { (void)e; }

Effect *nap_prepare2_factory(void) {
    Effect *e = effect_create_base("nap_prepare2", 4, zm_group_all_zones());
    if (!e) return NULL;
    e->init = np2_init; e->update = np2_update; e->deinit = np2_deinit;
    return e;
}
