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
// TODO: This seems pretty bodgy but oh well
bool heartbeat_all_started(void);

#endif // HEARTBEAT_H