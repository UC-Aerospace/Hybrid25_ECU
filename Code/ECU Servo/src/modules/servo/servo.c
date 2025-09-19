#include "servo.h"
#include "adc.h"
#include "debug_io.h"
#include "can.h"
#include "fsm.h"

static ServoQueue servoQueue = {
    .items = {0},
    .head = 0,
    .tail = 0
};

static uint16_t servoPositions[4];

/************
 * Private functions
 ************/

// Sets the position of the servo as a fraction of 1000 steps (0 to 1000)
static void servo_set_angle(Servo *servo, uint16_t angle);
// Internal helper: remove all queued items for a servo
static void servo_queue_remove_for_servo(Servo *servo);
// Internal helper: queue a position for a servo
static void servo_queue_position(Servo *servo, uint16_t position);

/************
 * Servo instances
 ************/

// Servo Channel 1
Servo servoVent = {
    .tim = &htim3,
    .channel = TIM_CHANNEL_1,
    .state = SERVO_STATE_DISARMED,
    .closePosition = 500,
    .openPosition = 150,
    .targetPosition = 500,
    .currentPosition = 0,
    .nrstPin = GPIO_PIN_1, // NRST pin for servo 1
    .nrstPort = GPIOD // NRST port for servo 1
};

// Servo Channel 2
Servo servoNitrogen = {
    .tim = &htim3,
    .channel = TIM_CHANNEL_2,
    .state = SERVO_STATE_DISARMED,
    .closePosition = 450,
    .openPosition = 0,
    .targetPosition = 450,
    .currentPosition = 0,
    .nrstPin = GPIO_PIN_2, // NRST pin for servo 2
    .nrstPort = GPIOD // NRST port for servo 2
};

// Servo Channel 3
Servo servoNitrousA = {
    .tim = &htim1,
    .channel = TIM_CHANNEL_1,
    .state = SERVO_STATE_DISARMED,
    .closePosition = 550,
    .openPosition = 50,
    .targetPosition = 550,
    .currentPosition = 0,
    .nrstPin = GPIO_PIN_3, // NRST pin for servo 3
    .nrstPort = GPIOD // NRST port for servo 3
};

// Servo Channel 4
Servo servoNitrousB = {
    .tim = &htim15,
    .channel = TIM_CHANNEL_1,
    .state = SERVO_STATE_DISARMED,
    .closePosition = 700,
    .openPosition = 200,
    .targetPosition = 700,
    .currentPosition = 0,
    .nrstPin = GPIO_PIN_4, // NRST pin for servo 4
    .nrstPort = GPIOD // NRST port for servo 4
};

Servo* servoByIndex[4] = {
    &servoVent,
    &servoNitrogen,
    &servoNitrousA,
    &servoNitrousB
};

/************
 * Public functions
 ************/

void servo_init(void) {
    servo_update_positions(); // Initialize servo positions from ADC
    servo_queue_clear();
}

void servo_send_status(void) {
    uint8_t fsm_status = (uint8_t)fsm_get_state();
    uint8_t servo_status = 0;
    for (int i = 3; i >= 0; i--) {
        servo_status |= (servoByIndex[i]->state & 0x03) << (i * 2);
    }
    can_send_status(CAN_NODE_TYPE_CENTRAL, CAN_NODE_ADDR_CENTRAL, fsm_status, servo_status);
}

