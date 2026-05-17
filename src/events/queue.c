#include <string.h>
#include "queue.h"
#include "types.h"
#include <stdio.h>

#define EVT_CAP 64
static Event evts[EVT_CAP];
static int ev_head=0, ev_tail=0;

void event_queue_init(void){ ev_head=ev_tail=0; }
void event_queue_push(const Event *e){
  int next = (ev_tail+1)%EVT_CAP;
  if(next==ev_head) return; // full, drop
  evts[ev_tail]=*e; ev_tail=next;
}
int event_queue_pop(Event *out){
  if(ev_head==ev_tail) return 0;
  *out = evts[ev_head]; ev_head=(ev_head+1)%EVT_CAP; return 1;
}
