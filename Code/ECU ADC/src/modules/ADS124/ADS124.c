#include "ADS124.h"
#include <string.h>

/* Static variables */
static ads124_handle_t *g_ads124_handle = NULL;

/* Timeout definitions */
#define ADS124_SPI_TIMEOUT    1000
#define ADS124_RESET_DELAY    10
#define ADS124_STARTUP_DELAY  50

/* Data rate to sample rate mapping (in Hz) */
static const uint16_t ads124_sample_rates[] = {
    3,    // 2.5 SPS (rounded to 3)
    5,    // 5 SPS
    10,   // 10 SPS
    17,   // 16.6 SPS (rounded to 17)
    20,   // 20 SPS
    50,   // 50 SPS
    60,   // 60 SPS
    100,  // 100 SPS
    200,  // 200 SPS
    400,  // 400 SPS
    800,  // 800 SPS
    1000, // 1000 SPS
    2000, // 2000 SPS
    4000  // 4000 SPS
};

/**
 * @brief Initialize the ADS124S08 ADC
 */
HAL_StatusTypeDef ads124_init(ads124_handle_t *handle, SPI_HandleTypeDef *hspi)
{
    if (handle == NULL || hspi == NULL) {
        return HAL_ERROR;
    }

    // Store global handle for interrupt callback
    g_ads124_handle = handle;
    
    // Initialize handle structure
    memset(handle, 0, sizeof(ads124_handle_t));
    handle->hspi = hspi;
    
    // Initialize buffers
    handle->buffer_a.buffer_id = 0;
    handle->buffer_b.buffer_id = 1;
    handle->active_buffer = &handle->buffer_a;
    
    // Hardware reset
    HAL_StatusTypeDef status = ads124_reset(handle);
    if (status != HAL_OK) {
        return status;
    }
    
    // Verify device ID
    uint8_t device_id;
    status = ads124_read_id(handle, &device_id);
    if (status != HAL_OK) {
        return status;
    }
    
    // Expected device ID for ADS124S08 (check datasheet for exact value)
    if ((device_id & 0xF0) != 0x00) {  // Device ID mask
        return HAL_ERROR;
    }
    
    // Configure default settings
    // Set internal reference
    status = ads124_write_register(handle, ADS124_REF_REG, 0x10);  // Internal 2.5V reference
    if (status != HAL_OK) return status;
    
    // Set system settings
    status = ads124_write_register(handle, ADS124_SYS_REG, 0x10);  // PGA enabled, timeout disabled
    if (status != HAL_OK) return status;
    
    handle->initialized = true;
    return HAL_OK;
}

/**
 * @brief Configure a differential input channel
 */
HAL_StatusTypeDef ads124_configure_channel(ads124_handle_t *handle, 
                                          uint8_t channel_index,
                                          ads124_input_t pos_input,
                                          ads124_input_t neg_input,
                                          ads124_pga_gain_t gain,
                                          ads124_datarate_t data_rate)
{
    if (handle == NULL || !handle->initialized || channel_index >= 12) {
        return HAL_ERROR;
    }
    
    // Store channel configuration
    handle->channels[channel_index].positive_input = pos_input;
    handle->channels[channel_index].negative_input = neg_input;
    handle->channels[channel_index].gain = gain;
    handle->channels[channel_index].data_rate = data_rate;
    handle->channels[channel_index].enabled = false;  // Enable separately
    
    return HAL_OK;
}

/**
 * @brief Enable/disable a channel
 */
HAL_StatusTypeDef ads124_enable_channel(ads124_handle_t *handle, uint8_t channel_index, bool enable)
{
    if (handle == NULL || !handle->initialized || channel_index >= 12) {
        return HAL_ERROR;
    }
    
    handle->channels[channel_index].enabled = enable;
    return HAL_OK;
}

/**
 * @brief Start continuous conversion
 */
HAL_StatusTypeDef ads124_start_conversion(ads124_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return HAL_ERROR;
    }
    
    // Find first enabled channel
    handle->current_channel = 255;  // Invalid channel
    for (uint8_t i = 0; i < 12; i++) {
        if (handle->channels[i].enabled) {
            handle->current_channel = i;
            break;
        }
    }
    
    if (handle->current_channel == 255) {
        return HAL_ERROR;  // No channels enabled
    }
    
    // Configure first channel
    HAL_StatusTypeDef status = ads124_switch_to_channel(handle, handle->current_channel);
    if (status != HAL_OK) return status;
    
    // Start conversions
    status = ads124_send_command(handle, ADS124_CMD_START);
    if (status != HAL_OK) return status;
    
    // Record start time
    handle->conversion_start_time = HAL_GetTick();
    handle->buffer_index = 0;
    
    return HAL_OK;
}

