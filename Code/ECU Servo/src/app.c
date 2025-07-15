#include "app.h"
#include "debug_io.h"

void app_init(void) {
    uint32_t uid[3];

    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();
    //dbg_printf("Device UID: %08lX %08lX %08lX\r\n", uid[0], uid[1], uid[2]);

    //HAL_ADCEx_Calibration_Start(&hadc1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
}

void app_run(void) {
    while (1) {
        HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
        htim3.Instance->CCR1 = 250;  // duty cycle is .5 ms
        HAL_Delay(2000);
        htim3.Instance->CCR1 = 750;  // duty cycle is 1.5 ms
        HAL_Delay(2000);
        htim3.Instance->CCR1 = 1250;  // duty cycle is 2.5 ms
        HAL_Delay(2000);
    }
    
}