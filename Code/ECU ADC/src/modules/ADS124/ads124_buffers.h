#ifndef ADS124_BUFFERS_H
#define ADS124_BUFFERS_H

#include <stdint.h>
#include <stdbool.h>
#include "can.h"

#define BUFFER_SIZE 29

typedef struct {
    int16_t data[BUFFER_SIZE];
    uint8_t head;
    uint8_t SID;
} ads124_buffer_t;

void ads124_buffer_init(ads124_buffer_t *buffer, uint8_t SID);
bool ads124_buffer_push(ads124_buffer_t *buffer, int16_t value);

#endif // ADS124_BUFFERS_H