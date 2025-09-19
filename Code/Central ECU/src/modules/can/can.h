#ifndef CAN_H
#define CAN_H

#include <stdbool.h>
#include "stm32g0xx_hal.h"
#include "peripherals.h"
#include "config.h"

#include "frames.h"
// Command enum used for CAN command frames
typedef enum {
    CAN_CMD_SET_STATE = 0b0000,
    CAN_CMD_SET_SERVO_ARM = 0b0001,
    CAN_CMD_SET_SERVO_POS = 0b0010,
    CAN_CMD_GET_SERVO_POS = 0b0011,
    CAN_CMD_GET_VOLTAGE = 0b0100,
    CAN_CMD_RESTART_MCU = 0b0101,
    CAN_CMD_SET_SENSOR_RATE = 0b0110,
    CAN_CMD_SET_SENSOR_STATE = 0b0111
} CommandType;

#include "filters.h"

void can_init(void);
bool can_send_error_warning(CAN_NodeType nodeType, CAN_NodeAddr nodeAddr, CAN_ErrorAction action, uint8_t errorCode);
bool can_send_command(CAN_NodeType nodeType, CAN_NodeAddr nodeAddr, CommandType commandType, uint8_t command);
bool can_send_status(CAN_NodeType nodeType, CAN_NodeAddr nodeAddr, uint8_t status, uint8_t substatus);
bool can_send_servo_position(CAN_NodeType nodeType, CAN_NodeAddr nodeAddr, uint8_t connected, uint8_t set_position[4], uint8_t current_position[4]);
bool can_send_heartbeat(CAN_NodeType nodeType, CAN_NodeAddr nodeAddr);
bool can_send_data(uint8_t sensorID, uint8_t *data, uint8_t length, uint32_t timestamp);

// Service routine to flush software TX queue (call ~ every 1ms)
void can_service_tx_queue(void);




#endif // CAN_H