/**
 * @brief Stop continuous conversion
 */
HAL_StatusTypeDef ads124_stop_conversion(ads124_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return HAL_ERROR;
    }
    
    return ads124_send_command(handle, ADS124_CMD_STOP);
}

/**
 * @brief Process ADC data ready interrupt
 */
HAL_StatusTypeDef ads124_process_data_ready(ads124_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return HAL_ERROR;
    }
    
    // Read conversion data
    int32_t raw_data;
    HAL_StatusTypeDef status = ads124_read_data(handle, &raw_data);
    if (status != HAL_OK) return status;
    
    // Convert to 16-bit and store in buffer
    ads124_channel_config_t *current_config = &handle->channels[handle->current_channel];
    uint16_t data_16bit = ads124_convert_to_16bit(raw_data, current_config->gain);
    
    handle->active_buffer->data[handle->buffer_index] = data_16bit;
    handle->buffer_index++;
    
    // Check if buffer is full
    if (handle->buffer_index >= 60) {
        // Mark buffer as ready
        handle->active_buffer->ready = true;
        handle->active_buffer->timestamp = handle->conversion_start_time;
        handle->active_buffer->sample_rate = ads124_sample_rates[current_config->data_rate];
        
        // Switch to other buffer
        if (handle->active_buffer == &handle->buffer_a) {
            handle->active_buffer = &handle->buffer_b;
        } else {
            handle->active_buffer = &handle->buffer_a;
        }
        
        // Reset buffer index and timestamp
        handle->buffer_index = 0;
        handle->conversion_start_time = HAL_GetTick();
        handle->active_buffer->ready = false;
    }
    
    // Switch to next enabled channel for next conversion
    return ads124_switch_to_next_channel(handle);
}

/**
 * @brief Get completed data buffer
 */
ads124_data_buffer_t* ads124_get_completed_buffer(ads124_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return NULL;
    }
    
    if (handle->buffer_a.ready) {
        return &handle->buffer_a;
    } else if (handle->buffer_b.ready) {
        return &handle->buffer_b;
    }
    
    return NULL;
}

/**
 * @brief Mark buffer as processed and available for reuse
 */
HAL_StatusTypeDef ads124_release_buffer(ads124_handle_t *handle, ads124_data_buffer_t *buffer)
{
    if (handle == NULL || buffer == NULL) {
        return HAL_ERROR;
    }
    
    buffer->ready = false;
    return HAL_OK;
}

/**
 * @brief Reset the ADS124S08
 */
HAL_StatusTypeDef ads124_reset(ads124_handle_t *handle)
{
    if (handle == NULL) {
        return HAL_ERROR;
    }
    
    // Hardware reset using GPIO
    HAL_GPIO_WritePin(ADC_RST_GPIO_Port, ADC_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(ADS124_RESET_DELAY);
    HAL_GPIO_WritePin(ADC_RST_GPIO_Port, ADC_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(ADS124_STARTUP_DELAY);
    
    // Send reset command over SPI as well
    return ads124_send_command(handle, ADS124_CMD_RESET);
}

/**
 * @brief Read device ID
 */
HAL_StatusTypeDef ads124_read_id(ads124_handle_t *handle, uint8_t *device_id)
{
    if (handle == NULL || device_id == NULL) {
        return HAL_ERROR;
    }
    
    return ads124_read_register(handle, ADS124_ID_REG, device_id);
}

/**
 * @brief GPIO interrupt callback for data ready signal
 */
void ads124_data_ready_callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == ADC_RDY_Pin && g_ads124_handle != NULL) {
        g_ads124_handle->conversion_ready = true;
        // Process in main loop or call directly if timing allows
        ads124_process_data_ready(g_ads124_handle);
    }
}

/**
 * @brief Write to ADS124 register
 */
HAL_StatusTypeDef ads124_write_register(ads124_handle_t *handle, uint8_t reg, uint8_t value)
{
    if (handle == NULL || handle->hspi == NULL) {
        return HAL_ERROR;
    }
    
    uint8_t tx_data[3];
    tx_data[0] = ADS124_CMD_WREG | reg;  // Write register command + address
    tx_data[1] = 0x00;                   // Number of bytes - 1 (writing 1 byte)
    tx_data[2] = value;                  // Data to write
    
    return HAL_SPI_Transmit(handle->hspi, tx_data, 3, ADS124_SPI_TIMEOUT);
}

/**
 * @brief Read from ADS124 register
 */
