#ifndef ADC_H
#define ADC_H

#include "stm32g0xx_hal.h"
#include "config.h"
#include "peripherals.h"

void adc_init(void);
void adc_get_servo_positions(uint16_t *positions);

#endif