#include "servo.h"

// Servo Channel 1
Servo servoVent = {
    .tim = &htim3,
    .channel = TIM_CHANNEL_1,
    .enabled = false,
    .safePosition = OPEN,
    .targetPosition = OPEN,
    .currentPosition = 0
};

// Servo Channel 2
Servo servoNitrogen = {
    .tim = &htim3,
    .channel = TIM_CHANNEL_2,
    .enabled = false,
    .safePosition = CLOSE,
    .targetPosition = CLOSE,
    .currentPosition = 0
};

// Servo Channel 3
Servo servoNitrousA = {
    .tim = &htim1,
    .channel = TIM_CHANNEL_1,
    .enabled = false,
    .safePosition = CLOSE,
    .targetPosition = CLOSE,
    .currentPosition = 0
};

// Servo Channel 4
Servo servoNitrousB = {
    .tim = &htim15,
    .channel = TIM_CHANNEL_1,
    .enabled = false,
    .safePosition = CLOSE,
    .targetPosition = CLOSE,
    .currentPosition = 0
};


void servo_init(void) {
    
}

void servo_set_angle(Servo *servo, uint16_t angle) {
    // Set the servo angle
}