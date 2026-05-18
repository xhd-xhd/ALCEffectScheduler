#include "can_sim.h"
#include "can_frames.h"
#include <string.h>

// ============================================================================
// CAN 模拟器 — 使用 CanFrame 位域构造真实 0x2CF 帧
// ============================================================================
// 演示场景: Welcome → RunningLight1 → FootwellPulse → RunningLight2 → RearAlert

typedef struct {
    uint32_t       at_ms;
    void          (*build)(CanFrame *f);
} Step;

// ---- 各步骤的帧构造函数 ----------------------------------------------------

static void build_ambient(CanFrame *f) {
    memset(f, 0, sizeof(CanFrame));
    f->id = 0x2CF;
    f->canfd_0x2cf.VIU_AL_PowerSt    = 1;   // 上电
    f->canfd_0x2cf.VIU_AL_AtmThemeMod = 0;   // 默认主题
}

static void build_rl1_on(CanFrame *f) {
    memset(f, 0, sizeof(CanFrame));
    f->id = 0x2CF;
    f->canfd_0x2cf.VIU_AL_PowerSt    = 1;
    f->canfd_0x2cf.VIU_AL_AtmThemeMod = 1;   // 主题 1 → rl1
}

static void build_welcome_on(CanFrame *f) {
    memset(f, 0, sizeof(CanFrame));
    f->id = 0x2CF;
    f->canfd_0x2cf.VIU_AL_PowerSt    = 1;
    f->canfd_0x2cf.VIU_AL_AtmThemeMod = 1;
    f->canfd_0x2cf.VIU_AL_Welcome     = 1;   // 迎宾激活
}

static void build_fp_on(CanFrame *f) {
    memset(f, 0, sizeof(CanFrame));
    f->id = 0x2CF;
    f->canfd_0x2cf.VIU_AL_PowerSt    = 1;
    f->canfd_0x2cf.VIU_AL_AtmThemeMod = 3;   // 主题 3 → footwell_pulse
    f->canfd_0x2cf.VIU_AL_Welcome     = 1;
}

static void build_rl2_on(CanFrame *f) {
    memset(f, 0, sizeof(CanFrame));
    f->id = 0x2CF;
    f->canfd_0x2cf.VIU_AL_PowerSt    = 1;
    f->canfd_0x2cf.VIU_AL_AtmThemeMod = 2;   // 主题 2 → rl2  ← 此时主题切到2，rl1/fp应停
    f->canfd_0x2cf.VIU_AL_Welcome     = 1;
}

static void build_rear_on(CanFrame *f) {
    memset(f, 0, sizeof(CanFrame));
    f->id = 0x2CF;
    f->canfd_0x2cf.VIU_AL_PowerSt    = 1;
    f->canfd_0x2cf.VIU_AL_AtmThemeMod = 2;
    f->canfd_0x2cf.VIU_AL_Welcome     = 1;
    f->canfd_0x2cf.VIU_AL_RearWarn    = 1;   // 后雷达告警激活
}

static void build_rear_off(CanFrame *f) {
    memset(f, 0, sizeof(CanFrame));
    f->id = 0x2CF;
    f->canfd_0x2cf.VIU_AL_PowerSt    = 1;
    f->canfd_0x2cf.VIU_AL_AtmThemeMod = 2;
    f->canfd_0x2cf.VIU_AL_Welcome     = 1;
    f->canfd_0x2cf.VIU_AL_RearWarn    = 0;   // 后雷达告警解除
}

static void build_welcome_off(CanFrame *f) {
    memset(f, 0, sizeof(CanFrame));
    f->id = 0x2CF;
    f->canfd_0x2cf.VIU_AL_PowerSt    = 1;
    f->canfd_0x2cf.VIU_AL_AtmThemeMod = 2;
    f->canfd_0x2cf.VIU_AL_Welcome     = 0;   // 迎宾结束
}

static void build_all_off(CanFrame *f) {
    memset(f, 0, sizeof(CanFrame));
    f->id = 0x2CF;
    f->canfd_0x2cf.VIU_AL_PowerSt    = 1;
    // 全部清零 = 只剩 ambient
}

static void build_rear_again(CanFrame *f) {
    memset(f, 0, sizeof(CanFrame));
    f->id = 0x2CF;
    f->canfd_0x2cf.VIU_AL_PowerSt    = 1;
    f->canfd_0x2cf.VIU_AL_RearWarn    = 1;
}

static void build_power_off(CanFrame *f) {
    memset(f, 0, sizeof(CanFrame));
    f->id = 0x2CF;
    f->canfd_0x2cf.VIU_AL_PowerSt    = 0;   // 断电，全部关
}

// ---- 场景时间线 ------------------------------------------------------------
static const Step scenario[] = {
    {    0, build_ambient },      // 上电 → ambient 激活
    {  500, build_rl1_on },       // 主题1 → rl1 cyan 流水
    { 1000, build_welcome_on },   // 开门 → welcome 蓝色呼吸
    { 1800, build_fp_on },        // 主题3 → footwell_pulse 琥珀呼吸
    { 2500, build_rl2_on },       // 主题2 → rl2 orange 流水, rl1/fp 停
    { 4500, build_rear_on },      // 雷达告警 → rear_alert 红色闪烁(最高优先)
    { 7000, build_rear_off },     // 雷达解除 → rear_alert 停
    { 9000, build_welcome_off },  // 关门 → welcome 停
    {11000, build_all_off },      // 主题归零 → rl2 停
    {14000, build_rear_again },   // 雷达再次触发
    {17000, build_power_off },    // 断电 → 全部停
};

static int step_count = sizeof(scenario) / sizeof(scenario[0]);
static int step_idx   = 0;

void can_sim_init(void) {
    step_idx = 0;
}

int can_sim_poll(uint32_t elapsed_ms, uint8_t frame[64]) {
    if (step_idx >= step_count) return 0;
    if (elapsed_ms >= scenario[step_idx].at_ms) {
        CanFrame f;
        scenario[step_idx].build(&f);
        memcpy(frame, f.raw, 64);
        step_idx++;
        return 1;
    }
    return 0;
}
