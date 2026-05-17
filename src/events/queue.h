#ifndef EVENTS_QUEUE_H
#define EVENTS_QUEUE_H
#include "types.h"

void event_queue_init(void);
void event_queue_push(const Event *e);
int event_queue_pop(Event *out);

#endif // EVENTS_QUEUE_H
