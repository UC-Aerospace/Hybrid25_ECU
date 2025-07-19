#ifndef CAN_HANDLER_H
#define CAN_HANDLER_H

#include "stm32g0xx_hal.h"
#include "peripherals.h"
#include "config.h"

void can_init(void);
void can_test_send(void);

#endif // CAN_HANDLER_H