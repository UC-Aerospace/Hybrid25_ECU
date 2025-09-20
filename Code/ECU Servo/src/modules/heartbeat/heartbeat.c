#include "heartbeat.h"
#include "debug_io.h"
#include "fsm.h"
#include "can.h"
#include "error_def.h"

void heartbeat_reload(void) {
    // Check if timer is running, else start it
    if (HAL_TIM_Base_GetState(&htim14) != HAL_TIM_STATE_READY) {
        if (HAL_TIM_Base_Start_IT(&htim14) != HAL_OK) {
            dbg_printf("Failed to start heartbeat timer.\n");
            return;
        }
        if (fsm_get_state() == STATE_INIT) {
            fsm_dispatch(FSM_EVENT_INIT_DONE);
        }
    }
    __HAL_TIM_SET_COUNTER(&htim14, 0); // Reset the timer counter to 0
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM14) {
        dbg_printf("Heartbeat timeout occurred, no heartbeat received in the last second.\n");
        can_send_error_warning(CAN_NODE_TYPE_CENTRAL, CAN_NODE_ADDR_CENTRAL, CAN_ERROR_ACTION_SHUTDOWN, SERVO_SHUTDOWN_HEARTBEAT_LOST); // Send heartbeat lost error
        fsm_dispatch(FSM_EVENT_HEARTBEAT_TIMEOUT);
    }
}