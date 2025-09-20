#include "heartbeat.h"
#include "debug_io.h"
#include "main_FSM.h"

RemoteHeartbeat_t heartbeat[MAX_COUNT] = {0};

// VERIFY: All of this

void heartbeat_reload(uint8_t BOARD_ID) 
{
    // Check if BOARD_ID is valid
    if (BOARD_ID >= MAX_COUNT) {
        dbg_printf("HRT_BT: Invalid BOARD_ID %d for heartbeat reload\n", BOARD_ID);
        return;
    }

    // Clear the heartbeat for the specified board
    heartbeat[BOARD_ID].is_active = true;
    heartbeat[BOARD_ID].cleared = true;

    // Check if timer is running, else start it
    if (HAL_TIM_Base_GetState(&htim14) != HAL_TIM_STATE_READY) {
        if (HAL_TIM_Base_Start_IT(&htim14) != HAL_OK) {
            dbg_printf("HRT_BT: Failed to start heartbeat timer for board %d\n", BOARD_ID);
            return;
        }
    }
}

bool heartbeat_all_started(void) {
    return (heartbeat[BOARD_ID_RIU].is_active && heartbeat[BOARD_ID_SERVO].is_active && heartbeat[BOARD_ID_ADC_A].is_active);
}

uint8_t get_heartbeat_status(void) {
    uint8_t status = 0;
    for (uint8_t i = 0; i < MAX_COUNT; i++) {
        if (heartbeat[i].is_active) {
            status |= (1 << i);
        }
    }
    return status;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM14) {
        for (uint8_t i = 0; i < MAX_COUNT; i++) {
            if (heartbeat[i].is_active) {
                if (heartbeat[i].cleared) {
                    heartbeat[i].cleared = false;  // Reset clear flag
                } else {
                    // If not cleared, mark as inactive
                    heartbeat[i].is_active = false;
                    dbg_printf("HRT_BT: Heartbeat for board %d is inactive\n", i);

                    #ifndef TEST_MODE
                    fsm_raise_error(ECU_ERROR_HEARTBEAT_LOST);
                    #endif
                }
            }
        }
    }
}