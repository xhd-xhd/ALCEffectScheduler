#ifndef TYPES_H
#define TYPES_H
#include <stdint.h>

// ============================================================================
// ZoneMask — 多字位图，用位掩码表示"哪些灯受控"
// ============================================================================
// 每个 zone 对应一个 bit。128 个 zone 用 4 个 uint32_t 存储。
// 32 位 MCU 上 uint32_t 位运算是原生指令，不用软件模拟。
// 编译时 -DMAX_ZONES=256 可以扩到 256 个 zone。
#ifndef MAX_ZONES
#define MAX_ZONES 128
#endif

#define ZM_WORDS ((MAX_ZONES + 31) / 32)

typedef struct {
    uint32_t w[ZM_WORDS];
} ZoneMask;

static inline void zm_clear(ZoneMask *m) {
    for (int i = 0; i < ZM_WORDS; i++) m->w[i] = 0;
}

static inline void zm_set(ZoneMask *m, int z) {
    if ((unsigned)z < MAX_ZONES)
        m->w[z / 32] |= (1UL << (z % 32));
}

// 批量设置连续范围 [start, start+count)
static inline void zm_set_range(ZoneMask *m, int start, int count) {
    for (int i = 0; i < count; i++)
        zm_set(m, start + i);
}

static inline int zm_test(const ZoneMask *m, int z) {
    return ((unsigned)z < MAX_ZONES)
        && (m->w[z / 32] & (1UL << (z % 32)));
}

// 两个 mask 是否有重叠的 zone（用于优先级仲裁）
static inline int zm_intersects(const ZoneMask *a, const ZoneMask *b) {
    for (int i = 0; i < ZM_WORDS; i++)
        if (a->w[i] & b->w[i]) return 1;
    return 0;
}

// dst 并入 src 的所有 zone
static inline void zm_or(ZoneMask *dst, const ZoneMask *src) {
    for (int i = 0; i < ZM_WORDS; i++)
        dst->w[i] |= src->w[i];
}

// 从 a 中移除 b 的 zone: a = a & ~b （按 zone 粒度仲裁用）
static inline void zm_andnot(ZoneMask *a, const ZoneMask *b) {
    for (int i = 0; i < ZM_WORDS; i++)
        a->w[i] &= ~b->w[i];
}

// mask 是否为空（所有 zone 都被占 → 完全影子化）
static inline int zm_is_empty(const ZoneMask *m) {
    for (int i = 0; i < ZM_WORDS; i++)
        if (m->w[i]) return 0;
    return 1;
}

// 精确匹配（用于 deactivate 时按 name+mask 定位效果实例）
static inline int zm_eq(const ZoneMask *a, const ZoneMask *b) {
    for (int i = 0; i < ZM_WORDS; i++)
        if (a->w[i] != b->w[i]) return 0;
    return 1;
}

// ============================================================================
// 时间类型 — 32 位毫秒，从系统启动计时，约 49 天回绕
// ============================================================================
typedef uint32_t TimeMs;

// ============================================================================
// Event — CAN 帧解析后的语义化事件
// ============================================================================
// 一个 CAN 帧可能产生一个或多个 Event。Event 描述"车上发生了什么"，
// 还不涉及具体哪个灯效应该响应——那是 commands/processor.c 的职责。
typedef enum {
    EVT_DOOR,       // 车门开关
    EVT_SEAT,       // 座椅占用
    EVT_SPEED,      // 车速
    EVT_MODE,       // 氛围灯模式切换
    EVT_MUSIC_BEAT, // 音乐节拍
    EVT_REAR_ALERT, // 后雷达告警
    EVT_RAW         // 原始 CAN 帧透传
} EventType;

typedef struct {
    EventType type;
    TimeMs    ts;        // 事件发生时刻
    union {
        struct { int door_index; int opened; }   door;
        struct { int seat_index; int occupied; } seat;
        struct { float kph; }                     speed;
        struct { int mode_id; void *params; }     mode;
        struct { float intensity; int phase; }    music;
        struct { int side; float distance; }      rear;
        uint8_t raw[64];
    } data;
} Event;

// ============================================================================
// EffectCommand — 对 EffectManager 的调度指令
// ============================================================================
// 由 commands/processor.c 根据 Event 产生（含边沿检测）。
// EffectManager 根据 Command 创建/销毁/暂停灯效实例。
typedef enum {
    CMD_ACTIVATE,   // 激活灯效
    CMD_DEACTIVATE, // 停用灯效
    CMD_UPDATE,     // 更新参数
    CMD_PAUSE,      // 暂停（预留）
    CMD_RESUME      // 恢复（预留）
} CmdType;

typedef struct {
    CmdType     type;
    const char *effect_name;   // 对应 em_register_factory 注册的名字
    ZoneMask    mask;          // 效果作用的 zone 范围
    int         priority;      // 优先级，越大越高
    void       *params;        // 可选参数
    TimeMs      ts;
} EffectCommand;

#endif
