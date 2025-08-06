/**
 * @file ads124_example.c
 * @brief Example implementation of ADS124S08 driver with STM32 HAL
 * 
 * This file demonstrates how to use the converted ADS124S08 HAL functions
 * with the STM32 HAL library.
 */

#include "ads124_example.h"
#include "peripherals.h"
#include "main.h"

// External SPI handle (defined in main.c)
extern SPI_HandleTypeDef hspi1;

/**
 * @brief Initialize the ADS124S08 ADC
 * @return HAL_OK on success, HAL_ERROR on failure
 */
HAL_StatusTypeDef ads124_init(void)
{
    // Initialize the ADC peripherals
    if (!InitADCPeripherals(&hspi1)) {
        return HAL_ERROR;
    }
    
    // Configure initial settings
    // Example: Set to differential mode with AIN0 as positive and AIN1 as negative
    if (ads124_configure_channel(ADS_P_AIN0, ADS_N_AIN1) != HAL_OK) {
        return HAL_ERROR;
    }

    // Enable VBIAS on AIN1 at VDD/12
    if (ads124_set_vbias(ADS_VB_AIN1, ADS_VBIAS_LVL_DIV12) != HAL_OK) {
        return HAL_ERROR;
    }

    // Set internal reference
    if (ads124_set_reference(ADS_REFSEL_INT) != HAL_OK) {
        return HAL_ERROR;
    }

    // Set PGA gain to 32
    if (ads124_set_pga_gain(ADS_GAIN_32) != HAL_OK) {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

/**
 * @brief Start continuous ADC conversions
 * @return HAL_OK on success, HAL_ERROR on failure
 */
HAL_StatusTypeDef ads124_start_conversion(void)
{
    // Start conversions
    startConversions(&hspi1);
    
    // Enable DRDY interrupt
    enableDRDYinterrupt(true);
    
    return HAL_OK;
}

/**
 * @brief Configures VBIAS for a specific channel
 * @param channel The channel to configure (ADS_VBIAS_AIN0, ADS_VBIAS_AIN1, etc.)
 * @param bias Voltage level for VBIAS (ADS_VBIAS_VDD12, ADS_VBIAS_VDD24, etc.)
 * @return HAL_OK on success, HAL_ERROR on failure
 */
HAL_StatusTypeDef ads124_set_vbias(uint8_t channel, uint8_t bias)
{
    uint8_t vbias_config = 0;

    if (bias == VBIAS_DEFAULT) {
        // Reset VBIAS to default
        vbias_config = 0x00; // Default value for VBIAS register
    } else {
        // Validate arguments
        if (channel != ADS_VB_AINC && channel != ADS_VB_AIN0 &&
            channel != ADS_VB_AIN1 && channel != ADS_VB_AIN2 &&
            channel != ADS_VB_AIN3 && channel != ADS_VB_AIN4 &&
            channel != ADS_VB_AIN5) {
            return HAL_ERROR; // Invalid channel
        }

        // Validate bias voltage
        if (bias != ADS_VBIAS_LVL_DIV2 && bias != ADS_VBIAS_LVL_DIV12) {
            return HAL_ERROR; // Invalid bias voltage
        }

        // Configure VBIAS
        vbias_config |= (channel | bias);
    }
    writeSingleRegister(&hspi1, REG_ADDR_VBIAS, vbias_config);

    return HAL_OK;
}

/**
 * @brief Set the PGA gain
 * @param gain The desired gain (ADS_PGA_GAIN_1, ADS_PGA_GAIN_2, etc.)
 * @return HAL_OK on success, HAL_ERROR on failure
 */
HAL_StatusTypeDef ads124_set_pga_gain(uint8_t gain)
{
    uint8_t pga_config = 0;

    // Validate gain argument
    if (gain != ADS_GAIN_1 && gain != ADS_GAIN_2 &&
        gain != ADS_GAIN_4 && gain != ADS_GAIN_8 &&
        gain != ADS_GAIN_16 && gain != ADS_GAIN_32 &&
        gain != ADS_GAIN_64) {
        return HAL_ERROR; // Invalid gain
    }

    // Enable the PGA if gain not 1
    if (gain != ADS_GAIN_1) {
        pga_config |= ADS_PGA_ENABLED; // Enable PGA
    }

    // Configure PGA gain
    pga_config |= gain;

    writeSingleRegister(&hspi1, REG_ADDR_PGA, pga_config);

    return HAL_OK;
}

/**
 * @brief Stop ADC conversions
 * @return HAL_OK on success, HAL_ERROR on failure
 */
HAL_StatusTypeDef ads124_stop_conversion(void)
{
    // Stop conversions
    stopConversions(&hspi1);
    
    // Disable DRDY interrupt
    enableDRDYinterrupt(false);
    
    return HAL_OK;
}

/**
 * @brief Read a conversion result
 * @param data Pointer to store the 24-bit conversion result (sign-extended to 32-bit)
 * @param status Pointer to store the status byte (if enabled)
 * @return HAL_OK on success, HAL_ERROR on failure
 */
HAL_StatusTypeDef ads124_read_conversion(int32_t *data, uint8_t *status)
{
    if (data == NULL) {
        return HAL_ERROR;
    }
    
    // Wait for conversion to complete with timeout
    if (!waitForDRDYHtoL(1000)) { // 1 second timeout
        return HAL_TIMEOUT;
    }
    
    // Read the conversion data
    *data = readConvertedData(&hspi1, status, DIRECT);
    
    return HAL_OK;
}

/**
 * @brief Configure the input channel multiplexer
 * @param pos_ch Positive input channel (ADS_P_AIN0, ADS_P_AIN1, etc.)
 * @param neg_ch Negative input channel (ADS_N_AIN0, ADS_N_AINCOM, etc.)
 * @return HAL_OK on success, HAL_ERROR on failure
 */
HAL_StatusTypeDef ads124_configure_channel(uint8_t pos_ch, uint8_t neg_ch)
{
    uint8_t mux_config = pos_ch | neg_ch;
    
    // Write to INPMUX register
    writeSingleRegister(&hspi1, REG_ADDR_INPMUX, mux_config);
    
    return HAL_OK;
}

HAL_StatusTypeDef ads124_set_reference(uint8_t refsel)
{
    uint8_t ref_reg;

    // Validate argument (external P0/P1 or internal)
    if (refsel != ADS_REFSEL_P0 && refsel != ADS_REFSEL_P1 && refsel != ADS_REFSEL_INT) {
        return HAL_ERROR;
    }

    // Read current REF register
    ref_reg = readSingleRegister(&hspi1, REG_ADDR_REF);

    // Clear old REFSEL (bits 3:2) and REFCON (bits 1:0)
    ref_reg &= ~(ADS_REFSEL_P1 | ADS_REFSEL_INT | ADS_REFINT_ON_PDWN | ADS_REFINT_ON_ALWAYS);

    // Set new reference selection
    ref_reg |= refsel;

    // If internal reference selected, enable it always
    if (refsel == ADS_REFSEL_INT) {
        ref_reg |= ADS_REFINT_ON_ALWAYS;
    }

    // Write back to REF register
    writeSingleRegister(&hspi1, REG_ADDR_REF, ref_reg);

    return HAL_OK;
}
