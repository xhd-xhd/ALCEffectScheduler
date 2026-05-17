#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "welcome.h"
#include "led_map.h"

// ============================================================================
// welcome — 开门迎宾灯效 (优先级 7)
// ============================================================================
// 管辖 zone: 四个车门（zone 0, 1, 2, 3）
// 效果: 蓝色呼吸渐变，800ms 淡入后持续呼吸

static void welcome_init(Effect *e, TimeMs now) {
    e->start_ts = now;
    e->state    = S_ACTIVE;
}

static void welcome_update(Effect *e, TimeMs now, int allowed, LedOutput *out) {
    if (!allowed || !out) return;
    uint32_t elapsed = now - e->start_ts;
    float    t       = (elapsed < 800) ? (float)elapsed / 800.0f : 1.0f;
    float    breath  = (sinf((float)elapsed / 600.0f * 3.14159265f) + 1.0f) / 2.0f;
    uint8_t  bright  = (uint8_t)(breath * 200.0f * t + 30.0f);

    for (int i = 0; i < MAX_ZONES; i++) {
        if (zm_test(&e->mask, i))
            zone_fill(out, i, 0, 100, 255, bright);
    }
}

static void welcome_deinit(Effect *e) { (void)e; }

Effect *welcome_factory(void) {
    ZoneMask mask = zm_group_all_doors();

    Effect *e = effect_create_base("welcome", 7, mask);
    if (!e) return NULL;
    e->init   = welcome_init;
    e->update = welcome_update;
    e->deinit = welcome_deinit;
    return e;
}
