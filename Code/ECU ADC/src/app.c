#include "app.h"
#include "debug_io.h"
#include "rtc_helper.h"
#include "ADS124.h"

static ads124_handle_t ads124_handle;

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
        //while (1); // Halt execution
    }

    HAL_ADCEx_Calibration_Start(&hadc1);
    HAL_ADC_Start(&hadc1); // Start ADC peripheral

}

void ads124_example_init(void)
{
    // Initialize the ADS124 module
    if (ads124_init(&ads124_handle, &hspi1) != HAL_OK) {
        // Handle initialization error
        return;
    }
    
    // Configure Channel 0: AIN0(+) to AIN1(-) differential
    // High gain (64x) for small signals, medium sample rate (100 SPS)
    ads124_configure_channel(&ads124_handle, 0, 
                            ADS124_AIN0, ADS124_AIN1, 
                            ADS124_PGA_GAIN_64, ADS124_DATARATE_100SPS);
    
    // Configure Channel 1: AIN2(+) to AIN3(-) differential  
    // Medium gain (16x), higher sample rate (400 SPS)
    ads124_configure_channel(&ads124_handle, 1,
                            ADS124_AIN2, ADS124_AIN3,
                            ADS124_PGA_GAIN_16, ADS124_DATARATE_400SPS);
    
    // Configure Channel 2: AIN4(+) to AIN5(-) differential
    // Low gain (4x) for larger signals, fast sample rate (1000 SPS)
    ads124_configure_channel(&ads124_handle, 2,
                            ADS124_AIN4, ADS124_AIN5,
                            ADS124_PGA_GAIN_4, ADS124_DATARATE_1000SPS);
    
    // Enable the configured channels
    ads124_enable_channel(&ads124_handle, 0, true);
    ads124_enable_channel(&ads124_handle, 1, true);
    ads124_enable_channel(&ads124_handle, 2, true);
    
    // Start continuous conversions
    if (ads124_start_conversion(&ads124_handle) != HAL_OK) {
        // Handle start error
        return;
    }
}

/**
 * @brief Example processing function
 * Call this from app_run() main loop
 */
void ads124_example_process(void)
{
    // Check for completed data buffers
    ads124_data_buffer_t *completed_buffer = ads124_get_completed_buffer(&ads124_handle);
    
    if (completed_buffer != NULL) {
        // Buffer is ready for transmission
        // Here you would typically:
        // 1. Package the data for CAN transmission
        // 2. Send over CAN bus
        // 3. Release the buffer
        
        // Example: Print buffer info (replace with CAN transmission)
       dbg_printf("Buffer %d ready: %d samples at %d Hz, timestamp: %lu\\n",
               completed_buffer->buffer_id,
               60, // Always 60 samples
               completed_buffer->sample_rate,
               completed_buffer->timestamp);
        
        // Example: Access the data
        for (int i = 0; i < 60; i++) {
            uint16_t sample = completed_buffer->data[i];
            // Process sample data here
            // Convert back to voltage if needed using ads124_convert_to_voltage()
        }
        
        // Mark buffer as processed
        ads124_release_buffer(&ads124_handle, completed_buffer);
    }
}

/**
 * @brief GPIO EXTI callback function
 * This should be called from the STM32 EXTI callback
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    // Call the ADS124 data ready callback
    ads124_data_ready_callback(GPIO_Pin);
}

void app_run(void) {
    ads124_example_init();
    while (1) {
        ads124_example_process();
        HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
        HAL_Delay(100); // Delay for 100 ms before the next reading
    }
}