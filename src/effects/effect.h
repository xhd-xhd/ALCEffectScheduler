#ifndef EFFECT_H
#define EFFECT_H
#include "types.h"
#include "led_map.h"

// ============================================================================
// Effect — 灯效对象基类
// ============================================================================
// 灯效在注册时创建，按优先级排序，之后持续存在直到 shutdown。
// 运行时只在 ACTIVE / SHADOWED / IDLE 之间切换，不增删。
//
// 开发新灯效只需:
//   1. 实现 init / update / deinit 三个方法
//   2. 写一个 xxx_factory()，在里面:
//        effect_create_base("名字", 优先级, 管辖的zone mask)
//        挂上 init / update / deinit
//   3. 在 main.c 里 em_register_factory("名字", xxx_factory)

// 效果一旦注册就一直存在，只需 IDLE / ACTIVE / SHADOWED 三种状态
typedef enum {
    S_IDLE,
    S_ACTIVE,
    S_SHADOWED   // 被高优先级效果遮挡，update 仍被调用但 allowed=0
} EffectState;

typedef struct Effect {
    const char *name;
    int         priority;
    ZoneMask    mask;       // 管辖哪些 zone，在工厂里设定后一般不动
    void       *params;
    TimeMs      start_ts;
    EffectState state;

    void (*init)  (struct Effect*, TimeMs now);
    void (*update)(struct Effect*, TimeMs now, int allowed, LedOutput *out);
    void (*deinit)(struct Effect*);
} Effect;

// mask 参数: 这个效果管哪些 zone，在工厂里设定
Effect *effect_create_base(const char *name, int priority, ZoneMask mask);
void    effect_destroy(Effect *e);   // 仅 shutdown 时用，归还对象池
#endif