void servo_send_can_positions(void) 
{
    servo_update_positions();

    uint8_t setPos[4] = {0};
    for (int i = 0; i < 4; i++) {
        uint8_t setposition;
        bool setIsOpen = (servoByIndex[i]->targetPosition == servoByIndex[i]->openPosition);
        bool setIsClose = (servoByIndex[i]->targetPosition == servoByIndex[i]->closePosition);
        if (setIsOpen) {
            setposition = 1; // Open position
        } else if (setIsClose) {
            setposition = 0; // Close position
        } else {
            setposition = 2; // Crack position (not implemented, default to 0)
        }
        setPos[i] = (setposition << 6) | (servoByIndex[i]->targetPosition / 50);
    }
    uint8_t currentPos[4] = {0};
    for (int i = 0; i < 4; i++) {
        bool atPosition = false;
        int16_t delta = (int16_t)servoByIndex[i]->targetPosition - servoByIndex[i]->currentPosition;
        if (delta < 0) delta = -delta;
        if (delta <= SERVO_POSITION_TOLERANCE*3) {
            atPosition = true;
        }
        currentPos[i] = (servoByIndex[i]->state << 6) | (atPosition << 5) | (servoByIndex[i]->currentPosition / 50);
    }
    can_send_servo_position(CAN_NODE_TYPE_CENTRAL, CAN_NODE_ADDR_CENTRAL, 0b1111, setPos, currentPos); // Assume all are connected
}

void servo_arm(Servo *servo) 
{
    // Enable the servo
    servo->state = SERVO_STATE_ARMED;
    servo_send_status();
    HAL_GPIO_WritePin(servo->nrstPort, servo->nrstPin, GPIO_PIN_SET); // Set NRST pin high to enable servo
}

void servo_disarm(Servo *servo) {
    // Disable the servo and stop PWM
    servo->state = SERVO_STATE_DISARMED;
    servo_send_status();
    HAL_TIM_PWM_Stop(servo->tim, servo->channel);
    HAL_GPIO_WritePin(servo->nrstPort, servo->nrstPin, GPIO_PIN_RESET); // Set NRST pin low to reset servo
    // Return to safe target and clear any queued moves for this servo
    // servo->targetPosition = servo->safePosition; // 
    servo_queue_remove_for_servo(servo);
}

void servo_disarm_all(void) {
    for (uint8_t i = 0; i < 4; i++) {
        servo_disarm(servoByIndex[i]);
    }
}

