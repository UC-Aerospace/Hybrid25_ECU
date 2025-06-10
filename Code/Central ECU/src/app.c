#include "app.h"
#include "debug_io.h"
#include "battery_check.h"
#include "ssd1306.h"

void app_init(void) {
    // Initialize the application
    // HAL_Delay(5000);
    // uint32_t uid[3];

    // uid[0] = HAL_GetUIDw0();
    // uid[1] = HAL_GetUIDw1();
    // uid[2] = HAL_GetUIDw2();

    // dbg_printf("UID: %08lX %08lX %08lX\n", uid[0], uid[1], uid[2]); // Print the unique device ID to subsequently lock firmware to this device
    // UID = 00130041 5442500C 20373357 on central ECU rev 1 (Board C)
    HAL_ADCEx_Calibration_Start(&hadc1);
    ssd1306_Init();
}

void app_run(void) {
    HAL_ADC_Start(&hadc1); // Start ADC peripheral
    
    while (1) {
        batt_check(); // Check battery status
        HAL_Delay(1000); // Delay for 1 second before the next reading
    }
}