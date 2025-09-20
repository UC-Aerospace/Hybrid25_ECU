#include "battery_check.h"
#include "debug_io.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "rs422_handler.h"

ADC_ChannelConfTypeDef ADC_6S_Config = {
    .Channel = ADC_CHANNEL_0,
    .Rank = ADC_REGULAR_RANK_1,
    .SamplingTime = ADC_SAMPLETIME_3CYCLES_5
};

ADC_ChannelConfTypeDef ADC_2S_Config = {
    .Channel = ADC_CHANNEL_1,
    .Rank = ADC_REGULAR_RANK_1,
    .SamplingTime = ADC_SAMPLETIME_3CYCLES_5
};

typedef struct {
    float voltage; // per-cell voltage in V
    uint8_t soc;   // corresponding SoC in percent
} OCVPoint;

// Also worth noting the cells we have are LiHV, so 4.35V max charge. This should be close enough though.
static const OCVPoint ocv_table[] = {
    {4.20, 100}, {4.14, 95}, {4.09, 90}, {4.05, 85}, {4.01, 80},
    {3.98, 75},  {3.95, 70}, {3.92, 65}, {3.89, 60}, {3.87, 55},
    {3.85, 50},  {3.83, 45}, {3.81, 40}, {3.79, 35}, {3.77, 30},
    {3.75, 25},  {3.73, 20}, {3.71, 15}, {3.68, 10}, {3.64, 5},
    {3.40,  0}
};

#define OCV_TABLE_SIZE (sizeof(ocv_table)/sizeof(ocv_table[0]))

BatteryStatus batt_get_volt(void) {
    BatteryStatus status = {0};

    HAL_ADC_ConfigChannel(&hadc1, &ADC_6S_Config);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    status.voltage_6s = (uint16_t)(HAL_ADC_GetValue(&hadc1) * ADC_6S_FACTOR);

    HAL_ADC_ConfigChannel(&hadc1, &ADC_2S_Config);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    status.voltage_2s = (uint16_t)(HAL_ADC_GetValue(&hadc1) * ADC_2S_FACTOR);

    return status;
}

uint8_t batt_volt_to_soc(uint16_t voltage_mV, uint8_t cell_count) {
    if (cell_count == 0) return 0; // prevent divide-by-zero

    float per_cell_voltage = voltage_mV / (1000.0f * cell_count);

    // Clamp above max
    if (per_cell_voltage >= ocv_table[0].voltage) return 100;
    // Clamp below min
    if (per_cell_voltage <= ocv_table[OCV_TABLE_SIZE - 1].voltage) return 0;

    // Find interval and interpolate
    for (uint8_t i = 0; i < OCV_TABLE_SIZE - 1; i++) {
        float v1 = ocv_table[i].voltage;
        float v2 = ocv_table[i + 1].voltage;
        uint8_t soc1 = ocv_table[i].soc;
        uint8_t soc2 = ocv_table[i + 1].soc;

        if (per_cell_voltage <= v1 && per_cell_voltage >= v2) {
            float fraction = (per_cell_voltage - v2) / (v1 - v2);
            return soc2 + (uint8_t)((soc1 - soc2) * fraction + 0.5f); // round to nearest
        }
    }

    return 0; // fallback (should never be reached)
}

void batt_check(void) {
    static uint8_t serial_limiter = 10; // Limit serial output to every 10th call
    BatteryStatus status = batt_get_volt();
    char buffer[16]; // Buffer to hold the formatted string
    // Calculate 6S battery percentage as an integer
    uint8_t percentage_6s = batt_volt_to_soc(status.voltage_6s, 6);
    if (percentage_6s > 100) percentage_6s = 100; // Clamp to 100%
    ssd1306_SetCursor(0, 0);
    sprintf(buffer, "6S : %d%%   ", percentage_6s);
    ssd1306_WriteString(buffer, Font_16x15, White);

    // Calculate 2S battery percentage as an integer
    uint8_t percentage_2s = batt_volt_to_soc(status.voltage_2s, 2);
    if (percentage_2s > 100) percentage_2s = 100; // Clamp to 100%
    ssd1306_SetCursor(0, 16);
    sprintf(buffer, "2S : %d%%   ", percentage_2s);
    ssd1306_WriteString(buffer, Font_16x15, White);

    if (serial_limiter++ % 10 == 0) {
        if (percentage_2s <= 20) {
            dbg_printf("2S Battery: %d%% (WARN)\r\n", percentage_2s);
        } else {
            dbg_printf("2S Battery: %d%%\r\n", percentage_2s);
        }
        if (percentage_6s <= 20) {
            dbg_printf("6S Battery: %d%% (WARN)\r\n", percentage_6s);
        } else {
            dbg_printf("6S Battery: %d%%\r\n", percentage_6s);
        }
    }

    rs422_send_battery_state(percentage_2s, percentage_6s);

    ssd1306_UpdateScreen();
}