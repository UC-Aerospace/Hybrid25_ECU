#include "adc.h"

uint16_t adcValues[5];

void startADCSequence()
{
    HAL_ADC_Start(&hadc1);  // Start the ADC
    
    for (int i = 0; i < 5; i++)
    {
        // Wait for conversion to finish
        if (HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY) == HAL_OK)
        {
            adcValues[i] = HAL_ADC_GetValue(&hadc1);
        }
    }

    HAL_ADC_Stop(&hadc1);  // Stop the ADC (optional in continuous mode)
}

void getServoPositions(uint16_t *positions)
{
    for (int i = 0; i < 4; i++)
    {
        positions[i] = adcValues[i];
    }
}

void 