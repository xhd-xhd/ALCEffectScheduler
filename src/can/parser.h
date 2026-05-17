#ifndef CAN_PARSER_H
#define CAN_PARSER_H
#include "types.h"
void can_parser_init(void);
int can_parse_frame(const uint8_t frame[64], Event *out_events, int max_out);

#endif // CAN_PARSER_H
