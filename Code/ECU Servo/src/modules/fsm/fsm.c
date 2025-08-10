#include "fsm.h"
#include "servo.h"
#include "debug_io.h"

static fsm_state_t currentState = STATE_INIT;
static uint32_t move_start_ms = 0;

void fsm_init(void) {
    currentState = STATE_INIT;
}

void fsm_run(void) {
    switch (currentState) {
        case STATE_INIT:
            // Wait for state update frame from ECU
            // For now immediately move to READY state
            currentState = STATE_READY;
            dbg_printf("FSM initialized, moving to READY state.\r\n");
            break;
        case STATE_READY:
            if (servo_queue_poll()) {
                currentState = STATE_MOVING;
                move_start_ms = HAL_GetTick(); // Record the start time of the movement
            }
            break;
        case STATE_MOVING:
            // Moving state logic
            if (servo_queue_check_complete()) {
                servo_queue_complete(true); // Movement completed successfully
                currentState = STATE_READY; // Return to ready state
                break;
            }
            uint32_t elapsed = HAL_GetTick() - move_start_ms;
            if (elapsed > MAX_MOVE_DURATION) {
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
