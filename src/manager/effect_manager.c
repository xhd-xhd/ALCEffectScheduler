#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "effect_manager.h"
#include "effects/effect.h"

// ============================================================================
// EffectManager — 灯效调度核心实现
// ============================================================================
// 效果在注册时就创建并按优先级排序，之后只遍历不改结构。
// 每帧 em_update_all() 做的事:
//   1. 按序（已排好）遍历所有效果，跳过 IDLE 的
//   2. 从高到低: 效果 mask 与已占用 zone 无重叠 → 渲染，否则 → 影子化
//   3. 自动切换效果状态: ACTIVE ↔ SHADOWED

#define MAX_EFFECTS 32

static Effect *effects[MAX_EFFECTS];
static int     effect_count = 0;

void em_init(void) {
    memset(effects, 0, sizeof(effects));
    effect_count = 0;
}

void em_register_factory(const char *name, Effect *(*factory)(void)) {
    if (effect_count >= MAX_EFFECTS) return;

    Effect *e = factory();
    if (!e) return;

    // 按优先级降序插入（高优先级在前），后续遍历不用排序
    int pos = effect_count;
    for (int i = 0; i < effect_count; i++) {
        if (e->priority > effects[i]->priority) {
            pos = i;
            break;
        }
    }
    for (int j = effect_count; j > pos; j--)
        effects[j] = effects[j - 1];
    effects[pos] = e;
    effect_count++;
}

void em_process_command(const EffectCommand *cmd) {
    if (!cmd || !cmd->effect_name || cmd->effect_name[0] == '\0') return;

    // 按名字查找效果（每种效果只有一个实例）
    for (int i = 0; i < effect_count; i++) {
        Effect *e = effects[i];
        if (!e || strcmp(e->name, cmd->effect_name) != 0) continue;

        if (cmd->type == CMD_ACTIVATE) {
            // IDLE → ACTIVE: 调用 init 启动效果
            if (e->state == S_IDLE) {
                if (e->init) e->init(e, cmd->ts);
            }
        } else if (cmd->type == CMD_DEACTIVATE) {
            // ACTIVE/SHADOWED → IDLE: 调用 deinit 停用效果
            if (e->state == S_ACTIVE || e->state == S_SHADOWED) {
                if (e->deinit) e->deinit(e);
                e->state = S_IDLE;
            }
        }
        // 优先级在注册时已确定，不做运行时覆盖
        return;
    }
}

void em_update_all(TimeMs now, LedOutput *out) {
    if (!out) return;
    memset(out, 0, sizeof(LedOutput));
    if (effect_count == 0) return;

    ZoneMask occupied;
    zm_clear(&occupied);

    // 按优先级顺序遍历，每个效果只渲染没被占的 zone
    for (int i = 0; i < effect_count; i++) {
        Effect *e = effects[i];
        if (!e || e->state == S_IDLE) continue;

        // 计算该效果实际能渲染的 zone：mask 减去已被占用的
        ZoneMask render_mask = e->mask;  // struct copy
        zm_andnot(&render_mask, &occupied);

        if (zm_is_empty(&render_mask)) {
            // 所有 zone 都已被高优先级效果占用 → 完全影子化
            if (e->state == S_ACTIVE)
                e->state = S_SHADOWED;
            if (e->update)
                e->update(e, now, 0, out);
        } else {
            if (e->state == S_SHADOWED)
                e->state = S_ACTIVE;

            // 临时把 mask 换成实际能渲染的子集，效果照常遍历 mask 输出
            ZoneMask saved_mask = e->mask;
            e->mask = render_mask;
            if (e->update)
                e->update(e, now, 1, out);
            e->mask = saved_mask;

            // 只标记实际渲染的 zone 为已占用
            zm_or(&occupied, &render_mask);
        }
    }
}

void em_shutdown(void) {
    for (int i = 0; i < effect_count; i++) {
        if (effects[i])
            effect_destroy(effects[i]);
        effects[i] = NULL;
    }
    effect_count = 0;
}

int em_get_active_info(EffectInfo *out, int max) {
    int n = 0;
    for (int i = 0; i < effect_count && n < max; i++) {
        Effect *e = effects[i];
        if (!e || e->state == S_IDLE) continue;
        out[n].name     = e->name;
        out[n].priority = e->priority;
        out[n].state    = (int)e->state;
        out[n].mask     = e->mask;
        n++;
    }
    return n;
}
