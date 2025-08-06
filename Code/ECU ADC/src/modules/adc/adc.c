#include "adc.h"
#include "debug_io.h"

static uint16_t adc_buffer[ADC_DOUBLE_BUFFER_SIZE][ADC_NUMBER_CHANNELS];
static bool buffer_write_first_half = true;

void process_adc_data(uint16_t (*data)[ADC_NUMBER_CHANNELS]);

void adc_start()
{
    // Start the ADC
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, ADC_DOUBLE_BUFFER_SIZE * ADC_NUMBER_CHANNELS);
    // Start Timer
    HAL_TIM_Base_Start(&htim15);
}

void adc_stop()
{
    // Stop the ADC
    HAL_ADC_Stop_DMA(&hadc1);
    // Stop Timer
    HAL_TIM_Base_Stop(&htim15);
}

uint16_t adc_get_batt_voltage()
{
    if (buffer_write_first_half) {
        return (uint16_t)(adc_buffer[ADC_DOUBLE_BUFFER_SIZE / 2][ADC_CHANNEL_V2S] * ADC_2S_FACTOR); // Return V2S from second half
    } else {
        return (uint16_t)(adc_buffer[0][ADC_CHANNEL_V2S] * ADC_2S_FACTOR); // Return V2S from first half
    }
}

uint16_t adc_get_NTC_temp()
{
    float miliVolts;
    if (buffer_write_first_half) {
        miliVolts = adc_buffer[ADC_DOUBLE_BUFFER_SIZE / 2][ADC_CHANNEL_CJT] * ADC_mV_FACTOR;
    } else {
        miliVolts = adc_buffer[0][ADC_CHANNEL_CJT] * ADC_mV_FACTOR;
    }
    // Convert mV to temperature in 100*Celsius
    return (uint16_t)(5604.29 - 1.8764 * miliVolts);

}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1) {
        // First half of buffer is ready: adc_buffer[0] to adc_buffer[5]
        process_adc_data(adc_buffer);
        buffer_write_first_half = false;
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1) {
        // Second half of buffer is ready: adc_buffer[6] to adc_buffer[11]
        process_adc_data(&adc_buffer[ADC_DOUBLE_BUFFER_SIZE / 2]);
        buffer_write_first_half = true;
    }
}

void process_adc_data(uint16_t (*data)[ADC_NUMBER_CHANNELS])
{
    uint16_t MIPA_A_Frame[ADC_DOUBLE_BUFFER_SIZE / 2 + 1];
    uint16_t MIPA_B_Frame[ADC_DOUBLE_BUFFER_SIZE / 2 + 1];
    for (uint8_t i = 0; i < ADC_DOUBLE_BUFFER_SIZE / 2; i++) {
        MIPA_A_Frame[i] = data[i][0];
        MIPA_B_Frame[i] = data[i][1];
    }

    MIPA_A_Frame[ADC_DOUBLE_BUFFER_SIZE / 2] = data[0][3]; // Add REF5V to end of MIPA_A_Frame
    MIPA_B_Frame[ADC_DOUBLE_BUFFER_SIZE / 2] = data[0][3]; // And MIPA_B_Frame

    // TODO: TX both frames over CAN
    
}