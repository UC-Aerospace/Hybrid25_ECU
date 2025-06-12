#include "app.h"
#include "debug_io.h"
#include "battery_check.h"
#include "ssd1306.h"
#include "rs422.h"
#include "rtc_helper.h"
#include "sd_log.h"

void app_init(void) {
    // Initialize the application
    // HAL_Delay(5000);
    uint32_t uid[3];

    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();
    if (uid[0] == 0x00130041 && uid[1] == 0x5442500C && uid[2] == 0x20373357) {
        // Device is recognized, proceed with initialization
        dbg_printf("Device recognized: UID %08lX %08lX %08lX\n", uid[0], uid[1], uid[2]);
    } else {
        // Unrecognized device, handle error
        dbg_printf("Unrecognized device UID: %08lX %08lX %08lX\n", uid[0], uid[1], uid[2]);
        while (1); // Halt execution
    }

    HAL_ADCEx_Calibration_Start(&hadc1);
    ssd1306_Init();
    rs422_init(&huart1); // Initialize RS422 communication
    batt_check();
    rtc_helper_init();
    sd_log_init();
}

void app_run(void) {
    HAL_ADC_Start(&hadc1); // Start ADC peripheral
    
    while (1) {
        batt_check(); // Check battery status
        rs422_send((uint8_t *)"Hello RS422", 11); // Send a test message over RS422
        HAL_Delay(10000); // Delay for 1 second before the next reading
    }
}