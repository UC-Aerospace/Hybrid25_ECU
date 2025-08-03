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
        
        // Using the debug read function get a number and send it over RS422 as an int
        int i = 0;
        int received = dbg_recv(buffer, sizeof(buffer));

        if (received > 0) {
            // Convert received string to integer
            i = atoi(buffer);
            dbg_printf("Sending number: %d\r\n", i);
            memcpy(buffer, &i, sizeof(i)); // Copy loop index to buffer
            rs422_send(buffer, 2, RS422_FRAME_HEARTBEAT); // Send test message over RS422
        } else {
            dbg_printf("No input received, using default value: %d\r\n", i);
        }

        if (rs422_get_rx_available() > 0) {
            uint16_t bytes_read = rs422_read(&rx_frame);
            if (bytes_read > 0) {
                dbg_printf("Received RS422 frame: Type %d, Size %d\r\n", rx_frame.frame_type, rx_frame.size);
                // Process the received frame based on its type
                switch (rx_frame.frame_type) {
                    case RS422_FRAME_HEARTBEAT:
                        dbg_printf("Heartbeat frame received with size %d\r\n", rx_frame.size);
                        break;
                    case RS422_FRAME_ARM_UPDATE:
                        dbg_printf("Arm update frame received with size %d\r\n", rx_frame.size);
                        break;
                    case RS422_FRAME_VALVE_UPDATE:
                        dbg_printf("Valve update frame received with size %d\r\n", rx_frame.size);
                        break;
                    case RS422_FRAME_TIME_SYNC:
                        dbg_printf("Time sync frame received with size %d\r\n", rx_frame.size);
                        break;
                    case RS422_FRAME_PRESSURE:
                        dbg_printf("Pressure frame received with size %d\r\n", rx_frame.size);
                        break;
                    case RS422_FRAME_PRESSURE_TEMPERATURE:
                        dbg_printf("Pressure and temperature frame received with size %d\r\n", rx_frame.size);
                        break;
                    case RS422_FRAME_TEMPERATURE:
                        dbg_printf("Temperature frame received with size %d\r\n", rx_frame.size);
                        break;
                    case RS422_FRAME_LOAD_CELL:
                        dbg_printf("Load cell frame received with size %d\r\n", rx_frame.size);
                        break;
                    case RS422_FRAME_ABORT:
                        dbg_printf("Abort frame received with size %d\r\n", rx_frame.size);
                        break;
                    case RS422_FRAME_WARNING:
                        dbg_printf("Warning frame received with size %d\r\n", rx_frame.size);
                        break;
                    case RS422_FRAME_FIRE:
                        dbg_printf("Fire frame received with size %d\r\n", rx_frame.size);
                        break;
                    default:
                        dbg_printf("Unknown frame type: %d\r\n", rx_frame.frame_type);
                }
            }
        }
        
        
        HAL_GPIO_TogglePin(LED_STATUS_GPIO_Port, LED_STATUS_Pin);
        HAL_Delay(4000); // Delay for 1 second before the next reading
    }
}