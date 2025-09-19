#ifndef MANUAL_VALVE_H
#define MANUAL_VALVE_H

#include <stdbool.h>
#include "main_FSM.h"

typedef enum { 
    VALVE_DISARMED = 0, 
    VALVE_ARMED
} manual_valve_state_t;

void manual_valve_tick(switch_state_t *switch_snapshot);
void manual_valve_set_safe(void);

#endif /* MANUAL_VALVE_H */