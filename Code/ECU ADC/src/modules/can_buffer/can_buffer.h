#ifndef CAN_BUFFER_H
#define CAN_BUFFER_H

#include "stm32g0xx_hal.h"
#include <stdint.h>
#include <stdbool.h>

#define CAN_BUFFER_SIZE 29

typedef struct {
    int16_t data[CAN_BUFFER_SIZE];
    uint8_t head;
    uint8_t length;
    uint8_t SID;
    uint32_t first_sample_timestamp; // Timestamp of the first sample
    bool enableTX; // Flag to enable/disable CAN transmission
} can_buffer_t;

void can_buffer_init(can_buffer_t *buffer, uint8_t SID, uint8_t length, bool enableTX);
bool can_buffer_push(can_buffer_t *buffer, int16_t value);

#endif // CAN_BUFFER_H