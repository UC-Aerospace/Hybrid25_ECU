#ifndef SERVO_H
#define SERVO_H

#include "stm32g0xx_hal.h"
#include "peripherals.h"
#include "config.h"
#include <stdbool.h>

// Actual tick values for servo positions. Full rotation is larger than 180 degrees.
// Based on 10000 ticks per 20ms period (50 Hz PWM frequency).
// 250 -> Minimum pulse width (0.5 ms)
// 400 -> 0 degrees
// 750 -> 90 degrees
// 1100 -> 180 degrees
// 1250 -> Maximum pulse width (2.5 ms)

typedef enum {
    CLOSE = 750,
    OPEN = 400,
    CRACK = 550
} ServoPosition;

typedef struct {
    TIM_HandleTypeDef *tim;
    uint32_t channel;
    bool enabled;
    ServoPosition safePosition;
    ServoPosition targetPosition;
    uint16_t currentPosition;
} Servo;

extern Servo servoVent;
extern Servo servoNitrogen;
extern Servo servoNitrousA;
extern Servo servoNitrousB;
extern Servo* servoByIndex[4];

void servo_init(void);
void servo_arm(Servo *servo);
void servo_disarm(Servo *servo);
// Sets position of the servo as a ServoPosition argument
void servo_set_position(Servo *servo, ServoPosition position);
// Takes angle as fraction of 1000 steps (0 to 1000)
void servo_set_angle(Servo *servo, uint16_t angle);
// Updates the current servo positions from ADC
void update_positions(void);
// Sets the servo to its safe position
void servo_set_safe(Servo *servo);


#endif