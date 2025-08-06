#ifndef ADC_H
#define ADC_H

#include <stdbool.h>
#include "stm32g0xx_hal.h"
#include "config.h"
#include "peripherals.h"


#define ADC_mV_FACTOR (3300.0f / 65520.0f) // Given oversampling
#define ADC_NUMBER_CHANNELS 5
#define ADC_DOUBLE_BUFFER_SIZE 28*2

#define MIPA_SAMPLE_DIVISOR 1 // Sample rate divisor


typedef enum {
    ADC_CHANNEL_CJT,
    ADC_CHANNEL_MIPAA,
    ADC_CHANNEL_MIPAB,
    ADC_CHANNEL_REF5V,
    ADC_CHANNEL_V2S
} adc_channel_t;

void adc_start();
void adc_stop();



#endif