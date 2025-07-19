#ifndef SERVO_H
#define SERVO_H

#include "stm32g0xx_hal.h"
#include "peripherals.h"
#include "config.h"

typedef enum {
    CLOSE = 250,
    OPEN = 750
} ServoPosition;

typedef struct {
    TIM_HandleTypeDef *tim;
    uint32_t channel;
    bool enabled = false;
    ServoPosition safePosition;
    ServoPosition targetPosition;
    uint16_t currentPosition;
} Servo;

void servo_init(void);
void servo_set_angle(Servo *servo, uint16_t angle);


#endif