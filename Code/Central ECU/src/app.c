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
#include "rs422.h"
#include "crc.h"

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

    // char receive_buffer[256];
    // int received = dbg_recv(receive_buffer, sizeof(receive_buffer));
    // if (received > 0) {
    //     rtc_helper_set_from_string(receive_buffer);
    // }

    batt_check();
    sd_log_init();
    can_init(); // Initialize CAN peripheral
    sd_log_write(SD_LOG_INFO, "ECU initialized");
    rs422_init(&huart1); // Initialize RS422 with DMA
    crc16_init(); // Initialize CRC peripheral
}

void app_run(void) {
    HAL_ADC_Start(&hadc1); // Start ADC peripheral
    HAL_Delay(1000);
    static RS422_RxFrame_t rx_frame;

    while (1) {
        batt_check(); // Check battery status
        char buffer[64];
        
        // Profile RS422 speed
        uint32_t start_time = HAL_GetTick();
        HAL_GPIO_TogglePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin);

        for (uint32_t i = 0; i < 10; i++) {

            memcpy(buffer, &i, sizeof(i)); // Copy loop index to buffer
            rs422_send(buffer, 64, RS422_FRAME_HEARTBEAT); // Send test message over RS422

            while (rs422_get_rx_available() == 0) {
                __NOP();
            }
            while (rs422_get_rx_available() > 0) {
                uint16_t bytes_read = rs422_read(&rx_frame);
                uint16_t combined_data = (rx_frame.data[1] << 8) | rx_frame.data[0];
                dbg_printf("%u %d\r\n", combined_data, bytes_read); // Print combined data[0] and data[1] as uint16
                if (bytes_read == 0) {
                    break;
                }
            }
        }
        HAL_GPIO_TogglePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin);
        uint32_t elapsed_time = HAL_GetTick() - start_time;
        
        dbg_printf("RS422 speed test completed in %lu ms\r\n", elapsed_time);
        
        //HAL_GPIO_TogglePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin);
        HAL_Delay(1300); // Delay for 1 second before the next reading
    }
}