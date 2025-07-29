#include "app.h"
#include "debug_io.h"
#include "battery_check.h"
#include "ssd1306.h"
#include "rs422.h"
#include "rtc_helper.h"
#include "sd_log.h"
#include "spicy.h"
#include "ssd1306_fonts.h"
#include "can_handler.h"

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
        while (1); // Halt execution
    }

    HAL_ADCEx_Calibration_Start(&hadc1);
    ssd1306_Init();
    /*
    bool rs422_ok = rs422_init(&huart1); // Initialize RS422 communication
    if (!rs422_ok) {
        dbg_printf("RS422 initialization failed!\r\n");
    } else {
        dbg_printf("RS422 initialization successful\r\n");
    }
    */

    char receive_buffer[256];
    int received = dbg_recv(receive_buffer, sizeof(receive_buffer));
    if (received > 0) {
        rtc_helper_set_from_string(receive_buffer);
    }

    batt_check();
    sd_log_init();
    can_init(); // Initialize CAN peripheral
    sd_log_write(SD_LOG_INFO, "ECU initialized");
}

void app_run(void) {
    HAL_ADC_Start(&hadc1); // Start ADC peripheral
    HAL_Delay(1000);
    
    char test_str[16];
    while (1) {
        batt_check(); // Check battery status
        dbg_printf("Supply servo position from 0 to 20\r\n");
        dbg_recv(test_str, sizeof(test_str)); // Receive debug input
        char* end;
        uint8_t num = strtol(test_str, &end, 10);

        CAN_ID id = {
            .priority = CAN_PRIORITY_COMMAND,
            .nodeType = CAN_NODE_TYPE_SERVO,
            .nodeAddr = CAN_NODE_ADDR_SERVO,
            .frameType = CAN_TYPE_COMMAND
        };

        uint8_t frame[8] = {0};

        frame[0] = 17;

        uint8_t servo = 1;
        uint8_t manual = 1;

        if (num >= 20) {
            num = 20;
        }

        frame[1] = servo << 6 | manual << 5 | num;

        can_send_command(id, frame, sizeof(frame));

        HAL_GPIO_TogglePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin);
        HAL_Delay(1300); // Delay for 1 second before the next reading
    }
}