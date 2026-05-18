#ifndef THEME_EFFECT_H
#define THEME_EFFECT_H
#include "effect.h"

// 四个预设主题：晨曦、极光、篝火、月影
// 结构完全相同，仅预设颜色不同。
Effect *theme_effect1_factory(void);  // AtmThemeMod=2  Aurora
Effect *theme_effect2_factory(void);  // AtmThemeMod=3  Dawn
Effect *theme_effect3_factory(void);  // AtmThemeMod=4  MoonShadow
Effect *theme_effect4_factory(void);  // AtmThemeMod=5  BonFire

#endif
