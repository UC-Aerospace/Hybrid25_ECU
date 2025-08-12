#ifndef STAGER_H
#define STAGER_H

#include "stm32g0xx_hal.h"
#include <stdbool.h>

typedef enum {
    STATE_INIT = 0b0000,
    STATE_READY = 0b0001,
    STATE_SEQUENCER = 0b0010,
    STATE_POST_FIRE = 0b0011,
    STATE_MANUAL_MODE = 0b1000,
    STATE_ERROR = 0b1110,
    STATE_ABORT = 0b1111
} stager_state_t;

typedef enum {
    SEQUENCER_READY = 0b0000,
    SEQUENCER_COUNTDOWN = 0b0001,
    SEQUENCER_FIRE = 0b0010,
    SEQUENCER_FAILED_START = 0b1111,
    SEQUENCER_UNINITIALIZED = 0b1110
} stager_sequencer_substate_t;

typedef struct {
    bool master_power;
    bool master_valve;
    bool master_pyro;
    bool sequencer_override;
    bool valve_nitrous_a;
    bool valve_nitrous_b;
    bool valve_nitrogen;
    bool valve_discharge;
    bool solenoid;
} stager_switch_state_t;

// Setup the state machine
bool stager_init(void);
// Get the current state
stager_state_t stager_get_state(void);
// Set the current state
void stager_set_state(stager_state_t state);
// Do a state machine cycle
void stager_tick(void);
// Set the switch states. Uses two bytes from RIU to set.
void stager_set_switches(uint16_t switches);

// Asynchronously request critical states; safe to call from ISRs.
void stager_request_abort_from_isr(void);
void stager_request_error_from_isr(void);

// Thread-safe (interrupt-safe) wrappers for foreground code.
void stager_request_abort(void);
void stager_request_error(void);

// Optional accessors
const stager_switch_state_t* stager_get_switch_state(void);


#endif // STAGER_H