#ifndef FRAMES_H
#define FRAMES_H

#include <stdint.h>

typedef struct __attribute__((packed)) {
    uint8_t what;
    uint8_t why;
    uint8_t timestamp[3];
} CAN_ErrorWarningFrame;

typedef struct __attribute__((packed)) {
    uint8_t what;
    uint8_t options[4];
    uint8_t timestamp[3];
} CAN_CommandFrame;

typedef struct __attribute__((packed)) {
    uint8_t what;
    uint8_t state;
    uint8_t timestamp[3];
} CAN_StatusFrame;

typedef struct __attribute__((packed)) {
    uint8_t what;
    uint8_t pos[4];      // Up to 4 servos
    uint8_t timestamp[3];
} CAN_ServoFrame;

typedef struct __attribute__((packed)) {
    uint8_t what;
    uint8_t timestamp[3];
    uint8_t data[60];     // 60 bytes of ADC data
} CAN_ADCFrame;

#endif // FRAMES_H