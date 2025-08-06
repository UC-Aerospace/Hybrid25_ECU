#include "app.h"
#include "debug_io.h"
#include "rtc_helper.h"
#include "ads124_handler.h"
#include "adc.h"
#include "frames.h"

uint8_t BOARD_ID = 0;

void app_init(void) {
    // Initialize the application
    uint32_t uid[3];

    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();
    if (uid[0] == 0x00130041 && uid[1] == 0x5442500C && uid[2] == 0x20373357) {
        // Device is recognized, proceed with initialization
        dbg_printf("Device recognized: UID %08lX %08lX %08lX\r\n", uid[0], uid[1], uid[2]);
        BOARD_ID = CAN_NODE_ADDR_ADC_1; // ADC A
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
    adc_start();
    while (1) {
        int32_t adc_value;
        uint8_t status;

        int16_t temperature = adc_get_NTC_temp();
        dbg_printf("NTC Temperature: %d C\r\n", temperature);

        HAL_GPIO_TogglePin(LED_IND_STATUS_GPIO_Port, LED_IND_STATUS_Pin);
        HAL_Delay(100); // Delay for 100 ms before the next reading
    }
}