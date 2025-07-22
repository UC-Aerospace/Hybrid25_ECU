#include "ADS124.h"
#include "app.h"

/* Example usage of ADS124 module
 * 
 * This file demonstrates how to configure and use the ADS124S08 ADC
 * for differential signal measurement with dual buffering for CAN transmission.
 */

// Global ADS124 handle
static ads124_handle_t ads124_handle;

/**
 * @brief Example initialization function
 * Call this from app_init() or main initialization
 */
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
        printf("Buffer %d ready: %d samples at %d Hz, timestamp: %lu\\n",
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

/**
 * @brief Example function to change channel configuration at runtime
 */
void ads124_example_reconfigure_channel(uint8_t channel, ads124_pga_gain_t new_gain, ads124_datarate_t new_rate)
{
    // Stop conversions
    ads124_stop_conversion(&ads124_handle);
    
    // Get current configuration
    ads124_channel_config_t *config = &ads124_handle.channels[channel];
    
    // Update configuration
    ads124_configure_channel(&ads124_handle, channel,
                            config->positive_input, config->negative_input,
                            new_gain, new_rate);
    
    // Restart conversions
    ads124_start_conversion(&ads124_handle);
}

/**
 * @brief Example function to enable/disable channels dynamically
 */
void ads124_example_toggle_channel(uint8_t channel, bool enable)
{
    // Stop conversions
    ads124_stop_conversion(&ads124_handle);
    
    // Enable/disable channel
    ads124_enable_channel(&ads124_handle, channel, enable);
    
    // Restart conversions if any channels are enabled
    bool any_enabled = false;
    for (int i = 0; i < 12; i++) {
        if (ads124_handle.channels[i].enabled) {
            any_enabled = true;
            break;
        }
    }
    
    if (any_enabled) {
        ads124_start_conversion(&ads124_handle);
    }
}

/* Usage Notes:
 * 
 * 1. GPIO Configuration Required:
 *    - ADC_RDY_Pin (PC5) must be configured as EXTI input with falling edge trigger
 *    - ADC_RST_Pin (PB0) must be configured as GPIO output
 *    - SPI1 pins must be properly configured
 * 
 * 2. SPI Configuration:
 *    - The current SPI configuration in main.c needs adjustment:
 *      - DataSize should be SPI_DATASIZE_8BIT (not 4BIT)
 *      - BaudRatePrescaler may need adjustment based on required speed
 *      - CLKPolarity and CLKPhase should match ADS124S08 requirements
 * 
 * 3. Interrupt Priority:
 *    - EXTI interrupt for ADC_RDY should have appropriate priority
 *    - Consider using HAL_NVIC_SetPriority() to set priority
 * 
 * 4. Buffer Management:
 *    - Each buffer holds 60 samples of 16-bit data
 *    - Timestamp is recorded at start of buffer filling
 *    - Sample rate is stored with each buffer
 *    - Dual buffering allows continuous operation
 * 
 * 5. Channel Switching:
 *    - Channels are switched automatically between conversions
 *    - Only enabled channels are measured
 *    - Each channel can have different gain and sample rate
 * 
 * 6. Data Format:
 *    - 24-bit ADC data is decimated to 16-bit for storage
 *    - Original precision can be maintained by adjusting decimation
 *    - Conversion to voltage available via ads124_convert_to_voltage()
 */
