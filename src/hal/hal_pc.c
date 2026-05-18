#include "hal.h"
#include "can_sim.h"
#include "manager/effect_manager.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

static uint64_t start_ms;

void hal_init(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    start_ms = (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    can_sim_init();
}

uint32_t hal_millis(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t now = (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    return (uint32_t)(now - start_ms);
}

void hal_delay_ms(uint32_t ms) {
    struct timespec ts = {
        .tv_sec  = ms / 1000,
        .tv_nsec = (long)(ms % 1000) * 1000000L,
    };
    nanosleep(&ts, NULL);
}

int hal_can_read(uint8_t frame[64]) {
    return can_sim_poll(hal_millis(), frame);
}

// ---- terminal dashboard ---------------------------------------------------

static const char *zone_names[] = {
    // 流水灯
    "IP-Flow-1  ", "IP-Flow-2  ", "IP-Flow-3  ",
    // 仪表高位
    "IP-H1      ", "IP-H2      ", "IP-H3      ",
    "IP-H4      ", "IP-H5      ", "IP-H6      ",
    // 左前门
    "LF-Foot    ", "LF-Map     ", "LF-Up-A    ", "LF-Up-B    ", "LF-Spk     ",
    // 左后门
    "LR-Foot    ", "LR-Map     ", "LR-Up-A    ", "LR-Up-B    ",
    // 右前门
    "RF-Foot    ", "RF-Map     ", "RF-Up-A    ", "RF-Up-B    ", "RF-Spk     ",
    // 右后门
    "RR-Foot    ", "RR-Map     ", "RR-Up-A    ", "RR-Up-B    ",
    // 中控
    "Center     ",
};
static const char *state_names[] = {
    "IDLE", "ACTIVE", "SHADOWED"
};

static void bar_rgb(uint8_t r, uint8_t g, uint8_t b, uint8_t br) {
    uint8_t rr = (uint8_t)(((uint16_t)r * br) / 255);
    uint8_t gg = (uint8_t)(((uint16_t)g * br) / 255);
    uint8_t bb = (uint8_t)(((uint16_t)b * br) / 255);
    if (br == 0) { rr = gg = bb = 0; }
    printf("\033[48;2;%d;%d;%dm", rr, gg, bb);
    for (int i = 0; i < 24; i++) putchar(' ');
    printf("\033[0m");
}

void hal_led_send(const LedOutput *out) {
    printf("\033[H");

    // ---- LED zones ----
    printf("┌──────────────────────────────────────────────────────────────────┐\n");
    printf("│         ALC Effect Scheduler  —  PC Simulator                   │\n");
    printf("├──────────────────────────────────────────────────────────────────┤\n");
    printf("│  LED Zones  (ZONE_COUNT=%d)                                      │\n", ZONE_COUNT);
    for (int z = 0; z < ZONE_COUNT; z++) {
        const ZoneLayout *zl = &g_zone_layout[z];
        LedPixel p = out->pixels[zl->offset];  // 显示每个 zone 的第一个像素
        printf("│  %s", zone_names[z]);
        if (zl->pixel_count > 1)
            printf(" ×%-3d", zl->pixel_count);
        else
            printf("     ");
        printf("  ");
        bar_rgb(p.r, p.g, p.b, p.l);
        printf("  R:%3d G:%3d B:%3d l:%3d  │\n", p.r, p.g, p.b, p.l);
    }

    // ---- active effects ----
    EffectInfo info[16];
    int n = em_get_active_info(info, 16);
    printf("├──────────────────────────────────────────────────────────────────┤\n");
    printf("│  Active Effects: %-2d                                             │\n", n);
    for (int i = 0; i < n; i++) {
        const char *sn = (info[i].state >= 0 && info[i].state <= 4)
                         ? state_names[info[i].state] : "?";
        printf("│    %-14s  pri=%2d  %-8s  mask=0x%08lx                   │\n",
               info[i].name, info[i].priority, sn,
               (unsigned long)info[i].mask.w[0]);
    }
    if (n == 0)
        printf("│    (none)                                                       │\n");

    printf("├──────────────────────────────────────────────────────────────────┤\n");
    printf("│  Time: %8.3f s  (Ctrl+C to stop)                              │\n",
           (double)hal_millis() / 1000.0);
    printf("└──────────────────────────────────────────────────────────────────┘\n");
}
