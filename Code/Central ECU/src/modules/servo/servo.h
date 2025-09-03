#ifndef SERVO_H
#define SERVO_H

#include "stm32g0xx_hal.h"
#include <stdbool.h>

typedef enum {
    SERVO_POSITION_CLOSE = 0,
    SERVO_POSITION_OPEN,
    SERVO_POSITION_VENT,
    SERVO_POSITION_SAFE
} servo_positions_t;

typedef enum {
    SERVO_DISARMED = 0,
    SERVO_ARMED,
    SERVO_MOVING,
    SERVO_ERROR
} servo_state_t;

typedef enum {
    VALVE_VENT = 0,
    VALVE_NITROGEN,
    VALVE_NOS_A,
    VALVE_NOS_B
} valve_index_t;

typedef struct {
    servo_positions_t setPos;
    servo_state_t state;
    bool atSetPos;
    uint8_t currentPos; // 0-20
} servo_feedback_t;

void servo_update(servo_feedback_t feedback[4]);
void servo_get_states(servo_feedback_t feedback[4]);
void servo_arm_all(void);
void servo_disarm_all(void);
void servo_set_position(uint8_t valve, servo_positions_t position);
void servo_print_current_state(void);

bool servo_helper_check_all_closed(void);

#endif // SERVO_H