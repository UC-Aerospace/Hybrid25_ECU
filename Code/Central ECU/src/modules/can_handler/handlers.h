#ifndef HANDLERS_H
#define HANDLERS_H

#include "stm32g0xx_hal.h"
#include "frames.h"
#include "can_handler.h"

void handle_error_warning(CAN_ErrorWarningFrame* frame, CAN_ID id, uint8_t dataLength);
void handle_command(CAN_CommandFrame* frame, CAN_ID id, uint8_t dataLength);
void handle_servo_pos(CAN_ServoFrame* frame, CAN_ID id, uint8_t dataLength);
void handle_adc_data(CAN_ADCFrame* frame, CAN_ID id, uint8_t dataLength);

#endif // HANDLERS_H