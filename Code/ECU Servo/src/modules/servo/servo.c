#include "servo.h"
#include "adc.h"
#include "debug_io.h"

static ServoQueue servoQueue = {
    .items = {0},
    .head = 0,
    .tail = 0
};
static uint16_t servoPositions[4];

static void servo_set_angle(Servo *servo, uint16_t angle);

// Servo Channel 1
Servo servoVent = {
    .tim = &htim3,
    .channel = TIM_CHANNEL_1,
    .state = SERVO_STATE_DISARMED,
    .safePosition = CRACK,
    .targetPosition = CRACK,
    .currentPosition = 0
};

// Servo Channel 2
Servo servoNitrogen = {
    .tim = &htim3,
    .channel = TIM_CHANNEL_2,
    .state = SERVO_STATE_DISARMED,
    .safePosition = CLOSE,
    .targetPosition = CLOSE,
    .currentPosition = 0
};

// Servo Channel 3
Servo servoNitrousA = {
    .tim = &htim1,
    .channel = TIM_CHANNEL_1,
    .state = SERVO_STATE_DISARMED,
    .safePosition = CLOSE,
    .targetPosition = CLOSE,
    .currentPosition = 0
};

// Servo Channel 4
Servo servoNitrousB = {
    .tim = &htim15,
    .channel = TIM_CHANNEL_1,
    .state = SERVO_STATE_DISARMED,
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
    servo_update_positions(); // Initialize servo positions from ADC
}

void servo_update_positions() {
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
    servo->state = SERVO_STATE_ARMED;
}

void servo_disarm(Servo *servo) {
    // Disable the servo
    servo->state = SERVO_STATE_DISARMED;
}

// Sets the position of the servo, will queue if armed or moving
void servo_set_position(Servo *servo, ServoPosition position) {
    if (servo->state == SERVO_STATE_ERROR) {
        dbg_printf("Error: Servo is in error state, cannot set position.\r\n");
        return;
    }

    // Set the target position
    servo->targetPosition = position;
    if (servo->state == SERVO_STATE_ARMED || servo->state == SERVO_STATE_MOVING) {
        servo_queue_position(servo, position);
    }
}

bool servo_queue_start() 
{
    // Process the servo queue
    if (servoQueue.head != servoQueue.tail) {
        ServoQueueItem item = servoQueue.items[servoQueue.head];

        // Set the servo position
        servo_set_angle(item.servo, item.position);
        item.servo->state = SERVO_STATE_MOVING; // Set state to moving
        HAL_TIM_PWM_Start(item.servo->tim, item.servo->channel);

        return true; // Successfully processed an item
    }
    return false; // No items to process
}

bool servo_queue_check_complete()
{
    servo_update_positions(); // Update current positions from ADC
    if (servoQueue.head != servoQueue.tail) {
        ServoQueueItem *item = &servoQueue.items[servoQueue.head];
        uint8_t coarsePosition = item->servo->currentPosition; // Placeholder, do the maths later to convert into same format.
        if (coarsePosition == item->position) {
            return true; // Item is complete
        }
    }
    return false; // No complete items
}

void servo_queue_complete(bool wasSuccess)
{
    // Mark the current item as complete
    if (servoQueue.head != servoQueue.tail) {
        ServoQueueItem *item = &servoQueue.items[servoQueue.head];
        HAL_TIM_PWM_Stop(item->servo->tim, item->servo->channel);
        if (wasSuccess) {
            item->servo->state = SERVO_STATE_ARMED; // Set back to armed state
        } else {
            item->servo->state = SERVO_STATE_ERROR; // Set to error state if failed
        }
        servoQueue.head = (servoQueue.head + 1) % 4; // Move head forward
    }
}

void servo_queue_position(Servo *servo, ServoPosition position) 
{
    if (servo->state == SERVO_STATE_ERROR) {
        dbg_printf("Error: Servo is in error state, cannot queue position.\r\n");
        return;
    }

    // Add to circular queue
    if ((servoQueue.tail + 1) % 4 == servoQueue.head) {
        dbg_printf("Error: Servo queue is full, cannot queue position.\r\n");
        return;
    }

    servoQueue.items[servoQueue.tail].servo = servo;
    servoQueue.items[servoQueue.tail].position = position;
    servoQueue.tail = (servoQueue.tail + 1) % 4;
}

// Sets position of the servo as a fraction of 1000 steps (0 to 1000)
static void servo_set_angle(Servo *servo, uint16_t angle) {
    // Set the servo angle
    if (angle > 1000) {
        dbg_printf("Error: Angle %d exceeds maximum of 1000.\r\n", angle);
        return;
    }
    servo->targetPosition = angle;

    // Update the PWM duty cycle based on the angle
    uint16_t pulseTicks = angle + 250;

    // Assuming the timer is configured for PWM mode and the pulse width is set in ticks
    __HAL_TIM_SET_COMPARE(servo->tim, servo->channel, pulseTicks);
}