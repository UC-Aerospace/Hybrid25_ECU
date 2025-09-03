#ifndef SEQUENCER_H
#define SEQUENCER_H

#include "stm32g0xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    SEQUENCER_READY = 0b0000,
    SEQUENCER_COUNTDOWN = 0b0001,
    SEQUENCER_FIRE = 0b0010,
    SEQUENCER_FAILED_START = 0b1111,
    SEQUENCER_UNINITIALISED = 0b1110
} sequencer_states_t;

void sequencer_set_state(sequencer_states_t new_state);
sequencer_states_t sequencer_get_state(void);
void sequencer_tick(void);
void sequencer_fire(uint8_t length);

#endif /* SEQUENCER_H */