#include <stdlib.h>
#include <string.h>
#include "running_light.h"
#include "led_map.h"

// ============================================================================
// running_light — 仪表流水灯效（共享 update，两个工厂）
// ============================================================================
// rl1: 仪表段1+段2, pri=5, cyan
// rl2: 仪表段2+段3, pri=6, orange
// 两者在 Z_IP_FLOW_2 上重叠 → rl2 优先级更高，仲裁时自动抢走段2
//
// zone 仲裁: rl2 优先级高，自动抢走 Z_IP_FLOW_2

#define TAIL_LEN 20

typedef struct { uint8_t r, g, b; } RLColor;

static void rl_init(Effect *e, TimeMs now) {
    e->start_ts = now;
    e->state    = S_ACTIVE;
}

static void rl_update(Effect *e, TimeMs now, int allowed, LedOutput *out) {
    if (!allowed || !out) return;

    RLColor *c = (RLColor *)e->params;
    uint32_t t = now - e->start_ts;

    for (int z = 0; z < ZONE_COUNT; z++) {
        if (!zm_test(&e->mask, z)) continue;  // ← 只看仲裁后实际拿到的 zone

        const ZoneLayout *zl = &g_zone_layout[z];
        int  n    = zl->pixel_count;
        int  head = (t / 15) % n;

        for (int i = 0; i < n; i++) {
            int dist = head - i;
            if (dist < 0) dist += n;
            if (dist > n / 2) dist = n - dist;
            if (dist > TAIL_LEN) dist = TAIL_LEN;

            float  fade = 1.0f - (float)dist / (float)TAIL_LEN;
            uint8_t br  = (uint8_t)(fade * fade * 200.0f);

            LedPixel *p = &out->pixels[zl->offset + i];
            p->r          = c->r;
            p->g          = c->g;
            p->b          = c->b;
            p->l = br; p->f = 0;
        }
    }
}

static void rl_deinit(Effect *e) { (void)e; }

// ---- 工厂 ---------------------------------------------------------------

static RLColor color_cyan   = { 0,   200, 255 };
static RLColor color_orange = { 255, 100, 0   };

Effect *running_light_1_factory(void) {
    ZoneMask mask; zm_clear(&mask);
    zm_set(&mask, Z_IP_FLOW_1);
    zm_set(&mask, Z_IP_FLOW_2);

    Effect *e = effect_create_base("running_light_1", 5, mask);
    if (!e) return NULL;
    e->params = &color_cyan;
    e->init   = rl_init;
    e->update = rl_update;
    e->deinit = rl_deinit;
    return e;
}

Effect *running_light_2_factory(void) {
    ZoneMask mask; zm_clear(&mask);
    zm_set(&mask, Z_IP_FLOW_2);
    zm_set(&mask, Z_IP_FLOW_3);

    Effect *e = effect_create_base("running_light_2", 6, mask);
    if (!e) return NULL;
    e->params = &color_orange;
    e->init   = rl_init;
    e->update = rl_update;
    e->deinit = rl_deinit;
    return e;
}
