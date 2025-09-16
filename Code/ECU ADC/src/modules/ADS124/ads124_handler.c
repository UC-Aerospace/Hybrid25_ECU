#include "ads124_handler.h"

static void ads124_conversion_tick(void);
static HAL_StatusTypeDef ads124_init_buffers(void);

// Define buffers for each sensor
can_buffer_t load_cell_a_buffer;
can_buffer_t load_cell_b_buffer;
can_buffer_t load_cell_c_buffer;
can_buffer_t thermo_a_buffer;
can_buffer_t thermo_b_buffer;
can_buffer_t thermo_c_buffer;

static int16_t cjt_correction = 0; // Cold Junction Temperature correction in adc counts

// Setup the scanning sequence
ads124_channel_conf_t scan_seq[10] = {
    // Positive Channel, Negative Channel, Use Internal Reference, Use VBIAS
    {ADS_P_AIN6, ADS_N_AIN7, false, false, &load_cell_a_buffer}, // Load cell A
    {ADS_P_AIN2, ADS_N_AIN3, false, false, &load_cell_b_buffer}, // Load cell B
    {ADS_P_AIN6, ADS_N_AIN7, false, false, &load_cell_a_buffer}, // Load cell A
    {ADS_P_AIN4, ADS_N_AIN5, false, false, &load_cell_c_buffer}, // Load cell C
    {ADS_P_AIN6, ADS_N_AIN7, false, false, &load_cell_a_buffer}, // Load cell A
    {ADS_P_AIN0, ADS_N_AIN1, true, true, &thermo_a_buffer}, // Thermocouple A
    {ADS_P_AIN6, ADS_N_AIN7, false, false, &load_cell_a_buffer}, // Load cell A
    {ADS_P_AIN8, ADS_N_AIN9, true, false, &thermo_b_buffer}, // Thermocouple B
    {ADS_P_AIN6, ADS_N_AIN7, false, false, &load_cell_a_buffer}, // Load cell A
    {ADS_P_AIN10, ADS_N_AIN11, true, false, &thermo_c_buffer} // Thermocouple C
};

/**
 * @brief Initialize the ADS124S08 ADC
 * @return HAL_OK on success, HAL_ERROR on failure
 */
HAL_StatusTypeDef ads124_init(void)
{
    resetADC(&hspi1);

    // Initialize the ADC peripherals
    if (!InitADCPeripherals(&hspi1)) {
        return HAL_ERROR;
    }

    // Setup the buffers for each sensor
    if (ads124_init_buffers() != HAL_OK) {
        return HAL_ERROR;
    }
    
    // Set first for load cell A
    if (ads124_configure_channel(ADS_P_AIN6, ADS_N_AIN7) != HAL_OK) {
        return HAL_ERROR;
    }

    // Set PGA gain to 128
    if (ads124_set_pga_gain(ADS_GAIN_128) != HAL_OK) {
        return HAL_ERROR;
    }

    // Set conversion mode to single shot
    if (ads124_set_conversion_mode(ADS_CONVMODE_SS) != HAL_OK) {
        return HAL_ERROR;
    }
    
    // Set data rate to 400 SPS
    if (ads124_set_data_rate(ADS_DR_400) != HAL_OK) {
        return HAL_ERROR;
    }

    // Set low latency filter
    if (ads124_set_low_latency_filter(true) != HAL_OK) {
        return HAL_ERROR;
    }

    // Start the conversion process
    if (ads124_start_conversion() != HAL_OK) {
        return HAL_ERROR;
    }

    // Start timer for periodic conversions @ 200 Hz
    if (HAL_TIM_Base_Start_IT(&htim14) != HAL_OK) {
        return HAL_ERROR;
    }

    return HAL_OK;
}

static HAL_StatusTypeDef ads124_init_buffers(void)
{
    // Initialize buffers for each sensor
    can_buffer_init(&load_cell_a_buffer, SID_SENSOR_LC_Thrust, BUFF_SIZE_LC_Thrust);
    can_buffer_init(&load_cell_b_buffer, SID_SENSOR_LC_N2O_A, BUFF_SIZE_LC_N2O_A);
    can_buffer_init(&load_cell_c_buffer, SID_SENSOR_LC_N2O_B, BUFF_SIZE_LC_N2O_B);
    can_buffer_init(&thermo_a_buffer, SID_SENSOR_THERMO_A, BUFF_SIZE_THERMO_A);
    can_buffer_init(&thermo_b_buffer, SID_SENSOR_THERMO_B, BUFF_SIZE_THERMO_B);
    can_buffer_init(&thermo_c_buffer, SID_SENSOR_THERMO_C, BUFF_SIZE_THERMO_C);

    return HAL_OK;
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM14) {
        ads124_conversion_tick();
    }
}

void ads124_set_cjt_correction(int16_t correction)
{
    cjt_correction = correction;
}

