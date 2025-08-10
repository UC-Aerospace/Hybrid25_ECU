#ifndef SERVO_H
#define SERVO_H

#include "stm32g0xx_hal.h"
#include "peripherals.h"
#include "config.h"
#include <stdbool.h>

#define SERVO_QUEUE_SIZE 4
// Tolerance (in 0-1000 units) when checking if a servo has reached its target
#define SERVO_POSITION_TOLERANCE 50

// Actual tick values for servo positions. Full rotation is larger than 180 degrees.
// Based on 10000 ticks per 20ms period (50 Hz PWM frequency).
// 250 -> Minimum pulse width (0.5 ms)
// 400 -> 0 degrees
// 750 -> 90 degrees
// 1100 -> 180 degrees
// 1250 -> Maximum pulse width (2.5 ms)

typedef enum {
    CLOSE = 500, // 750 ticks
    OPEN = 150,  // 400 ticks
    CRACK = 300  // 550 ticks
} ServoPosition;

typedef enum {
    SERVO_STATE_DISARMED,
    SERVO_STATE_ARMED,
    SERVO_STATE_MOVING,
    SERVO_STATE_ERROR
} servo_state_t;

typedef struct {
    TIM_HandleTypeDef *tim;
    uint32_t channel;
    servo_state_t state;
    ServoPosition safePosition;
    ServoPosition targetPosition;
    int16_t currentPosition;
    GPIO_TypeDef *nrstPort; // GPIO port for NRST pin
    uint16_t nrstPin;       // GPIO pin for NRST
} Servo;

typedef struct {
    Servo *servo;
    uint16_t position;
} ServoQueueItem;

typedef struct {
    ServoQueueItem items[SERVO_QUEUE_SIZE];
    uint8_t head;
    uint8_t tail;
} ServoQueue;

extern Servo servoVent;
extern Servo servoNitrogen;
extern Servo servoNitrousA;
extern Servo servoNitrousB;
extern Servo* servoByIndex[4];

void servo_init(void);
void servo_arm(Servo *servo);
void servo_disarm(Servo *servo);
// Disarm all servos and stop PWM
void servo_disarm_all(void);
// Processes queued movements one at a time
bool servo_queue_poll(void);
// Completes the current queued item, setting servo state back to armed
void servo_queue_complete(bool wasSuccess);
// Checks if the current queued item is complete
bool servo_queue_check_complete(void);
// Clear the servo queue
void servo_queue_clear(void);
// Remove all queued items for a specific servo
void servo_queue_clear_for_servo(Servo *servo);
// Sets the position of the servo, will queue if armed or moving
void servo_set_position(Servo *servo, ServoPosition position);
// Updates the current positions of the servos from the ADC
void servo_update_positions(void);
// Sends the current servo positions over CAN
void servo_send_can_positions(void);

#endif