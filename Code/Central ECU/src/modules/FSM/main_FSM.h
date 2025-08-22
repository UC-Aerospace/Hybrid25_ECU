#ifndef MAIN_FSM_H
#define MAIN_FSM_H

#include <stdint.h>
#include <stdbool.h>

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
} switch_state_t;

void fsm_set_switch_states(uint16_t switches);

#endif /* MAIN_FSM_H */