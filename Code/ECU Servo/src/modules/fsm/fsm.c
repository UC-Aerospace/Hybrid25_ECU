#include "fsm.h"
#include "servo.h"

static fsm_state_t currentState = STATE_INIT;
uint32_t move_start_tick = 0;

void fsm_init(void) {
    currentState = STATE_INIT;
}

void fsm_run(void) {
    switch (currentState) {
        case STATE_INIT:
            // Wait for state update frame from ECU
            break;
        case STATE_READY:
            bool servo_to_move = servo_queue_start();
            if (servo_to_move) {
                currentState = STATE_MOVING;
                move_start_tick = SysTick->VAL; // Record the start time of the movement
            }
            break;
        case STATE_MOVING:
            // Moving state logic
            uint32_t move_duration = SysTick->VAL - move_start_tick;
            if (servo_queue_check_complete()) {
                servo_queue_complete(true); // Movement completed successfully
                currentState = STATE_READY; // Return to ready state
            }
            if (move_duration > MAX_MOVE_DURATION) {
                servo_queue_complete(false); // Movement failed, timeout
                // TODO: Send a CAN error message
                currentState = STATE_READY; // Return to ready state
            }
            break;
        case STATE_ERROR:
            // Error handling logic
            break;
        default:
            // Handle unexpected state
            currentState = STATE_ERROR;
            break;
    }
}

void fsm_set_state(fsm_state_t state) {
    if (state >= STATE_INIT && state <= STATE_ERROR) {
        currentState = state;
    } else {
        // Handle invalid state
        currentState = STATE_ERROR;
    }
}

fsm_state_t fsm_get_state(void) {
    return currentState;
}
