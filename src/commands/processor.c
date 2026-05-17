#include <string.h>
#include <stdio.h>
#include "processor.h"
#include "led_map.h"

// ============================================================================
// 命令处理器 — Event → EffectCommand 转换 + 边沿检测
// ============================================================================
// mask 不在这里设定——每个灯效的 mask 在工厂注册时已确定。
// 这里只负责根据 CAN 信号生成"激活谁 / 停用谁"的指令。
//
// 门计数逻辑: 只要有一扇门开着，welcome 效果就保持激活。
//            所有门都关了才停用 welcome。
// 雷达计数同理: 任意一侧有障碍物就激活 rear_alert，两侧都清空才停用。

#define CMD_CAP 64
static EffectCommand cmdq[CMD_CAP];
static int cmd_head = 0, cmd_tail = 0;

static int door_open_count;       // 当前开着的门数量
static int rear_alert_count;      // 当前有障碍物的雷达侧数

void commands_init(void) {
    cmd_head = cmd_tail = 0;
    door_open_count  = 0;
    rear_alert_count = 0;
}

void event_to_command(const Event *e, EffectCommand *out) {
    if (!e || !out) return;
    memset(out, 0, sizeof(EffectCommand));
    out->ts = e->ts;

    switch (e->type) {
    case EVT_DOOR: {
        int now_open = e->data.door.opened;
        int was_open_cnt = door_open_count;

        if (now_open) door_open_count++; else if (door_open_count > 0) door_open_count--;

        if (was_open_cnt == 0 && door_open_count > 0) {
            // 第一扇门打开 → 激活迎宾
            out->type        = CMD_ACTIVATE;
            out->effect_name = "welcome";
            out->priority    = 7;
        } else if (was_open_cnt > 0 && door_open_count == 0) {
            // 最后一扇门关闭 → 停用迎宾
            out->type        = CMD_DEACTIVATE;
            out->effect_name = "welcome";
        } else {
            out->type        = CMD_UPDATE;
            out->effect_name = "";
        }
        break;
    }
    case EVT_REAR_ALERT: {
        float now_dist = e->data.rear.distance;
        int was_cnt = rear_alert_count;

        if (now_dist > 0.0f) rear_alert_count++;
        else if (rear_alert_count > 0) rear_alert_count--;

        if (was_cnt == 0 && rear_alert_count > 0) {
            out->type        = CMD_ACTIVATE;
            out->effect_name = "rear_alert";
            out->priority    = 10;
        } else if (was_cnt > 0 && rear_alert_count == 0) {
            out->type        = CMD_DEACTIVATE;
            out->effect_name = "rear_alert";
        } else {
            out->type        = CMD_UPDATE;
            out->effect_name = "";
        }
        break;
    }
    case EVT_MODE: {
        int mid = e->data.mode.mode_id;

        // ambient: 首次收到任意 mode 事件时激活（常驻背景）
        static int ambient_on = 0;
        if (!ambient_on) {
            ambient_on = 1;
            out->type        = CMD_ACTIVATE;
            out->effect_name = "ambient";
            out->priority    = 1;
            break;
        }

        // running_light_1: mode==1 时 toggle（cyan, pri=5, roof front+mid）
        // running_light_2: mode==2 时 toggle（orange, pri=6, roof mid+rear）
        if (mid == 1 || mid == 2) {
            static int rl1_on = 0, rl2_on = 0;
            if (mid == 1) {
                rl1_on = !rl1_on;
                out->type        = rl1_on ? CMD_ACTIVATE : CMD_DEACTIVATE;
                out->effect_name = "running_light_1";
                out->priority    = 5;
            } else {
                rl2_on = !rl2_on;
                out->type        = rl2_on ? CMD_ACTIVATE : CMD_DEACTIVATE;
                out->effect_name = "running_light_2";
                out->priority    = 6;
            }
        } else {
            out->type        = CMD_UPDATE;
            out->effect_name = "";
        }
        break;
    }
    default:
        out->type        = CMD_UPDATE;
        out->effect_name = "";
        break;
    }
}

void command_queue_push(const EffectCommand *cmd) {
    int next = (cmd_tail + 1) % CMD_CAP;
    if (next == cmd_head) return;
    cmdq[cmd_tail] = *cmd;
    cmd_tail = next;
}

int command_queue_pop(EffectCommand *out) {
    if (cmd_head == cmd_tail) return 0;
    *out = cmdq[cmd_head];
    cmd_head = (cmd_head + 1) % CMD_CAP;
    return 1;
}
