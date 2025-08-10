#include "adc.h"
#include "debug_io.h"
#include "can.h"

static uint16_t adc_buffer[ADC_DOUBLE_BUFFER_SIZE][ADC_NUMBER_CHANNELS];
static bool buffer_write_first_half = true;

static void process_adc_data(uint16_t (*data)[ADC_NUMBER_CHANNELS], uint32_t timestamp);

static volatile uint32_t first_half_timestamp = 0;
static volatile uint32_t second_half_timestamp = 0;

bool adc_start()
{
    // Start the ADC
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, ADC_DOUBLE_BUFFER_SIZE * ADC_NUMBER_CHANNELS) != HAL_OK) {
        dbg_printf("Failed to start ADC DMA\n");
        return false;
    }
    // Start Timer
    if (HAL_TIM_Base_Start(&htim15) != HAL_OK) {
        dbg_printf("Failed to start Timer\n");
        return false;
    }
    return true;
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

int16_t adc_get_NTC_temp()
{
    float miliVolts;
    if (buffer_write_first_half) {
        miliVolts = adc_buffer[ADC_DOUBLE_BUFFER_SIZE / 2][ADC_CHANNEL_CJT] * ADC_mV_FACTOR;
    } else {
        miliVolts = adc_buffer[0][ADC_CHANNEL_CJT] * ADC_mV_FACTOR;
    }
    // Convert mV to temperature in 100*Celsius
    return (int16_t)(5604.29 - 1.8764 * miliVolts); // FIXME: Magic numbers

}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1) {
        // First half of buffer is ready: adc_buffer[0] to adc_buffer[5]
        second_half_timestamp = HAL_GetTick()+1; // Store timestamp for first half
        process_adc_data(adc_buffer, first_half_timestamp);
        buffer_write_first_half = false;
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1) {
        // Second half of buffer is ready: adc_buffer[6] to adc_buffer[11]
        first_half_timestamp = HAL_GetTick()+1; // FIXME: Adding 1 here to approximate the start time of the next conversion
        process_adc_data(&adc_buffer[ADC_DOUBLE_BUFFER_SIZE / 2], second_half_timestamp);
        buffer_write_first_half = true;
    }
}

static void process_adc_data(uint16_t (*data)[ADC_NUMBER_CHANNELS], uint32_t timestamp)
{
    uint16_t MIPA_A_Frame[ADC_DOUBLE_BUFFER_SIZE / 2 + 1];
    uint16_t MIPA_B_Frame[ADC_DOUBLE_BUFFER_SIZE / 2 + 1];
    for (uint8_t i = 0; i < ADC_DOUBLE_BUFFER_SIZE / 2; i++) {
        MIPA_A_Frame[i] = data[i][ADC_CHANNEL_MIPAA];
        MIPA_B_Frame[i] = data[i][ADC_CHANNEL_MIPAB];
    }

    MIPA_A_Frame[ADC_DOUBLE_BUFFER_SIZE / 2] = data[0][ADC_CHANNEL_REF5V]; // Add REF5V to end of MIPA_A_Frame
    MIPA_B_Frame[ADC_DOUBLE_BUFFER_SIZE / 2] = data[0][ADC_CHANNEL_REF5V]; // And MIPA_B_Frame

    can_send_data(SID_SENSOR_MIPA_A, (uint8_t*)MIPA_A_Frame, ADC_DOUBLE_BUFFER_SIZE + 2, timestamp); // ADC_DOUBLE_BUFFER_SIZE * 2 (uint16_t) / 2 (half buffer) + 2 (for REF5V)
    can_send_data(SID_SENSOR_MIPA_B, (uint8_t*)MIPA_B_Frame, ADC_DOUBLE_BUFFER_SIZE + 2, timestamp);
}