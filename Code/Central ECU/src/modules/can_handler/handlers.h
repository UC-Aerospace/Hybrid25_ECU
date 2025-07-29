#ifndef HANDLERS_H
#define HANDLERS_H

#include "stm32g0xx_hal.h"
#include "frames.h"
#include "can_handler.h"

void handle_error_warning(CAN_ErrorWarningFrame* frame, CAN_ID id, uint8_t dataLength);
void handle_command(CAN_CommandFrame* frame, CAN_ID id, uint8_t dataLength);
void handle_servo_pos(CAN_ServoFrame* frame, CAN_ID id, uint8_t dataLength);
void handle_adc_data(CAN_ADCFrame* frame, CAN_ID id, uint8_t dataLength);
void handle_status(CAN_StatusFrame* frame, CAN_ID id, uint8_t dataLength);

// Command enum
typedef enum {
    CAN_CMD_UPDATE_STATE = 0b0000,
    CAN_CMD_SERVO_ARM = 0b0001,
    CAN_CMD_SERVO_MOVE = 0b0010
    // Add more commands as needed
} CommandType;

#endif // HANDLERS_H