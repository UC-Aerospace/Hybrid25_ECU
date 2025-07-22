#ifndef ADS124_H
#define ADS124_H

#include "stm32g0xx_hal.h"
#include "peripherals.h"
#include "config.h"
#include <stdint.h>
#include <stdbool.h>

/* ADS124S08 Register Definitions */
#define ADS124_ID_REG           0x00
#define ADS124_STATUS_REG       0x01
#define ADS124_INPMUX_REG       0x02
#define ADS124_PGA_REG          0x03
#define ADS124_DATARATE_REG     0x04
#define ADS124_REF_REG          0x05
#define ADS124_IDACMAG_REG      0x06
#define ADS124_IDACMUX_REG      0x07
#define ADS124_VBIAS_REG        0x08
#define ADS124_SYS_REG          0x09
#define ADS124_OFCAL0_REG       0x0A
#define ADS124_OFCAL1_REG       0x0B
#define ADS124_OFCAL2_REG       0x0C
#define ADS124_FSCAL0_REG       0x0D
#define ADS124_FSCAL1_REG       0x0E
#define ADS124_FSCAL2_REG       0x0F
#define ADS124_GPIODAT_REG      0x10
#define ADS124_GPIOCON_REG      0x11

/* ADS124S08 Commands */
#define ADS124_CMD_NOP          0x00
#define ADS124_CMD_WAKEUP       0x02
#define ADS124_CMD_POWERDOWN    0x04
#define ADS124_CMD_RESET        0x06
#define ADS124_CMD_START        0x08
#define ADS124_CMD_STOP         0x0A
#define ADS124_CMD_SYOCAL       0x16
#define ADS124_CMD_SYGCAL       0x17
#define ADS124_CMD_SFOCAL       0x19
#define ADS124_CMD_RDATA        0x12
#define ADS124_CMD_RREG         0x20
#define ADS124_CMD_WREG         0x40

/* PGA Gain Settings */
typedef enum {
    ADS124_PGA_GAIN_1   = 0x00,
    ADS124_PGA_GAIN_2   = 0x01,
    ADS124_PGA_GAIN_4   = 0x02,
    ADS124_PGA_GAIN_8   = 0x03,
    ADS124_PGA_GAIN_16  = 0x04,
    ADS124_PGA_GAIN_32  = 0x05,
    ADS124_PGA_GAIN_64  = 0x06,
    ADS124_PGA_GAIN_128 = 0x07
} ads124_pga_gain_t;

/* Data Rate Settings */
typedef enum {
    ADS124_DATARATE_2_5SPS   = 0x00,
    ADS124_DATARATE_5SPS     = 0x01,
    ADS124_DATARATE_10SPS    = 0x02,
    ADS124_DATARATE_16_6SPS  = 0x03,
    ADS124_DATARATE_20SPS    = 0x04,
    ADS124_DATARATE_50SPS    = 0x05,
    ADS124_DATARATE_60SPS    = 0x06,
    ADS124_DATARATE_100SPS   = 0x07,
    ADS124_DATARATE_200SPS   = 0x08,
    ADS124_DATARATE_400SPS   = 0x09,
    ADS124_DATARATE_800SPS   = 0x0A,
    ADS124_DATARATE_1000SPS  = 0x0B,
    ADS124_DATARATE_2000SPS  = 0x0C,
    ADS124_DATARATE_4000SPS  = 0x0D
} ads124_datarate_t;

/* Input Channel Definitions */
typedef enum {
    ADS124_AIN0 = 0x00,
    ADS124_AIN1 = 0x01,
    ADS124_AIN2 = 0x02,
    ADS124_AIN3 = 0x03,
    ADS124_AIN4 = 0x04,
    ADS124_AIN5 = 0x05,
    ADS124_AIN6 = 0x06,
    ADS124_AIN7 = 0x07,
    ADS124_AIN8 = 0x08,
    ADS124_AIN9 = 0x09,
    ADS124_AIN10 = 0x0A,
    ADS124_AIN11 = 0x0B,
    ADS124_AINCOM = 0x0C,
    ADS124_TEMP_SENSOR = 0x0D,
    ADS124_ANALOG_SUPPLY = 0x0E,
    ADS124_DIGITAL_SUPPLY = 0x0F
} ads124_input_t;

/* Channel Configuration Structure */
typedef struct {
    ads124_input_t positive_input;
    ads124_input_t negative_input;
    ads124_pga_gain_t gain;
    ads124_datarate_t data_rate;
    bool enabled;
} ads124_channel_config_t;

