#include "ads124_buffers.h"
#include "frames.h"

inline void ads124_buffer_can_tx(ads124_buffer_t *buffer, uint8_t SID);

void ads124_buffer_init(ads124_buffer_t *buffer, uint8_t SID) 
{
    buffer->head = 0;
    memset(buffer->data, 0, sizeof(buffer->data));
    buffer->SID = SID; // Set the Sensor ID for CAN transmission
}

bool ads124_buffer_push(ads124_buffer_t *buffer, int16_t value) 
{
    if (buffer->head < BUFFER_SIZE) {
        buffer->data[buffer->head++] = value;

        if (buffer->head >= BUFFER_SIZE) {
            ads124_buffer_can_tx(buffer, buffer->SID); // Send buffer via CAN when full
            buffer->head = 0; // Reset head 
        }
        return true; // Success
    }
    return false; // Buffer full
}

inline void ads124_buffer_can_tx(ads124_buffer_t *buffer, uint8_t SID)
{
    can_send_data(SID, (uint8_t *)buffer->data, BUFFER_SIZE*2);
}

//bool can_send_data(CAN_NodeType nodeType, CAN_NodeAddr nodeAddr, uint8_t *data, uint8_t length)