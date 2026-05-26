#ifndef CAN_PARSER_H
#define CAN_PARSER_H
#include "types.h"

void can_parser_init(void);
int  can_handle_frame(const uint8_t frame[64]);

#endif