static void ads124_conversion_tick(void)
{
    static uint8_t current_conv_index = 0;

    // Read the conversion result
    int32_t conversion_result;
    uint8_t status_byte;

    if (ads124_read_conversion(&conversion_result, &status_byte) != HAL_OK) {
        conversion_result = 0; // Default to 0 on error
    }

    // Concatenate the result to 16 bits
    conversion_result = (conversion_result >> 8) & 0xFFFF; // Keep only the 16 MSBits

    // If thermocouple (determined by internal reference usage), apply cjt correction
    if (scan_seq[current_conv_index].use_internal_ref) {
        conversion_result += cjt_correction;
    }

    // Put result onto the buffer based on the current channel
    can_buffer_push(scan_seq[current_conv_index].buffer, (int16_t)conversion_result);

    // Update the index for the next conversion
    current_conv_index++;
    if (current_conv_index >= 10) {
        current_conv_index = 0;
    }

    // Stop the current conversion
    // This is necessary to ensure we can configure the next channel
    ads124_stop_conversion();

    // Configure the next channel
    ads124_configure_channel(scan_seq[current_conv_index].pos_ch, scan_seq[current_conv_index].neg_ch);

    if (scan_seq[current_conv_index].use_internal_ref) {
        // Set internal reference
        ads124_set_reference(ADS_REFSEL_INT);
    } else {
        // Set external reference
        ads124_set_reference(ADS_REFSEL_P0);
    }

    if (scan_seq[current_conv_index].use_vbias) {
        // Set VBIAS for the channel
        ads124_set_vbias(scan_seq[current_conv_index].pos_ch, ADS_VBIAS_LVL_DIV12);
    } else {
        // Reset VBIAS to default
        ads124_set_vbias(ADS_VB_AINC, VBIAS_DEFAULT);
    }

    // Start the next conversion
    ads124_start_conversion();

}

/**
 * @brief Start continuous ADC conversions
 * @return HAL_OK on success, HAL_ERROR on failure
 */
HAL_StatusTypeDef ads124_start_conversion(void)
{
    // Start conversions
    startConversions(&hspi1);
    
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
        gain != ADS_GAIN_64 && gain != ADS_GAIN_128) {
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
    if (!waitForDRDYHtoL(0)) { // 1 second timeout
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

/**
 * @brief Set the conversion mode of the ADC
 * @param mode The desired conversion mode (ADS_CONVMODE_SS, ADS_CONVMODE_CONT)
 * @return HAL_OK on success, HAL_ERROR on failure
 */
HAL_StatusTypeDef ads124_set_conversion_mode(uint8_t mode)
{
    uint8_t status_reg;
    // Validate mode argument
    if (mode != ADS_CONVMODE_SS && mode != ADS_CONVMODE_CONT) {
        return HAL_ERROR; // Invalid mode
    }
    // Read current DATARATE register
    status_reg = readSingleRegister(&hspi1, REG_ADDR_DATARATE);
    // Clear old conversion mode bits
    status_reg &= ~(ADS_CONVMODE_SS | ADS_CONVMODE_CONT);
    // Set new conversion mode
    status_reg |= mode;
    // Write back to DATARATE register
    writeSingleRegister(&hspi1, REG_ADDR_DATARATE, status_reg);
    return HAL_OK;
}

/**
 * @brief Set the low latency filter mode
 * @param useLowLat boolean to set or clear ADS_FILTERTYPE_LL
 * @return HAL_OK on success, HAL_ERROR on failure
 */
HAL_StatusTypeDef ads124_set_low_latency_filter(bool useLowLat)
{
    uint8_t data_rate_reg;

    // Read current DATARATE register
    data_rate_reg = readSingleRegister(&hspi1, REG_ADDR_DATARATE);

    // Clear old filter type bits
    data_rate_reg &= ~ADS_FILTERTYPE_LL;

    // Set new filter type
    if (useLowLat) {
        data_rate_reg |= ADS_FILTERTYPE_LL;
    }

    // Write back to DATARATE register
    writeSingleRegister(&hspi1, REG_ADDR_DATARATE, data_rate_reg);

    return HAL_OK;
}

/**
 * @brief Set the data rate of the ADC
 * @param data_rate The desired data rate (ADS_DR_50, ADS_DR_100, etc.)
 * @return HAL_OK on success, HAL_ERROR on failure
 */
HAL_StatusTypeDef ads124_set_data_rate(uint8_t data_rate)
{
    uint8_t data_rate_reg;

    // Validate data rate argument
    if (data_rate > ADS_DR_4000) {
        return HAL_ERROR; // Invalid data rate
    }

    // Read current DATARATE register
    data_rate_reg = readSingleRegister(&hspi1, REG_ADDR_DATARATE);

    // Clear old data rate bits
    data_rate_reg &= ~(0x0F); // Clear bits 3:0 for data rate

    // Set new data rate
    data_rate_reg |= data_rate;

    // Write back to DATARATE register
    writeSingleRegister(&hspi1, REG_ADDR_DATARATE, data_rate_reg);

    return HAL_OK;
}