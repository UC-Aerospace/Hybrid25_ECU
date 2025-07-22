#ifndef ADS124_CAN_H
#define ADS124_CAN_H

#include "stm32g0xx_hal.h"
#include "ADS124.h"
#include "peripherals.h"

/* CAN Message IDs for ADS124 data */
#define ADS124_CAN_BASE_ID      0x100
#define ADS124_CAN_HEADER_ID    0x100  // Buffer metadata
#define ADS124_CAN_DATA_ID      0x101  // Sample data

/* CAN message structure for buffer header */
typedef struct {
    uint32_t timestamp;      // 4 bytes
    uint16_t sample_rate;    // 2 bytes  
    uint8_t buffer_id;       // 1 byte
    uint8_t num_samples;     // 1 byte (always 60)
} __attribute__((packed)) ads124_can_header_t;

/* CAN message structure for sample data */
typedef struct {
    uint8_t sequence;        // 1 byte - sequence number (0-29 for 60 samples)
    uint8_t reserved;        // 1 byte - reserved/padding
    uint16_t samples[3];     // 6 bytes - 3 samples per message
} __attribute__((packed)) ads124_can_data_t;

/**
 * @brief Initialize CAN for ADS124 data transmission
 * @param hcan Pointer to CAN handle
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef ads124_can_init(FDCAN_HandleTypeDef *hcan);

/**
 * @brief Transmit ADS124 buffer data over CAN
 * @param hcan Pointer to CAN handle
 * @param buffer Pointer to completed ADS124 buffer
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef ads124_can_transmit_buffer(FDCAN_HandleTypeDef *hcan, ads124_data_buffer_t *buffer);

/**
 * @brief Transmit buffer header information
 * @param hcan Pointer to CAN handle  
 * @param buffer Pointer to completed ADS124 buffer
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef ads124_can_transmit_header(FDCAN_HandleTypeDef *hcan, ads124_data_buffer_t *buffer);

/**
 * @brief Transmit sample data chunk
 * @param hcan Pointer to CAN handle
 * @param samples Pointer to sample data
 * @param sequence Sequence number (0-19 for 60 samples in chunks of 3)
 * @retval HAL_StatusTypeDef
 */
HAL_StatusTypeDef ads124_can_transmit_data_chunk(FDCAN_HandleTypeDef *hcan, uint16_t *samples, uint8_t sequence);

#endif /* ADS124_CAN_H */
