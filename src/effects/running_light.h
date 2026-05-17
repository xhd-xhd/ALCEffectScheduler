#ifndef RUNNING_LIGHT_H
#define RUNNING_LIGHT_H
#include "effect.h"

// rl1: 车顶前段+中段 (pri=5, cyan)
// rl2: 车顶中段+后段 (pri=6, orange)
// 两者在 Z_ROOF_MID 上重叠 — rl2 优先级更高，抢走中段
Effect *running_light_1_factory(void);
Effect *running_light_2_factory(void);
#endif
