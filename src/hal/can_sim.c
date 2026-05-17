#include "can_sim.h"
#include <string.h>

// CAN FD frame format (simulated):
//   bytes 0..3  -> CAN ID  (uint32 LE)
//   bytes 4..63 -> payload (60 bytes)

typedef struct {
    uint32_t at_ms;
    uint32_t can_id;
    uint8_t  data[60];
} Step;

// 演示场景 —— 展示 zone 级仲裁:
//   t=500:  rl1 激活 (pri=5, roof front+mid, cyan)
//   t=2500: rl2 激活 (pri=6, roof mid+rear, orange)
//           → rl2 抢走 Z_ROOF_MID，rl1 只剩 Z_ROOF_FRONT
// 关键观察: 车顶中段从 cyan 变成 orange 的那一刻
static const Step scenario[] = {
    {    0, 0x400, {0x00} },  // mode 0 → ambient
    {  500, 0x400, {0x01} },  // mode 1 → rl1 ON (cyan, front+mid)
    { 1000, 0x100, {0x00, 0x01} },                               // door 0 open → welcome ON
    { 2500, 0x400, {0x02} },  // mode 2 → rl2 ON (orange, mid+rear) ← 抢走中段!
    { 4500, 0x200, {0x00, 0x00,0x00,0xA0,0x40} },                // rear alert ON
    { 7000, 0x200, {0x00, 0x00,0x00,0x00,0x00} },                // rear alert OFF
    { 9000, 0x400, {0x01} },  // mode 1 → rl1 OFF
    {11000, 0x100, {0x00, 0x00} },                               // door 0 close → welcome OFF
    {14000, 0x400, {0x02} },  // mode 2 → rl2 OFF
    {17000, 0x200, {0x01, 0x00,0x00,0x60,0x41} },                // rear alert again
    {20000, 0x200, {0x01, 0x00,0x00,0x00,0x00} },                // rear alert OFF
};

static int step_count = sizeof(scenario) / sizeof(scenario[0]);
static int step_idx   = 0;

void can_sim_init(void) {
    step_idx = 0;
}

int can_sim_poll(uint32_t elapsed_ms, uint8_t frame[64]) {
    if (step_idx >= step_count) return 0;
    if (elapsed_ms >= scenario[step_idx].at_ms) {
        memset(frame, 0, 64);
        uint32_t id = scenario[step_idx].can_id;
        memcpy(frame, &id, 4);
        memcpy(frame + 4, scenario[step_idx].data, 60);
        step_idx++;
        return 1;
    }
    return 0;
}