HAL_StatusTypeDef ads124_read_register(ads124_handle_t *handle, uint8_t reg, uint8_t *value)
{
    if (handle == NULL || handle->hspi == NULL || value == NULL) {
        return HAL_ERROR;
    }
    
    uint8_t tx_data[3];
    uint8_t rx_data[3];
    
    tx_data[0] = ADS124_CMD_RREG | reg;  // Read register command + address
    tx_data[1] = 0x00;                   // Number of bytes - 1 (reading 1 byte)
    tx_data[2] = 0x00;                   // Dummy byte
    
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(handle->hspi, tx_data, rx_data, 3, ADS124_SPI_TIMEOUT);
    if (status == HAL_OK) {
        *value = rx_data[2];
    }
    
    return status;
}

/**
 * @brief Send command to ADS124
 */
HAL_StatusTypeDef ads124_send_command(ads124_handle_t *handle, uint8_t command)
{
    if (handle == NULL || handle->hspi == NULL) {
        return HAL_ERROR;
    }
    
    return HAL_SPI_Transmit(handle->hspi, &command, 1, ADS124_SPI_TIMEOUT);
}

/**
 * @brief Read conversion data
 */
HAL_StatusTypeDef ads124_read_data(ads124_handle_t *handle, int32_t *data)
{
    if (handle == NULL || handle->hspi == NULL || data == NULL) {
        return HAL_ERROR;
    }
    
    uint8_t tx_data[4] = {ADS124_CMD_RDATA, 0x00, 0x00, 0x00};
    uint8_t rx_data[4];
    
    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(handle->hspi, tx_data, rx_data, 4, ADS124_SPI_TIMEOUT);
    if (status == HAL_OK) {
        // Combine 24-bit result (sign extend to 32-bit)
        *data = ((int32_t)rx_data[1] << 16) | ((int32_t)rx_data[2] << 8) | rx_data[3];
        
        // Sign extend from 24-bit to 32-bit
        if (*data & 0x800000) {
            *data |= 0xFF000000;
        }
    }
    
    return status;
}

/**
 * @brief Switch to specific channel
 */
HAL_StatusTypeDef ads124_switch_to_channel(ads124_handle_t *handle, uint8_t channel_index)
{
    if (handle == NULL || !handle->initialized || channel_index >= 12) {
        return HAL_ERROR;
    }
    
    ads124_channel_config_t *config = &handle->channels[channel_index];
    if (!config->enabled) {
        return HAL_ERROR;
    }
    
    HAL_StatusTypeDef status;
    
    // Configure input multiplexer
    uint8_t inpmux_value = (config->positive_input << 4) | config->negative_input;
    status = ads124_write_register(handle, ADS124_INPMUX_REG, inpmux_value);
    if (status != HAL_OK) return status;
    
    // Configure PGA
    uint8_t pga_value = 0x08 | config->gain;  // PGA enabled + gain setting
    status = ads124_write_register(handle, ADS124_PGA_REG, pga_value);
    if (status != HAL_OK) return status;
    
    // Configure data rate
    status = ads124_write_register(handle, ADS124_DATARATE_REG, config->data_rate);
    if (status != HAL_OK) return status;
    
    return HAL_OK;
}

/**
 * @brief Switch to next enabled channel
 */
HAL_StatusTypeDef ads124_switch_to_next_channel(ads124_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) {
        return HAL_ERROR;
    }
    
    // Find next enabled channel
    uint8_t start_channel = handle->current_channel;
    uint8_t next_channel = start_channel;
    
    do {
        next_channel = (next_channel + 1) % 12;
        if (handle->channels[next_channel].enabled) {
            handle->current_channel = next_channel;
            return ads124_switch_to_channel(handle, next_channel);
        }
    } while (next_channel != start_channel);
    
    // No other enabled channels found, stay on current
    return HAL_OK;
}

/**
 * @brief Convert 24-bit raw data to 16-bit
 */
uint16_t ads124_convert_to_16bit(int32_t raw_data, ads124_pga_gain_t gain)
{
    // ADS124S08 provides 24-bit signed data
    // Scale down to 16-bit while preserving sign and relative magnitude
    
    // For 24-bit signed: range is -8,388,608 to +8,388,607
    // For 16-bit signed: range is -32,768 to +32,767
    
    // Simple approach: shift right by 8 bits (divide by 256)
    int16_t result = (int16_t)(raw_data >> 8);
    
    return (uint16_t)result;
}

/**
 * @brief Convert raw data to voltage
 */
float ads124_convert_to_voltage(int32_t raw_data, ads124_pga_gain_t gain, float vref)
{
    // Calculate gain multiplier
    uint8_t gain_multiplier = 1 << gain;  // 2^gain
    
    // ADS124S08 is 24-bit
    float full_scale = 8388607.0f;  // 2^23 - 1
    
    // Convert to voltage
    float voltage = (float)raw_data * vref / (full_scale * gain_multiplier);
    
    return voltage;
}
