#include <math.h>
#include "footwell_pulse.h"
#include "led_map.h"

// ============================================================================
// footwell_pulse — 脚窝灯呼吸效果 (优先级 3)
// ============================================================================
// 演示"点名式"写法: 不遍历所有 zone，直接用枚举值指定目标 zone，
// 设之前顺手用 zm_test 查一下有没有被更高优先级效果抢走。
// 管辖 zone: 所有照脚灯 (LAYER_FOOT)
// 效果: 暖琥珀色慢呼吸 (2.5s 周期)

static void fp_init(Effect *e, TimeMs now) {
    e->start_ts = now;
    e->state    = S_ACTIVE;
}

static void fp_update(Effect *e, TimeMs now, int allowed, LedOutput *out) {
    if (!allowed || !out) return;

    float   t     = (float)(now - e->start_ts) / 1000.0f;
    float   pulse = (sinf(t * 2.512f) + 1.0f) / 2.0f;
    uint8_t br    = (uint8_t)(pulse * 180.0f + 20.0f);

    // 点名式: 明确知道要操作哪些 zone，逐个查 mask
    if (zm_test(&e->mask, Z_LF_FOOT))
        zone_fill(out, Z_LF_FOOT,  255, 140, 0, br, 0);

    if (zm_test(&e->mask, Z_LR_FOOT))
        zone_fill(out, Z_LR_FOOT, 255, 140, 0, br, 0);
    if (zm_test(&e->mask, Z_RF_FOOT))
        zone_fill(out, Z_RF_FOOT, 255, 140, 0, br, 0);
    if (zm_test(&e->mask, Z_RR_FOOT))
        zone_fill(out, Z_RR_FOOT, 255, 140, 0, br, 0);
}

static void fp_deinit(Effect *e) { (void)e; }

Effect *footwell_pulse_factory(void) {
    ZoneMask mask = zm_group_layer(LAYER_FOOT);

    Effect *e = effect_create_base("footwell_pulse", 3, mask);
    if (!e) return NULL;
    e->init   = fp_init;
    e->update = fp_update;
    e->deinit = fp_deinit;
    return e;
}
