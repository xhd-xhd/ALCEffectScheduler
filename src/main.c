#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "hal.h"
#include "types.h"
#include "led_map.h"
#include "manager/effect_manager.h"
#include "can/parser.h"
#include "render/renderer.h"
#include "commands/processor.h"
#include "effects/rear_alert.h"
#include "effects/welcome.h"
#include "effects/ambient.h"
#include "effects/running_light.h"
#include "effects/footwell_pulse.h"
#include "effects/theme_off.h"
#include "effects/theme_custom.h"
#include "effects/theme_effect.h"
#include "effects/theme_music.h"
#include "effects/nap_prepare1.h"
#include "effects/nap_prepare2.h"
#include "effects/nap_sleep.h"
#include "effects/nap_wake.h"
#include "effects/tweeter_off.h"
#include "effects/video_ambient.h"

static volatile int running = 1;
static void on_sigint(int _) { (void)_; running = 0; }

int main(void) {
    signal(SIGINT, on_sigint);

    hal_init();
    can_parser_init();
    commands_init();
    em_init();

    em_register_factory("rear_alert",       rear_alert_factory);
    em_register_factory("welcome",          welcome_factory);
    em_register_factory("running_light_2",  running_light_2_factory);
    em_register_factory("running_light_1",  running_light_1_factory);
    em_register_factory("ambient",          ambient_factory);
    em_register_factory("footwell_pulse",   footwell_pulse_factory);
    
    em_register_factory("theme_off",        theme_off_factory);
    em_register_factory("theme_custom",     theme_custom_factory);
    em_register_factory("theme_effect1",    theme_effect1_factory);
    em_register_factory("theme_effect2",    theme_effect2_factory);
    em_register_factory("theme_effect3",    theme_effect3_factory);
    em_register_factory("theme_effect4",    theme_effect4_factory);
    em_register_factory("theme_music",      theme_music_factory);
    em_register_factory("nap_prepare1",     nap_prepare1_factory);
    em_register_factory("nap_prepare2",     nap_prepare2_factory);
    em_register_factory("nap_sleep",        nap_sleep_factory);
    em_register_factory("nap_wake",         nap_wake_factory);
    em_register_factory("tweeter_off",      tweeter_off_factory);
    em_register_factory("video_ambient",    video_ambient_factory);

    puts("\033[2J\033[HALC Effect Scheduler -- start (Ctrl+C)");
    hal_delay_ms(500);

    while (running) {
        uint32_t now = hal_millis();
        uint8_t frame[64];
        EffectCommand cmd;

        while (hal_can_read(frame))
            can_handle_frame(frame);

        commands_tick(now);

        while (command_queue_pop(&cmd))
            em_process_command(&cmd);

        LedOutput out;
        em_update_all(now, &out);
        commit_led_output(&out);

        uint32_t elapsed = hal_millis() - now;
        if (elapsed < 10)
            hal_delay_ms(10 - elapsed);
    }

    puts("\033[2J\033[HALC Effect Scheduler -- stopped.");
    em_shutdown();
    return 0;
}
