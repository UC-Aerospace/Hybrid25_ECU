#include "can_buffer.h"
#include "can.h"

static inline void can_buffer_tx(can_buffer_t *buffer, uint8_t SID);

void can_buffer_init(can_buffer_t *buffer, uint8_t SID, uint8_t length) 
{
    buffer->head = 0;
    memset(buffer->data, 0, sizeof(buffer->data));
    buffer->SID = SID; // Set the Sensor ID for CAN transmission
    buffer->length = length;
}

bool can_buffer_push(can_buffer_t *buffer, int16_t value) 
{
    if (buffer->head < buffer->length) {
        if (buffer->head == 0) {
            buffer->first_sample_timestamp = HAL_GetTick(); // Store timestamp of the first sample
        }
        buffer->data[buffer->head++] = value;

        if (buffer->head >= buffer->length) {
            can_buffer_tx(buffer, buffer->SID); // Send buffer via CAN when full
            buffer->head = 0; // Reset head 
        }
        return true; // Success
    }
    return false; // Buffer full
}

static inline void can_buffer_tx(can_buffer_t *buffer, uint8_t SID)
{
    can_send_data(SID, (uint8_t *)buffer->data, buffer->length*2, buffer->first_sample_timestamp);
}

//bool can_send_data(CAN_NodeType nodeType, CAN_NodeAddr nodeAddr, uint8_t *data, uint8_t length)