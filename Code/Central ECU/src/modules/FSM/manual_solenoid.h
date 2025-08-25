#ifndef MANUAL_SOLENOID_H
#define MANUAL_SOLENOID_H

typedef enum { 
    PYRO_SAFE = 0, 
    PYRO_ARMED, 
    PYRO_SOLENOID 
} pyro_states_t;

extern pyro_states_t pyro_state;

void manual_pyro_arm(void);
void manual_pyro_disarm(void);
void manual_solenoid_on(void);
void manual_solenoid_off(void);
void pyro_state_decoder(void);

#endif /* MANUAL_SOLENOID_H */