#include <stdlib.h>
#include <string.h>
#include "running_light.h"
#include "led_map.h"

// ============================================================================
// running_light — 流水灯效（共享 update，两个工厂）
// ============================================================================
// rl1: 车顶前段+中段, pri=5, cyan
// rl2: 车顶中段+后段, pri=6, orange
// 两者在 Z_ROOF_MID 上重叠 → rl2 优先级更高，仲裁时自动抢走中段
//
// 这是演示 zone 级仲裁的标准案例：
//   - 效果通过 e->mask 声明自己"想要"哪些 zone
//   - EffectManager 根据优先级分配：高优先级拿走重叠 zone
//   - 低优先级效果在 update() 里通过 zm_test(&e->mask, z) 看到已被裁剪的 mask
//   - 效果只需要遍历 e->mask，不用关心谁抢走了哪个 zone

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

    for (int z = 0; z < MAX_ZONES; z++) {
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
            p->brightness = br;
        }
    }
}

static void rl_deinit(Effect *e) { (void)e; }

// ---- 工厂 ---------------------------------------------------------------

static RLColor color_cyan   = { 0,   200, 255 };
static RLColor color_orange = { 255, 100, 0   };

Effect *running_light_1_factory(void) {
    ZoneMask mask; zm_clear(&mask);
    zm_set(&mask, Z_ROOF_FRONT);
    zm_set(&mask, Z_ROOF_MID);

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
    zm_set(&mask, Z_ROOF_MID);
    zm_set(&mask, Z_ROOF_REAR);

    Effect *e = effect_create_base("running_light_2", 6, mask);
    if (!e) return NULL;
    e->params = &color_orange;
    e->init   = rl_init;
    e->update = rl_update;
    e->deinit = rl_deinit;
    return e;
}
