#include "app.h"
#include "debug_io.h"
#include "rtc_helper.h"
#include "ads124_handler.h"
#include "adc.h"
#include "frames.h"
#include "pte7300.h"
#include "can_handlers.h"

uint8_t BOARD_ID = 0;
PTE7300_HandleTypeDef hpte7300_A;
PTE7300_HandleTypeDef hpte7300_B;
PTE7300_HandleTypeDef hpte7300_C;

uint16_t voltage;
uint16_t cjt_temp;

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

    // Initialise the PTE7300 Pressure Sensors
    PTE7300_Init(&hpte7300_A, &hi2c1, SID_SENSOR_PT_A);
    PTE7300_Init(&hpte7300_B, &hi2c2, SID_SENSOR_PT_B);
    PTE7300_Init(&hpte7300_C, &hi2c3, SID_SENSOR_PT_C);
}

void task_toggle_status_led(void) {
    // Toggle the status LED
    HAL_GPIO_TogglePin(LED_IND_STATUS_GPIO_Port, LED_IND_STATUS_Pin);
}

void task_update_battery_voltage(void) {
    // Read and update the battery voltage
    voltage = adc_get_batt_voltage();
    dbg_printf("Battery Voltage: %d mV\r\n", voltage);
}

void task_update_NTC_temperature(void) {
    // Read and update the NTC temperature
    cjt_temp = adc_get_NTC_temp();
    dbg_printf("NTC Temperature: %d C\r\n", cjt_temp);
}

void task_sample_pte7300(void) {
    // Sample the PTE7300 pressure sensors
    PTE7300_sample_to_buffer(&hpte7300_A);
    PTE7300_sample_to_buffer(&hpte7300_B);
    PTE7300_sample_to_buffer(&hpte7300_C);
}

void task_poll_can_hanlers(void) {
    // Poll CAN handlers for incoming messages
    can_handler_poll();
}

void app_run(void) {
    // Start conversions
    adc_start();
    
    // Define tasks
    Task tasks[] = {
        {0, 1000, task_update_battery_voltage},    // Battery voltage reading every 1000 ms
        {0, 500, task_update_NTC_temperature},         // Temperature reading every 500 ms
        {0, 500, task_toggle_status_led},         // LED toggle every 500 ms
        {0, 100, task_sample_pte7300},            // Sample PTE7300 sensors every 100 ms
        {0, 300, task_poll_can_hanlers}            // Poll CAN handlers every 300 ms
    };

    while (1) {
        uint32_t now = HAL_GetTick();

        for (int i = 0; i < sizeof(tasks) / sizeof(Task); i++) {
            if (now - tasks[i].last_run_time >= tasks[i].interval) {
                tasks[i].last_run_time = now;
                tasks[i].task_function();
            }
        }
    }
}