/* Data Buffer Structure */
typedef struct {
    uint16_t data[60];
    uint32_t timestamp;
    uint16_t sample_rate;
    uint8_t buffer_id;
    bool ready;
} ads124_data_buffer_t;

/* ADS124 Handle Structure */
typedef struct {
    SPI_HandleTypeDef *hspi;
    ads124_channel_config_t channels[12];  // Support up to 12 differential channels
    ads124_data_buffer_t buffer_a;
    ads124_data_buffer_t buffer_b;
    ads124_data_buffer_t *active_buffer;
    uint8_t current_channel;
    uint8_t buffer_index;
    uint32_t conversion_start_time;
    bool initialized;
    bool conversion_ready;
} ads124_handle_t;

/* Function Prototypes */

/**
 * @brief Initialize the ADS124S08 ADC
 * @param handle Pointer to ADS124 handle structure
 * @param hspi Pointer to SPI handle
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef ads124_init(ads124_handle_t *handle, SPI_HandleTypeDef *hspi);

/**
 * @brief Configure a differential input channel
 * @param handle Pointer to ADS124 handle structure
 * @param channel_index Channel index (0-11)
 * @param pos_input Positive input pin
 * @param neg_input Negative input pin
 * @param gain PGA gain setting
 * @param data_rate Sample rate setting
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef ads124_configure_channel(ads124_handle_t *handle, 
                                          uint8_t channel_index,
                                          ads124_input_t pos_input,
                                          ads124_input_t neg_input,
                                          ads124_pga_gain_t gain,
                                          ads124_datarate_t data_rate);

/**
 * @brief Enable/disable a channel
 * @param handle Pointer to ADS124 handle structure
 * @param channel_index Channel index (0-11)
 * @param enable True to enable, false to disable
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef ads124_enable_channel(ads124_handle_t *handle, uint8_t channel_index, bool enable);

/**
 * @brief Start continuous conversion
 * @param handle Pointer to ADS124 handle structure
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef ads124_start_conversion(ads124_handle_t *handle);

/**
 * @brief Stop continuous conversion
 * @param handle Pointer to ADS124 handle structure
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef ads124_stop_conversion(ads124_handle_t *handle);

/**
 * @brief Process ADC data ready interrupt
 * @param handle Pointer to ADS124 handle structure
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef ads124_process_data_ready(ads124_handle_t *handle);

/**
 * @brief Get completed data buffer
 * @param handle Pointer to ADS124 handle structure
 * @retval Pointer to completed buffer or NULL if none ready
 */
ads124_data_buffer_t* ads124_get_completed_buffer(ads124_handle_t *handle);

/**
 * @brief Mark buffer as processed and available for reuse
 * @param handle Pointer to ADS124 handle structure
 * @param buffer Pointer to buffer to mark as processed
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef ads124_release_buffer(ads124_handle_t *handle, ads124_data_buffer_t *buffer);

/**
 * @brief Reset the ADS124S08
 * @param handle Pointer to ADS124 handle structure
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef ads124_reset(ads124_handle_t *handle);

/**
 * @brief Read device ID
 * @param handle Pointer to ADS124 handle structure
 * @param device_id Pointer to store device ID
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef ads124_read_id(ads124_handle_t *handle, uint8_t *device_id);

/**
 * @brief GPIO interrupt callback for data ready signal
 * @param GPIO_Pin Pin number that triggered the interrupt
 */
void ads124_data_ready_callback(uint16_t GPIO_Pin);

/* Internal helper functions */
HAL_StatusTypeDef ads124_write_register(ads124_handle_t *handle, uint8_t reg, uint8_t value);
HAL_StatusTypeDef ads124_read_register(ads124_handle_t *handle, uint8_t reg, uint8_t *value);
HAL_StatusTypeDef ads124_send_command(ads124_handle_t *handle, uint8_t command);
HAL_StatusTypeDef ads124_read_data(ads124_handle_t *handle, int32_t *data);
HAL_StatusTypeDef ads124_switch_to_channel(ads124_handle_t *handle, uint8_t channel_index);
HAL_StatusTypeDef ads124_switch_to_next_channel(ads124_handle_t *handle);

/* Conversion utilities */
uint16_t ads124_convert_to_16bit(int32_t raw_data, ads124_pga_gain_t gain);
float ads124_convert_to_voltage(int32_t raw_data, ads124_pga_gain_t gain, float vref);

#endif /* ADS124_H */
