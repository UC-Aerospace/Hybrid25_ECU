#ifndef MANUAL_SOLENOID_H
#define MANUAL_SOLENOID_H

#include "main_FSM.h"

typedef enum { 
    PYRO_SAFE = 0, 
    PYRO_ARMED, 
    PYRO_SOLENOID 
} manual_solenoid_state_t;

void manual_solenoid_tick(switch_state_t *switch_snapshot);
void manual_solenoid_set_safe(void);

#endif /* MANUAL_SOLENOID_H */