#include "ADS124_can.h"
#include <string.h>

/* CAN transmission timeout */
#define ADS124_CAN_TIMEOUT 100

/**
 * @brief Initialize CAN for ADS124 data transmission
 */
HAL_StatusTypeDef ads124_can_init(FDCAN_HandleTypeDef *hcan)
{
    if (hcan == NULL) {
        return HAL_ERROR;
    }
    
    /* Configure CAN filters for ADS124 messages if needed */
    FDCAN_FilterTypeDef sFilterConfig;
    
    sFilterConfig.IdType = FDCAN_STANDARD_ID;
    sFilterConfig.FilterIndex = 0;
    sFilterConfig.FilterType = FDCAN_FILTER_MASK;
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    sFilterConfig.FilterID1 = ADS124_CAN_BASE_ID;
    sFilterConfig.FilterID2 = 0x7F0;  // Mask for base ID
    
    HAL_StatusTypeDef status = HAL_FDCAN_ConfigFilter(hcan, &sFilterConfig);
    if (status != HAL_OK) {
        return status;
    }
    
    /* Start CAN peripheral */
    status = HAL_FDCAN_Start(hcan);
    if (status != HAL_OK) {
        return status;
    }
    
    return HAL_OK;
}

/**
 * @brief Transmit ADS124 buffer data over CAN
 */
HAL_StatusTypeDef ads124_can_transmit_buffer(FDCAN_HandleTypeDef *hcan, ads124_data_buffer_t *buffer)
{
    if (hcan == NULL || buffer == NULL) {
        return HAL_ERROR;
    }
    
    HAL_StatusTypeDef status;
    
    /* First, send the header with buffer metadata */
    status = ads124_can_transmit_header(hcan, buffer);
    if (status != HAL_OK) {
        return status;
    }
    
    /* Then send data in chunks of 3 samples per CAN message */
    /* 60 samples / 3 samples per message = 20 messages */
    for (uint8_t chunk = 0; chunk < 20; chunk++) {
        uint16_t *sample_ptr = &buffer->data[chunk * 3];
        status = ads124_can_transmit_data_chunk(hcan, sample_ptr, chunk);
        if (status != HAL_OK) {
            return status;
        }
        
        /* Small delay between messages to avoid overwhelming the CAN bus */
        HAL_Delay(1);
    }
    
    return HAL_OK;
}

/**
 * @brief Transmit buffer header information
 */
HAL_StatusTypeDef ads124_can_transmit_header(FDCAN_HandleTypeDef *hcan, ads124_data_buffer_t *buffer)
{
    if (hcan == NULL || buffer == NULL) {
        return HAL_ERROR;
    }
    
    FDCAN_TxHeaderTypeDef TxHeader;
    ads124_can_header_t header_data;
    
    /* Configure CAN message header */
    TxHeader.Identifier = ADS124_CAN_HEADER_ID;
    TxHeader.IdType = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType = FDCAN_DATA_FRAME;
    TxHeader.DataLength = FDCAN_DLC_BYTES_8;
    TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
    TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
    TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker = 0;
    
    /* Populate header data */
    header_data.timestamp = buffer->timestamp;
    header_data.sample_rate = buffer->sample_rate;
    header_data.buffer_id = buffer->buffer_id;
    header_data.num_samples = 60;
    
    /* Transmit header message */
    return HAL_FDCAN_AddMessageToTxFifoQ(hcan, &TxHeader, (uint8_t*)&header_data);
}

/**
 * @brief Transmit sample data chunk
 */
HAL_StatusTypeDef ads124_can_transmit_data_chunk(FDCAN_HandleTypeDef *hcan, uint16_t *samples, uint8_t sequence)
{
    if (hcan == NULL || samples == NULL) {
        return HAL_ERROR;
    }
    
    FDCAN_TxHeaderTypeDef TxHeader;
    ads124_can_data_t data_chunk;
    
    /* Configure CAN message header */
    TxHeader.Identifier = ADS124_CAN_DATA_ID;
    TxHeader.IdType = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType = FDCAN_DATA_FRAME;
    TxHeader.DataLength = FDCAN_DLC_BYTES_8;
    TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    TxHeader.BitRateSwitch = FDCAN_BRS_OFF;
    TxHeader.FDFormat = FDCAN_CLASSIC_CAN;
    TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker = 0;
    
    /* Populate data chunk */
    data_chunk.sequence = sequence;
    data_chunk.reserved = 0;
    data_chunk.samples[0] = samples[0];
    data_chunk.samples[1] = samples[1];
    data_chunk.samples[2] = samples[2];
    
    /* Transmit data message */
    return HAL_FDCAN_AddMessageToTxFifoQ(hcan, &TxHeader, (uint8_t*)&data_chunk);
}

/**
 * @brief Example function to integrate with main application
 */
void ads124_can_example_usage(void)
{
    static ads124_handle_t ads124_handle;
    static bool can_initialized = false;
    
    /* Initialize CAN once */
    if (!can_initialized) {
        if (ads124_can_init(&hfdcan1) == HAL_OK) {
            can_initialized = true;
        }
    }
    
    /* Check for completed ADC buffers and transmit over CAN */
    ads124_data_buffer_t *buffer = ads124_get_completed_buffer(&ads124_handle);
    if (buffer != NULL && can_initialized) {
        /* Transmit buffer over CAN */
        if (ads124_can_transmit_buffer(&hfdcan1, buffer) == HAL_OK) {
            /* Successfully transmitted, release buffer */
            ads124_release_buffer(&ads124_handle, buffer);
        }
    }
}

/* Usage Notes:
 * 
 * CAN Message Protocol:
 * 
 * 1. Header Message (ID 0x100):
 *    - Timestamp (4 bytes): System tick when buffer started
 *    - Sample Rate (2 bytes): Samples per second
 *    - Buffer ID (1 byte): 0 or 1 for dual buffering
 *    - Number of Samples (1 byte): Always 60
 * 
 * 2. Data Messages (ID 0x101):
 *    - Sequence (1 byte): Message sequence 0-19
 *    - Reserved (1 byte): Padding/future use
 *    - Samples (6 bytes): Three 16-bit samples
 * 
 * Complete transmission sequence:
 * - 1 header message (8 bytes)
 * - 20 data messages (8 bytes each)
 * - Total: 168 bytes per buffer (60 samples)
 * 
 * Receiving End:
 * - Listen for header message to get buffer info
 * - Collect 20 data messages in sequence
 * - Reconstruct 60 samples using sequence numbers
 * - Use timestamp and sample rate for time correlation
 * 
 * Error Handling:
 * - Check sequence numbers for missing messages
 * - Implement timeout for incomplete buffers
 * - Consider implementing message acknowledgment if needed
 */
