#ifndef MAIN_FSM_H
#define MAIN_FSM_H

#include "stm32g0xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    STATE_INIT = 0b0000,
    STATE_READY = 0b0001,
    STATE_SEQUENCER = 0b0010,
    STATE_POST_FIRE = 0b0011,
    STATE_MANUAL_MODE = 0b1000,
    STATE_ABORT = 0b1111
} main_states_t;

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
    bool changed;
} switch_state_t;

void fsm_set_switch_states(uint16_t switches);
void fsm_set_state(main_states_t new_state);
main_states_t fsm_get_state(void);
void fsm_tick(void);
bool both_armed(void);
bool prefire_ok(void);
void outputs_safe(void);
void fsm_raise_error(uint8_t code);
void fsm_set_abort(uint8_t code);
uint8_t fsm_get_error_code(void);

#endif /* MAIN_FSM_H */