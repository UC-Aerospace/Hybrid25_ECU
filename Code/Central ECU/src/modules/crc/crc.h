#ifndef CRC_H
#define CRC_H

#include "stm32g0xx_hal.h"
#include "peripherals.h"

void crc16_init(void);
uint16_t crc16_compute(uint8_t *data, uint32_t length);

#endif // CRC_H