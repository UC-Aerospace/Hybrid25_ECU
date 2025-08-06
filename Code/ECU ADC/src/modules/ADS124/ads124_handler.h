#ifndef ADS124_HANDLER_H
#define ADS124_HANDLER_H

#include "stm32g0xx_hal.h"
#include <stdbool.h>

#include "hal.h"
#include "ads124.h"
#include "peripherals.h"

typedef struct {
    uint8_t pos_ch;
    uint8_t neg_ch;
    bool use_internal_ref;
    bool use_vbias;
} ads124_channel_conf_t;

// Example function prototypes
HAL_StatusTypeDef ads124_init(void);
HAL_StatusTypeDef ads124_start_conversion(void);
HAL_StatusTypeDef ads124_stop_conversion(void);
HAL_StatusTypeDef ads124_read_conversion(int32_t *data, uint8_t *status);
HAL_StatusTypeDef ads124_configure_channel(uint8_t pos_ch, uint8_t neg_ch);
HAL_StatusTypeDef ads124_set_reference(uint8_t refsel);
HAL_StatusTypeDef ads124_set_vbias(uint8_t vbias_ch, uint8_t vbias_value);
HAL_StatusTypeDef ads124_set_pga_gain(uint8_t gain);
HAL_StatusTypeDef ads124_set_conversion_mode(uint8_t mode);
HAL_StatusTypeDef ads124_set_data_rate(uint8_t data_rate);
HAL_StatusTypeDef ads124_set_low_latency_filter(bool useLowLat);

#endif // ADS124_HANDLER_H
