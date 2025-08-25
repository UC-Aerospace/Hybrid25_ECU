#ifndef MANUAL_VALVE_H
#define MANUAL_VALVE_H

typedef enum { 
    VALVE_DISARMED = 0, 
    VALVE_ARMED, 
    VALVE_ACTIVE 
} valve_states_t;

extern valve_states_t valve_state;

void manual_valve_send_arm(void);
void manual_valve_send_disarm(void);
void valve_state_decoder(void);

#endif /* MANUAL_VALVE_H */