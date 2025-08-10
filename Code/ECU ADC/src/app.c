#include "app.h"
#include "debug_io.h"
#include "rtc_helper.h"
#include "ads124_handler.h"
#include "adc.h"
#include "frames.h"
#include "pte7300.h"
#include "can_handlers.h"
#include "can_buffer.h"

uint8_t BOARD_ID = 0;
PTE7300_HandleTypeDef hpte7300_A;
PTE7300_HandleTypeDef hpte7300_B;
PTE7300_HandleTypeDef hpte7300_C;

uint16_t voltage;
int16_t cjt_temp;
can_buffer_t cjt_buffer;

void setup_panic(uint8_t err_code)
{
    // Initialize panic mode
    __disable_irq();
    dbg_printf("Panic mode initialized\n");
    while (1) {
        for (uint8_t i = 0; i < err_code; i++) {
            HAL_GPIO_TogglePin(LED_IND_ERROR_GPIO_Port, LED_IND_ERROR_Pin); // Toggle error LED
            HAL_Delay(100);
        }
        HAL_Delay(2000);
    }
}

void app_init(void) {

    /*****************
     * Start TIM2 for timebase
     *****************/

    // Set counter to zero
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    
    // Start the timer
    if (HAL_TIM_Base_Start(&htim2) != HAL_OK) {
        HAL_GPIO_WritePin(LED_IND_ERROR_GPIO_Port, LED_IND_ERROR_Pin, GPIO_PIN_SET); // Set error LED
        setup_panic(1);
    }

    /*****************
     * Check UID of MCU
     *****************/

    uint32_t uid[3];

    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();
    if (uid[0] == 0x00170041 && uid[1] == 0x5442500C && uid[2] == 0x20373357) {
        // Device is recognized, proceed with initialization
        dbg_printf("Device recognized: UID %08lX %08lX %08lX\r\n", uid[0], uid[1], uid[2]);
        BOARD_ID = CAN_NODE_ADDR_ADC_1; // ADC A
    } else {
        // Unrecognized device, handle error
        dbg_printf("Unrecognized device UID: %08lX %08lX %08lX\r\n", uid[0], uid[1], uid[2]);
        HAL_GPIO_WritePin(LED_IND_ERROR_GPIO_Port, LED_IND_ERROR_Pin, GPIO_PIN_SET); // Set error LED
        setup_panic(2);
    }

    /****************
     * Initialize peripherals
     ****************/
    
    can_init();

    // Initialise the ADC functionality
    HAL_ADCEx_Calibration_Start(&hadc1);

    // Initialize ADS124S08
    if (ads124_init() != HAL_OK) {
        setup_panic(3);
    }

    // Start conversions
    if (!adc_start()) {
        setup_panic(4);
    }

    // TODO: Enable the PTE7300 Pressure Sensors
    // Initialise the PTE7300 Pressure Sensors
    // bool stat_a = PTE7300_Init(&hpte7300_A, &hi2c1, SID_SENSOR_PT_A);
    // bool stat_b = PTE7300_Init(&hpte7300_B, &hi2c2, SID_SENSOR_PT_B);
    // bool stat_c = PTE7300_Init(&hpte7300_C, &hi2c3, SID_SENSOR_PT_C);

    // if (!stat_a || !stat_b || !stat_c) {
    //     setup_panic(5);
    // }

    can_buffer_init(&cjt_buffer, SID_SENSOR_CJT, 2);
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
    can_buffer_push(&cjt_buffer, cjt_temp);
    // TODO: Rather than TX should convert to a thermocouple voltage and apply as a correction term.
}

void task_sample_pte7300(void) {
    // Sample the PTE7300 pressure sensors
    PTE7300_sample_to_buffer(&hpte7300_A);
    PTE7300_sample_to_buffer(&hpte7300_B);
    PTE7300_sample_to_buffer(&hpte7300_C);
}

void task_poll_can_handlers(void) {
    // Poll CAN handlers for incoming messages
    can_handler_poll();
}

void task_send_heartbeat(void) {
    // Send heartbeat message
    can_send_heartbeat(CAN_NODE_TYPE_CENTRAL, CAN_NODE_ADDR_CENTRAL);
}

void app_run(void) {
    // Define tasks
    Task tasks[] = {
        //{0, 1000, task_update_battery_voltage},    // Battery voltage reading every 1000 ms
        {0, 500, task_update_NTC_temperature},         // Temperature reading every 500 ms
        {0, 500, task_toggle_status_led},         // LED toggle every 500 ms
        //{0, 100, task_sample_pte7300},            // Sample PTE7300 sensors every 100 ms
        {0, 500, task_send_heartbeat},              // Send heartbeat every 500 ms
        {0, 300, task_poll_can_handlers}            // Poll CAN handlers every 300 ms
    };

    while (1) {
        uint32_t now = HAL_GetTick();

        for (int i = 0; i < sizeof(tasks) / sizeof(Task); i++) {
            if (now - tasks[i].last_run_time >= tasks[i].interval) {
                tasks[i].last_run_time = now;
                tasks[i].task_function();
            }
        }
        HAL_Delay(5);
    }
}