/* ADS124S08 Configuration Guide
 * 
 * This file contains the required configuration changes for proper ADS124S08 operation.
 * Apply these changes to your STM32CubeMX configuration or modify the generated code directly.
 */

#ifndef ADS124_CONFIG_GUIDE_H
#define ADS124_CONFIG_GUIDE_H

/* 
 * ==============================================================================
 * REQUIRED SPI1 CONFIGURATION CHANGES
 * ==============================================================================
 * 
 * The current SPI1 configuration in main.c needs the following changes:
 * 
 * FROM:
 *   hspi1.Init.DataSize = SPI_DATASIZE_4BIT;
 * TO:
 *   hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
 * 
 * REASON: ADS124S08 communicates using 8-bit words, not 4-bit
 * 
 * OPTIONAL CHANGES (depending on timing requirements):
 * - BaudRatePrescaler: Current SPI_BAUDRATEPRESCALER_8 should work fine
 * - CLKPolarity and CLKPhase: Current settings (CPOL=0, CPHA=0) are correct for ADS124S08
 * - NSS: Consider using SPI_NSS_SOFT for manual CS control if needed
 */

/* 
 * ==============================================================================
 * REQUIRED GPIO CONFIGURATION
 * ==============================================================================
 * 
 * The following GPIO pins need to be configured:
 * 
 * 1. ADC_RDY_Pin (PC5) - Data Ready Signal from ADS124S08
 *    - Mode: GPIO_MODE_IT_FALLING (External Interrupt, Falling Edge)
 *    - Pull: GPIO_PULLUP (Internal pull-up)
 *    - Speed: GPIO_SPEED_FREQ_HIGH
 *    - Enable EXTI5 interrupt in NVIC
 * 
 * 2. ADC_RST_Pin (PB0) - Reset Control to ADS124S08  
 *    - Mode: GPIO_MODE_OUTPUT_PP (Push-Pull Output)
 *    - Pull: GPIO_NOPULL
 *    - Initial State: GPIO_PIN_SET (High - not in reset)
 *    - Speed: GPIO_SPEED_FREQ_LOW
 * 
 * 3. SPI1 Pins (if not already configured):
 *    - PA5: SPI1_SCK (Alternate Function)
 *    - PA6: SPI1_MISO (Alternate Function) 
 *    - PA7: SPI1_MOSI (Alternate Function)
 *    - PA4: SPI1_NSS (if using hardware NSS) or GPIO for manual CS
 */

/* 
 * ==============================================================================
 * INTERRUPT CONFIGURATION
 * ==============================================================================
 * 
 * Configure EXTI5_9_IRQn interrupt for ADC_RDY_Pin:
 * 
 * In main.c, add to HAL_GPIO_EXTI_Callback():
 * 
 * void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
 * {
 *     if (GPIO_Pin == ADC_RDY_Pin) {
 *         ads124_data_ready_callback(GPIO_Pin);
 *     }
 * }
 * 
 * Set appropriate interrupt priority:
 * HAL_NVIC_SetPriority(EXTI4_15_IRQn, 2, 0);
 * HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
 */

/* 
 * ==============================================================================
 * APPLICATION INTEGRATION
 * ==============================================================================
 * 
 * 1. Include ADS124.h in your main application files
 * 2. Add ADS124 source files to your build system
 * 3. Call ads124_init() during system initialization
 * 4. Call ads124_example_process() in your main loop
 * 5. Configure channels as needed using ads124_configure_channel()
 * 6. Start conversions with ads124_start_conversion()
 */

/* 
 * ==============================================================================
 * EXAMPLE INTEGRATION IN APP.C
 * ==============================================================================
 * 
 * #include "ADS124.h"
 * 
 * static ads124_handle_t ads124_handle;
 * 
 * void app_init(void) {
 *     // ... existing initialization ...
 *     
 *     // Initialize ADS124
 *     if (ads124_init(&ads124_handle, &hspi1) == HAL_OK) {
 *         // Configure channels for your specific application
 *         ads124_configure_channel(&ads124_handle, 0, 
 *                                 ADS124_AIN0, ADS124_AIN1, 
 *                                 ADS124_PGA_GAIN_32, ADS124_DATARATE_100SPS);
 *         ads124_enable_channel(&ads124_handle, 0, true);
 *         ads124_start_conversion(&ads124_handle);
 *     }
 * }
 * 
 * void app_run(void) {
 *     while (1) {
 *         // Check for completed buffers
 *         ads124_data_buffer_t *buffer = ads124_get_completed_buffer(&ads124_handle);
 *         if (buffer != NULL) {
 *             // Process/transmit data via CAN
 *             transmit_data_via_can(buffer);
 *             ads124_release_buffer(&ads124_handle, buffer);
 *         }
 *         
 *         // ... other application tasks ...
 *         HAL_Delay(10);
 *     }
 * }
 */

#endif /* ADS124_CONFIG_GUIDE_H */
