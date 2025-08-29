#ifndef MANUAL_VALVE_H
#define MANUAL_VALVE_H

#include <stdbool.h>

// Lastest set position from servo feedback
struct ServoFeedback {
    bool servoPosCommandedVent;
    bool servoPosCommandedNitrogen;
    bool servoPosCommandedNosA;
    bool servoPosCommandedNosB;
    bool initialised;
};

// Declare it here (no storage, just a declaration)
extern struct ServoFeedback servo_feedback;


typedef enum { 
    VALVE_DISARMED = 0, 
    VALVE_ARMED, 
    VALVE_ACTIVE 
} valve_states_t;

extern valve_states_t valve_state;

void manual_valve_send_arm(void);
void manual_valve_send_disarm(void);
void valve_state_decoder(void);
void valve_set_servo_feedback_position(bool servoSetPosition[4]);
// bool valve_get_servo_position(void);

#endif /* MANUAL_VALVE_H */