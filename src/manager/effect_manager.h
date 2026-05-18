#ifndef EFFECT_MANAGER_H
#define EFFECT_MANAGER_H
#include "types.h"
#include "led_map.h"

typedef struct Effect Effect;

// ============================================================================
// EffectManager — 灯效调度核心
// ============================================================================
// 职责:
//   1. 工厂注册 — 创建效果并按优先级排序插入，效果持续存在不销毁
//   2. 命令处理 — CMD_ACTIVATE/DEACTIVATE 切换效果 IDLE/ACTIVE 状态
//   3. 每帧调度 — 遍历已排序的效果，跳过 IDLE 的，高优先级先渲染
//   4. 仲裁 — 高优先级效果占用的 zone，低优先级效果自动进入 SHADOWED

void em_init(void);
void em_register_factory(const char *name, Effect* (*factory)(void));
void em_process_command(const EffectCommand *cmd);
void em_update_all(TimeMs now, LedOutput *out);
void em_shutdown(void);
void em_set_mask(const char *name, ZoneMask mask);

// 查询当前活跃效果（供仪表板调试用）
typedef struct {
    const char *name;
    int         priority;
    int         state;   // EffectState
    ZoneMask    mask;
} EffectInfo;
int  em_get_active_info(EffectInfo *out, int max);

#endif
