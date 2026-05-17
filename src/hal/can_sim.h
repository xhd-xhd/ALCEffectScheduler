#ifndef CAN_SIM_H
#define CAN_SIM_H
#include <stdint.h>

void can_sim_init(void);
int  can_sim_poll(uint32_t elapsed_ms, uint8_t frame[64]);

#endif
