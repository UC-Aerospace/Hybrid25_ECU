#include "app.h"
#include "debug_io.h"
#include "can_handlers.h"
#include "servo.h"
#include "fsm.h"
#include "adc.h"
#include "servo.h"

static uint16_t adcValues[4];
uint8_t BOARD_ID = 0;

void setup_panic(uint8_t err_code)
{
    // Initialize panic mode
    __disable_irq();
    dbg_printf("Panic mode initialized\n");
    while (1) {
        for (uint8_t i = 0; i < err_code; i++) {
            HAL_GPIO_WritePin(LED_IND_ERROR_GPIO_Port, LED_IND_ERROR_Pin, GPIO_PIN_SET); // Toggle error LED
            HAL_Delay(200);
            HAL_GPIO_WritePin(LED_IND_ERROR_GPIO_Port, LED_IND_ERROR_Pin, GPIO_PIN_RESET);
            HAL_Delay(200);
        }
        //HAL_GPIO_WritePin(LED_IND_ERROR_GPIO_Port, LED_IND_ERROR_Pin, GPIO_PIN_RESET); // Set error LED
        HAL_Delay(2000);
    }
}

void app_init(void) 
{
    uint32_t uid[3];

    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();
    if (uid[0] == 0x00430040 && uid[1] == 0x5442500C && uid[2] == 0x20373357) {
        // Device is recognized, proceed with initialization
        dbg_printf("Device recognized: UID %08lX %08lX %08lX\r\n", uid[0], uid[1], uid[2]);
        BOARD_ID = CAN_NODE_ADDR_SERVO;
    } else {
        // Unrecognized device, handle error
        dbg_printf("Unrecognized device UID: %08lX %08lX %08lX\r\n", uid[0], uid[1], uid[2]);
        HAL_GPIO_WritePin(LED_IND_ERROR_GPIO_Port, LED_IND_ERROR_Pin, GPIO_PIN_SET); // Set error LED
        setup_panic(1); // Enter panic mode
    }

    adc_init(); // Initialize ADC for servo position reading
    can_init(); // Initialize CAN peripheral
    servo_init(); // Initialize servos
    fsm_init(); // Initialize FSM
}

void task_poll_servo_fsm(void) 
{
    // Poll servo FSM
    fsm_tick();
}

void task_poll_can_handlers(void) 
{
    // Poll CAN handlers for incoming messages
    can_handler_poll();
}

void task_toggle_status_led(void) 
{
    // Toggle the status LED
    //dbg_printf("Toggling status LED\r\n");
    HAL_GPIO_TogglePin(LED_IND_STATUS_GPIO_Port, LED_IND_STATUS_Pin);
}

void task_send_heartbeat(void) 
{
    // Send a heartbeat message over CAN
    can_send_heartbeat(CAN_NODE_TYPE_CENTRAL, CAN_NODE_ADDR_CENTRAL);
}


void app_run(void) 
{
    // Define tasks
    Task tasks[] = {
        {0, 500, task_toggle_status_led},           // LED toggle every 500 ms
        {0, 300, task_poll_can_handlers},            // Poll CAN handlers every 300 ms
        {0, 100, task_poll_servo_fsm},              // Poll servo FSM every 100 ms
        {0, 1000, servo_update_positions},
        {0, 500, servo_send_can_positions},         // Send servo positions every 500 ms
        {0, 500, task_send_heartbeat},               // Send heartbeat every 500 ms
        {0, 1, can_service_tx_queue}                // Service CAN TX queue every 1 ms
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