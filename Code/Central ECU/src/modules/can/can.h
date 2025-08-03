#ifndef CAN_H
#define CAN_H

#include "stm32g0xx_hal.h"
#include "peripherals.h"
#include "config.h"
#include <stdbool.h>
#include "frames.h"

void can_init(void);
void can_test_send(void);
void can_send_command(CAN_ID id, uint8_t *data, uint8_t length);

#endif // CAN_H