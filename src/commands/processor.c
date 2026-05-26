#include <string.h>
#include "processor.h"
#include "can_frames.h"
#include "manager/effect_manager.h"
#include "effects/video_ambient.h"
#include "effects/theme_music.h"

// ============================================================================
// 命令处理器 — 读 g_rx_frame → 边沿检测 → EffectCommand
// ============================================================================
// 每个信号独立判断触发条件，不关心其他效果。
// EffectManager 根据优先级 + zone mask 自动仲裁覆盖。

#define CMD_CAP 64
static EffectCommand cmdq[CMD_CAP];
static int cmd_head = 0, cmd_tail = 0;

static void push_cmd(CmdType type, const char *name, TimeMs ts) {
    if (!name) return;
    int next = (cmd_tail + 1) % CMD_CAP;
    if (next == cmd_head) return;
    EffectCommand *c = &cmdq[cmd_tail];
    memset(c, 0, sizeof(*c));
    c->ts          = ts;
    c->type        = type;
    c->effect_name = name;
    cmd_tail = next;
}

// ---- 各信号的 prev 值 ------------------------------------------------------
static struct {
    uint8_t power, theme, nap, start, welcome, driving, voice;
    uint8_t fl_ac, fr_ac, rl_ac, rr_ac;
    uint8_t fl_door, fr_door, rl_door, rr_door;
    uint8_t fl_rear, fr_rear, rl_rear, rr_rear, tweeter, video_amb;
} pv;

void commands_init(void) {
    cmd_head = cmd_tail = 0;
    memset(&pv, 0, sizeof(pv));
}

// ---- 小憩阶段映射 --------------------------------------------------------
static const char *nap_name(uint8_t n) {
    switch (n) {
    case 1:  return "nap_prepare1";
    case 2:  return "nap_prepare2";
    case 3:  return "nap_sleep";
    case 4:  return "nap_wake";
    default: return NULL;
    }
}

// ---- 主题名映射 ------------------------------------------------------------ ------------------------------------------------------------
static const char *theme_name(uint8_t t) {
    switch (t) {
    case 0:  return "theme_off";
    case 1:  return "theme_custom";
    case 2:  return "theme_effect1";
    case 3:  return "theme_effect2";
    case 4:  return "theme_effect3";
    case 5:  return "theme_effect4";
    case 6:  return "theme_music";
    default: return NULL;
    }
}

