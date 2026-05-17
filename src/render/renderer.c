#include "renderer.h"
#include "hal.h"

void commit_led_output(const LedOutput *out) {
    hal_led_send(out);
}
