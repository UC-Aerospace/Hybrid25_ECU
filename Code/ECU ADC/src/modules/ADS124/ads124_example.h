/**
 * @file ads124_example.h
 * @brief Example usage of ADS124S08 driver with STM32 HAL
 */

#ifndef ADS124_EXAMPLE_H
#define ADS124_EXAMPLE_H

#include "stm32g0xx_hal.h"
#include "hal.h"
#include "ads124.h"

// Example function prototypes
HAL_StatusTypeDef ads124_init(void);
HAL_StatusTypeDef ads124_start_conversion(void);
HAL_StatusTypeDef ads124_stop_conversion(void);
HAL_StatusTypeDef ads124_read_conversion(int32_t *data, uint8_t *status);
HAL_StatusTypeDef ads124_configure_channel(uint8_t pos_ch, uint8_t neg_ch);
HAL_StatusTypeDef ads124_set_reference(uint8_t refsel);
HAL_StatusTypeDef ads124_set_vbias(uint8_t vbias_ch, uint8_t vbias_value);
HAL_StatusTypeDef ads124_set_pga_gain(uint8_t gain);

#endif // ADS124_EXAMPLE_H
