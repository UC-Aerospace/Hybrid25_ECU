#ifndef ERROR_DEF_H
#define ERROR_DEF_H

#include "config.h"

typedef enum {
    ERROR_NONE = 0,
    GENERIC_CAN_ERROR,
    ECU_ERROR_HEARTBEAT_LOST = (BOARD_ID_ECU << 4) + 1, // 17
    ECU_ERROR_INVALID_STATE,                            // 18
    ECU_ERROR_ARM_FAIL,                                 // 19
    ECU_ERROR_PREFIRE_CHECKS_FAIL,                      // 20
    ECU_ERROR_CAN_TX_FAIL,                              // 21
    ECU_ERROR_SD_DATA_WRITE_FAIL,                       // 22
    ECU_ERROR_RS422_RX_RESTART_FAIL,                    // 23
    ECU_ERROR_CHAMBER_OVERPRESSURE,                     // 24
    ECU_ERROR_CHAMBER_UNDERPRESSURE,                    // 25

    SERVO_WARNING_STARTUP = (BOARD_ID_SERVO << 4) + 0,   // 32
    SERVO_SHUTDOWN_HEARTBEAT_LOST,                       // 33
    SERVO_SHUTDOWN_MOVEMENT_FAIL,                        // 34
    
    ADC_WARNING_STARTUP = (BOARD_ID_ADC_A << 4) + 0,        // 48
    ADC_ERROR_HEARTBEAT_LOST,                               // 49
    ADC_ERROR_FAIL_INIT_ADC,                                // 50
    ADC_ERROR_FAIL_INIT_ADS124,                             // 51
    ADC_ERROR_FAIL_READ_PTE7300,                            // 52
    ADC_ERROR_FAIL_INIT_PTE7300,                            // 53
} error_t;

#endif // ERROR_DEF_H