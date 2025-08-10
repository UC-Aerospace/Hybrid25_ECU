#ifndef HEARTBEAT_H
#define HEARTBEAT_H

#include "stm32g0xx_hal.h"
#include "peripherals.h"
#include "config.h"
#include <stdbool.h>

#define MAX_COUNT 5


typedef struct {
    bool is_active; // Is the heartbeat active
    bool cleared;  // Clear the heartbeat
} RemoteHeartbeat_t;

// Note: BOARD_ID is as in config.h and CAN messages. The RIU is id 0x00
void heartbeat_reload(uint8_t BOARD_ID);

#endif // HEARTBEAT_H