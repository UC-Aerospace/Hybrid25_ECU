#include "adc.h"

static uint16_t adcValues[5];

void adc_init(void) {
    // Initialize ADC peripheral and start calibration
    HAL_ADCEx_Calibration_Start(&hadc1);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adcValues, 5);
}

void adc_get_servo_positions(uint16_t *positions)
{
    // No need to start ADC sequence manually, DMA will handle it
    for (int i = 0; i < 4; i++)
    {
        positions[i] = adcValues[i];
    }
}