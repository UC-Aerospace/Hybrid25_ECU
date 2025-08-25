#ifndef SEQUENCER_H
#define SEQUENCER_H

#include <stdbool.h>

extern bool fire;

void uninitialise_sequencer(void);
void set_sequencer_ready(void);
void sequencer_tick(void);

#endif /* SEQUENCER_H */