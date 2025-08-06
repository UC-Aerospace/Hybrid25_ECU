#ifndef CAN_H
#define CAN_H

#include <stdbool.h>
#include "stm32g0xx_hal.h"
#include "peripherals.h"
#include "config.h"

#include "frames.h"
#include "can_handlers.h"
#include "filters.h"

void can_init(void);
bool can_send_error_warning(CAN_NodeType nodeType, CAN_NodeAddr nodeAddr, CAN_ErrorAction action, uint8_t errorCode);
bool can_send_command(CAN_NodeType nodeType, CAN_NodeAddr nodeAddr, CAN_CMDType commandType, uint8_t command);
bool can_send_status(CAN_NodeType nodeType, CAN_NodeAddr nodeAddr, uint8_t status, uint8_t substatus);
bool can_send_servo_position(CAN_NodeType nodeType, CAN_NodeAddr nodeAddr, uint8_t connected, uint8_t position[4]);
bool can_send_heartbeat(CAN_NodeAddr nodeAddr);
bool can_send_data(uint8_t sensorID, uint8_t *data, uint8_t length);




#endif // CAN_H