// ---- 主入口 ----------------------------------------------------------------
void commands_tick(TimeMs now) {
    if (g_rx_frame.id != 0x2CF) return;

    uint8_t power   = g_rx_frame.canfd_0x2cf.VIU_AL_PowerSt;
    uint8_t theme   = g_rx_frame.canfd_0x2cf.VIU_AL_AtmThemeMod;
    uint8_t nap     = g_rx_frame.canfd_0x2cf.VIU_AL_NapMod;
    uint8_t start   = g_rx_frame.canfd_0x2cf.VIU_AL_StartMotion;
    uint8_t welcome = g_rx_frame.canfd_0x2cf.VIU_AL_Welcome;
    uint8_t driving = g_rx_frame.canfd_0x2cf.VIU_AL_DrivingMod;
    uint8_t voice   = g_rx_frame.canfd_0x2cf.VIU_AL_VoiceInteractIndcrSt;
    uint8_t fl_ac   = g_rx_frame.canfd_0x2cf.VIU_AL_FLAC;
    uint8_t fr_ac   = g_rx_frame.canfd_0x2cf.VIU_AL_FRAC;
    uint8_t rl_ac   = g_rx_frame.canfd_0x2cf.VIU_AL_RLAC;
    uint8_t rr_ac   = g_rx_frame.canfd_0x2cf.VIU_AL_RRAC;
    uint8_t fl_door = g_rx_frame.canfd_0x2cf.VIU_AL_FLDoorWarn;
    uint8_t fr_door = g_rx_frame.canfd_0x2cf.VIU_AL_FRDoorWarn;
    uint8_t rl_door = g_rx_frame.canfd_0x2cf.VIU_AL_RLDoorWarn;
    uint8_t rr_door = g_rx_frame.canfd_0x2cf.VIU_AL_RRDoorWarn;
    uint8_t fl_rear = g_rx_frame.canfd_0x2cf.VIU_AL_FLRearWarn;
    uint8_t fr_rear = g_rx_frame.canfd_0x2cf.VIU_AL_FRRearWarn;
    uint8_t rl_rear = g_rx_frame.canfd_0x2cf.VIU_AL_RLRearWarn;
    uint8_t rr_rear = g_rx_frame.canfd_0x2cf.VIU_AL_RRRearWarn;

    // L0 电源
    if (power != pv.power) {
        push_cmd(power ? CMD_ACTIVATE : CMD_DEACTIVATE, "ambient", now);
        pv.power = power;
    }
    if (!power) return;

    // L1 氛围主题
    if (theme != pv.theme) {
        push_cmd(CMD_DEACTIVATE, theme_name(pv.theme), now);
        push_cmd(CMD_ACTIVATE,   theme_name(theme),    now);
        pv.theme = theme;
    }
    if (theme == 6) em_set_mask("theme_music", theme_music_active_mask());

    // L4 小憩模式 (4 个阶段，值切换时换效果)
    if (nap != pv.nap) {
        push_cmd(CMD_DEACTIVATE, nap_name(pv.nap), now);
        push_cmd(CMD_ACTIVATE,   nap_name(nap),    now);
        pv.nap = nap;
    }
    // L5 启动动画
    if (start != pv.start) {
        push_cmd(start ? CMD_ACTIVATE : CMD_DEACTIVATE, "start_motion", now);
        pv.start = start;
    }
    // L6 迎宾
    if (welcome != pv.welcome) {
        push_cmd(welcome ? CMD_ACTIVATE : CMD_DEACTIVATE, "welcome", now);
        pv.welcome = welcome;
    }
    // L7 驾驶模式
    if (driving != pv.driving) {
        push_cmd(driving ? CMD_ACTIVATE : CMD_DEACTIVATE, "driving", now);
        pv.driving = driving;
    }
    // L8 空调联动
    {
        uint8_t ac_on  = fl_ac || fr_ac || rl_ac || rr_ac;
        uint8_t ac_was = pv.fl_ac || pv.fr_ac || pv.rl_ac || pv.rr_ac;
        pv.fl_ac = fl_ac; pv.fr_ac = fr_ac;
        pv.rl_ac = rl_ac; pv.rr_ac = rr_ac;
        if (ac_on != ac_was)
            push_cmd(ac_on ? CMD_ACTIVATE : CMD_DEACTIVATE, "temperature", now);
    }
    // L9 语音交互
    {
        uint8_t voice_on  = (voice == 2 || voice == 4 || voice == 7);
        uint8_t voice_was = (pv.voice == 2 || pv.voice == 4 || pv.voice == 7);
        pv.voice = voice;
        if (voice_on != voice_was)
            push_cmd(voice_on ? CMD_ACTIVATE : CMD_DEACTIVATE, "voice", now);
    }
    // L10 开门预警
    {
        uint8_t door_on  = fl_door || fr_door || rl_door || rr_door;
        uint8_t door_was = pv.fl_door || pv.fr_door || pv.rl_door || pv.rr_door;
        pv.fl_door = fl_door; pv.fr_door = fr_door;
        pv.rl_door = rl_door; pv.rr_door = rr_door;
        if (door_on != door_was)
            push_cmd(door_on ? CMD_ACTIVATE : CMD_DEACTIVATE, "door_warn", now);
    }
    // L11 后方来车
    {
        uint8_t rear_on  = fl_rear || fr_rear || rl_rear || rr_rear;
        uint8_t rear_was = pv.fl_rear || pv.fr_rear || pv.rl_rear || pv.rr_rear;
        pv.fl_rear = fl_rear; pv.fr_rear = fr_rear;
        pv.rl_rear = rl_rear; pv.rr_rear = rr_rear;
        if (rear_on != rear_was)
            push_cmd(rear_on ? CMD_ACTIVATE : CMD_DEACTIVATE, "rear_alert", now);
    }
    // L12 施罗德使能
    {
        uint8_t tw = g_rx_frame.canfd_0x2cf.VIU_AL_TwetterLampEnable;
        if (tw != pv.tweeter) {
            push_cmd(tw == 1 ? CMD_DEACTIVATE : CMD_ACTIVATE, "tweeter_off", now);
            pv.tweeter = tw;
        }
    }
    // L3 视频氛围 光随影动 (动态 mask)
    {
        ZoneMask va_mask = video_ambient_active_mask();
        uint8_t  va_on   = !zm_is_empty(&va_mask);
        if (va_on != pv.video_amb) {
            push_cmd(va_on ? CMD_ACTIVATE : CMD_DEACTIVATE, "video_ambient", now);
            pv.video_amb = va_on;
        }
    }
}

// ---- 兼容旧接口 ------------------------------------------------------------
void event_to_command(const Event *e, EffectCommand *out) {
    (void)e;
    memset(out, 0, sizeof(EffectCommand));
    out->effect_name = "";
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
