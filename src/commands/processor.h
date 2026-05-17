#ifndef COMMANDS_PROCESSOR_H
#define COMMANDS_PROCESSOR_H
#include "types.h"

void commands_init(void);
void event_to_command(const Event *e, EffectCommand *out);
void command_queue_push(const EffectCommand *cmd);
int command_queue_pop(EffectCommand *out);

#endif // COMMANDS_PROCESSOR_H
