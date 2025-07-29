#include "servo.h"
#include "adc.h"

static uint16_t servoPositions[4];

// Servo Channel 1
Servo servoVent = {
    .tim = &htim3,
    .channel = TIM_CHANNEL_1,
    .enabled = false,
    .safePosition = CRACK,
    .targetPosition = CRACK,
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

Servo* servoByIndex[4] = {
    &servoVent,
    &servoNitrogen,
    &servoNitrousA,
    &servoNitrousB
};

void servo_init(void) {
    update_positions(); // Initialize servo positions from ADC
    servo_arm(&servoVent);
    servo_arm(&servoNitrogen);
    servo_arm(&servoNitrousA);
    servo_arm(&servoNitrousB);
}

void update_positions() {
    // Get the latest servo positions from the ADC
    getServoPositions(servoPositions);

    // Update the servo current positions based on the ADC values
    servoVent.currentPosition = servoPositions[0];
    servoNitrogen.currentPosition = servoPositions[1];
    servoNitrousA.currentPosition = servoPositions[2];
    servoNitrousB.currentPosition = servoPositions[3];
}

void servo_arm(Servo *servo) {
    // Enable the servo
    servo->enabled = true;

    // Set the servo to its safe position
    servo_set_position(servo, servo->safePosition);

    // Enable the PWM output for the servo
    HAL_TIM_PWM_Start(servo->tim, servo->channel);
}

void servo_disarm(Servo *servo) {
    // Disable the servo
    servo->enabled = false;

    // Disable the PWM output for the servo
    HAL_TIM_PWM_Stop(servo->tim, servo->channel);
}

void servo_set_safe(Servo *servo) {
    // Set the servo to its safe position
    servo_set_position(servo, servo->safePosition);
}

void servo_set_position(Servo *servo, ServoPosition position) {
    // Set the servo target position
    servo->targetPosition = position;

    // Set the current position assuming the servo has moved if enabled
    // Remove this later with actual ADC feedback
    if (servo->enabled) {
        servo->currentPosition = position;
    }

    // Convert position to angle
    uint16_t angle = position;

    // Set the servo angle
    servo_set_angle(servo, angle);
}

// Sets position of the servo as a fraction of 1000 steps (0 to 1000)
void servo_set_angle(Servo *servo, uint16_t angle) {
    // Set the servo angle
    if (angle > 1000) {
        angle = 1000; // Maximum position
    }
    servo->targetPosition = angle;

    // Update the PWM duty cycle based on the angle
    uint16_t pulseTicks = angle + 250;

    // Assuming the timer is configured for PWM mode and the pulse width is set in ticks
    __HAL_TIM_SET_COMPARE(servo->tim, servo->channel, pulseTicks);
}