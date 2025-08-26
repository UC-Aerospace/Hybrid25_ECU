#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <stdint.h>
#include <stdbool.h>

extern bool fire;

void uninitialise_sequencer(void);
void set_sequencer_ready(void);
void sequencer_tick(void);
bool set_burn_time(uint32_t new_burn_time);

#endif /* SEQUENCER_H */