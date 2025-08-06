#include "app.h"
#include "debug_io.h"
#include "rtc_helper.h"
#include "ads124_example.h"
#include "adc.h"

void app_init(void) {
    // Initialize the application
    uint32_t uid[3];

    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();
    if (uid[0] == 0x00130041 && uid[1] == 0x5442500C && uid[2] == 0x20373357) {
        // Device is recognized, proceed with initialization
        dbg_printf("Device recognized: UID %08lX %08lX %08lX\r\n", uid[0], uid[1], uid[2]);
    } else {
        // Unrecognized device, handle error
        dbg_printf("Unrecognized device UID: %08lX %08lX %08lX\r\n", uid[0], uid[1], uid[2]);
        //while (1); // Halt execution
    }

    HAL_ADCEx_Calibration_Start(&hadc1);

    // Initialize ADS124S08
    if (ads124_init() != HAL_OK) {
        Error_Handler();
    }
}

void app_run(void) {
    // Start conversions
    ads124_start_conversion();
    adc_start();
    while (1) {
        int32_t adc_value;
        uint8_t status;
        
        if (ads124_read_conversion(&adc_value, &status) == HAL_OK) {
            // Calculate the temperature in Celsius. A K type thermocouple is attached.
            // The ADS124S08 is using the internal reference and has a gain of 32.
            float temperature = (adc_value * INT_VREF / 16777216.0) * 1000.0; // Convert to mV and then to Celsius
            dbg_printf("ADC Value: %ld, Status: %02X, Temperature: %.2f C\r\n", 
                       adc_value, status, temperature);
        }
        
        HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
        HAL_Delay(100); // Delay for 100 ms before the next reading
    }
}