// Sets the position of the servo, will queue if armed or moving
// This can be done externally
void servo_set_position(Servo *servo, uint16_t position) {
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

// Poll the servo queue for the next item to process. Done from FSM
bool servo_queue_poll() 
{
    // Process the servo queue
    if (servoQueue.head != servoQueue.tail) {
        ServoQueueItem item = servoQueue.items[servoQueue.head];

        // Set the servo position
        servo_set_angle(item.servo, item.position);
        // Set state to moving and start the PWM
        item.servo->state = SERVO_STATE_MOVING; 
        HAL_TIM_PWM_Start(item.servo->tim, item.servo->channel);
        servo_send_status();

        return true; // Successfully processed an item
    }
    return false; // No items to process
}

// Done in move mode in the FSM to check if current move completed.
bool servo_queue_check_complete()
{
    if (servoQueue.head != servoQueue.tail) {
        servo_update_positions(); // Update current positions from ADC
        ServoQueueItem *item = &servoQueue.items[servoQueue.head];
        int16_t delta = (int16_t)item->position - item->servo->currentPosition;
        if (delta < 0) delta = -delta;
        if (delta <= SERVO_POSITION_TOLERANCE) {
            return true; // Item is complete
        }
    }
    return false; // No complete items
}

// On completion of the current servo movement move it to ERROR if failed and move to ARMED if successful
void servo_queue_complete(bool wasSuccess)
{
    // Mark the current item as complete
    if (servoQueue.head != servoQueue.tail) {
        ServoQueueItem *item = &servoQueue.items[servoQueue.head];
        HAL_TIM_PWM_Stop(item->servo->tim, item->servo->channel);
        HAL_GPIO_WritePin(item->servo->nrstPort, item->servo->nrstPin, GPIO_PIN_RESET); // Set NRST pin low to reset servo

        if (wasSuccess) {
            item->servo->state = SERVO_STATE_ARMED; // Set back to armed state
        } else {
            item->servo->state = SERVO_STATE_ERROR; // Set to error state if failed
            can_send_error_warning(CAN_NODE_TYPE_CENTRAL, CAN_NODE_ADDR_CENTRAL, CAN_ERROR_ACTION_SHUTDOWN, (CAN_NODE_ADDR_SERVO << 6) | (CAN_ERROR_SERVO_MOVE_FAILED << 3) | servoQueue.head); // Send error warning over CAN
            dbg_printf("Error: Servo number %d movement failed, setting to ERROR state.\r\n", servoQueue.head);
        }
        servo_send_status();
        servoQueue.head = (servoQueue.head + 1) % SERVO_QUEUE_SIZE; // Move head forward
        // Delay for a period to allow the servo to reset
        for (volatile int i = 0; i < 500; i++) {
            __NOP(); // NOP for 50us delay
        }
        HAL_GPIO_WritePin(item->servo->nrstPort, item->servo->nrstPin, GPIO_PIN_SET); // Set NRST pin high to enable servo
    }
}

// Clear the servo queue
void servo_queue_clear() {
    servoQueue.head = 0;
    servoQueue.tail = 0;
}

void servo_queue_clear_for_servo(Servo *servo) {
    servo_queue_remove_for_servo(servo);
}

void servo_update_positions() {
    // Get the latest servo positions from the ADC as uint16_t array
    adc_get_servo_positions(servoPositions);

    // Convert from 12 bit ADC values to 0-1000 servo positions and set current positions
    servoVent.currentPosition = (int16_t)(-0.305944*servoPositions[0] + 1128);
    servoPositions[0] = servoVent.currentPosition; // Update servoPositions array
    servoNitrogen.currentPosition = (int16_t)(-0.299401*servoPositions[1] + 1120);
    servoPositions[1] = servoNitrogen.currentPosition; // Update servoPositions array
    servoNitrousA.currentPosition = (int16_t)(0.286369*servoPositions[2] - 138);
    servoPositions[2] = servoNitrousA.currentPosition; // Update servoPositions array
    servoNitrousB.currentPosition = (int16_t)(0.291206*servoPositions[3] - 136);
    servoPositions[3] = servoNitrousB.currentPosition; // Update servoPositions array
}

/************
 * Private functions
 ************/

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

// Add a position to the servo queue, will queue if armed or moving
static void servo_queue_position(Servo *servo, uint16_t position) 
{
    if (servo->state == SERVO_STATE_ERROR) {
        dbg_printf("Error: Servo is in error state, cannot queue position.\r\n");
        return;
    }
    if (!(servo->state == SERVO_STATE_ARMED || servo->state == SERVO_STATE_MOVING)) {
        dbg_printf("Warn: Servo is not armed, ignoring queued position.\r\n");
        return;
    }

    // Add to circular queue
    if ((servoQueue.tail + 1) % SERVO_QUEUE_SIZE == servoQueue.head) {
        dbg_printf("Error: Servo queue is full, cannot queue position.\r\n");
        return;
    }

    servoQueue.items[servoQueue.tail].servo = servo;
    servoQueue.items[servoQueue.tail].position = position;
    servoQueue.tail = (servoQueue.tail + 1) % SERVO_QUEUE_SIZE;
}

static void servo_queue_remove_for_servo(Servo *servo) {
    if (servoQueue.head == servoQueue.tail) {
        return; // empty
    }
    ServoQueue newQueue = { .items = {0}, .head = 0, .tail = 0 };
    while (servoQueue.head != servoQueue.tail) {
        ServoQueueItem item = servoQueue.items[servoQueue.head];
        servoQueue.head = (servoQueue.head + 1) % SERVO_QUEUE_SIZE;
        if (item.servo != servo) {
            newQueue.items[newQueue.tail] = item;
            newQueue.tail = (newQueue.tail + 1) % SERVO_QUEUE_SIZE;
        }
    }
    servoQueue = newQueue;
}
