#include "app.h"
#include "debug_io.h"
#include "can_handler.h"
#include "servo.h"

void app_init(void) {
    uint32_t uid[3];

    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();
    if (uid[0] == 0x00430040 && uid[1] == 0x5442500C && uid[2] == 0x20373357) {
        // Device is recognized, proceed with initialization
        dbg_printf("Device recognized: UID %08lX %08lX %08lX\r\n", uid[0], uid[1], uid[2]);
        BOARD_ID = 0b010;
    } else {
        // Unrecognized device, handle error
        dbg_printf("Unrecognized device UID: %08lX %08lX %08lX\r\n", uid[0], uid[1], uid[2]);
        while (1); // Halt execution
    }

    //HAL_ADCEx_Calibration_Start(&hadc1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    can_init(); // Initialize CAN peripheral
    servo_init(); // Initialize servos
}

void app_run(void) {
    while (1) {
        //can_test_send(); // Send a test CAN message
        HAL_GPIO_TogglePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin);
        HAL_Delay(1300);
    }
    
}