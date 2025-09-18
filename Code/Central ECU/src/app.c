#include "app.h"
#include "debug_io.h"
#include "battery_check.h"
#include "ssd1306.h"
#include "rs422.h"
#include "rtc_helper.h"
#include "sd_log.h"
#include "spicy.h"
#include "ssd1306_fonts.h"
#include "can_handlers.h"
#include "rs422_handler.h"
#include "rs422.h"
#include "crc.h"
#include "test_servo.h"
#include "main_FSM.h"

uint8_t BOARD_ID = 0;

void setup_panic(uint8_t err_code)
{
    // Initialize panic mode
    dbg_printf("Panic mode initialized\n");
    while (1) {
        for (uint8_t i = 0; i < err_code; i++) {
            HAL_GPIO_WritePin(LED_IND_STATUS_GPIO_Port, LED_IND_STATUS_Pin, GPIO_PIN_SET); // Toggle status LED
            HAL_Delay(200);
            HAL_GPIO_WritePin(LED_IND_STATUS_GPIO_Port, LED_IND_STATUS_Pin, GPIO_PIN_RESET);
            HAL_Delay(200);
        }
        //HAL_GPIO_WritePin(LED_IND_ERROR_GPIO_Port, LED_IND_ERROR_Pin, GPIO_PIN_RESET); // Set error LED
        HAL_Delay(2000);
    }
}

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
        setup_panic(2);
    }

    HAL_ADCEx_Calibration_Start(&hadc1);
    ssd1306_Init();
    batt_check();
    HAL_Delay(100); // Wait for battery check to stabilize
    // TODO: increase the size of sd log preallocation for actual operation.
    if (!sd_log_init(2, 10)) { // Log 2MB, Sensor 10MB
        dbg_printf("INIT: Failed to initialize SD log\n");
        setup_panic(3);
    }
    can_init(); // Initialize CAN peripheral
    if (!sd_log_write(SD_LOG_INFO, "ECU initialized")) {
        dbg_printf("INIT: Failed to write SD log\n");
        setup_panic(4);
    }
    if (!rs422_init(&huart1)) { // Initialize RS422 with DMA
        dbg_printf("INIT: Failed to initialize RS422\n");
        setup_panic(5);
    }
    crc16_init(); // Initialize CRC peripheral
    if (HAL_ADC_Start(&hadc1) != HAL_OK) { // Start ADC peripheral
        dbg_printf("INIT: Failed to start ADC\n");
        setup_panic(6);
    }
    test_servo_init();
}

void task_poll_can_handlers(void) {
    // Poll CAN handlers for incoming messages
    can_handler_poll();
}

void task_poll_rs422(void) {
    // Poll RS422 for incoming messages
    rs422_handler_rx_poll();
}

void task_flush_sd_card(void) {
    // Flush SD card to ensure all data is written
    sd_log_service(50); // Allow upto 50ms to service SD card, else will get it on next tick.
}

void task_poll_battery(void) {
    // Poll battery status
    batt_check();
}

void task_send_heartbeat(void) 
{
    // Send a heartbeat message over CAN
    // TODO: Also feed watchdog here
    can_send_heartbeat(CAN_NODE_TYPE_BROADCAST, CAN_NODE_ADDR_BROADCAST);
    rs422_send_heartbeat();
    HAL_GPIO_TogglePin(LED_IND_STATUS_GPIO_Port, LED_IND_STATUS_Pin);
}

void app_run(void) {
    // Define tasks
    Task tasks[] = {
        {0, 20,  task_poll_can_handlers},     // Poll CAN handlers every 20 ms
        {0, 100, task_poll_rs422},            // Poll RS422 every 100 ms
        {0, 1000, task_poll_battery},         // Poll battery every 1000 ms
        {0, 100, test_servo_poll},            // Poll test servo interface //TODO: Remove this in production
        {0, 500, task_flush_sd_card},         // Flush SD card every 500 ms
        {0, 100, fsm_tick},
        {0, 3, can_service_tx_queue},         // Service CAN TX queue every 3 ms
        {0, 400, task_send_heartbeat}         // Send heartbeat every 400 ms
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