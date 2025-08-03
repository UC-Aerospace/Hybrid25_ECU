#ifndef FSM_H
#define FSM_H

#include "stm32g0xx_hal.h"
#include "config.h"
#include "peripherals.h"

#define MAX_MOVE_DURATION 1000 // Maximum duration for a move in milliseconds

typedef enum {
    STATE_INIT,
    STATE_READY,
    STATE_MOVING,
    STATE_ERROR
} fsm_state_t;

void fsm_init(void);
void fsm_run(void);
void fsm_set_state(fsm_state_t state);
fsm_state_t fsm_get_state(void);

#endif // FSM_H