#ifndef CAN_HANDLER_H
#define CAN_HANDLER_H

#include "stm32g0xx_hal.h"
#include "peripherals.h"
#include "config.h"

// CAN Identifier bit positions
#define CAN_ID_TYPE_SHIFT     8
#define CAN_ID_SRC_SHIFT      5
#define CAN_ID_DEST_SHIFT     2
#define CAN_ID_PRIORITY_SHIFT 0

typedef enum {
    CAN_TYPE_BROADCAST = 0b000,
    CAN_TYPE_ECU       = 0b001,
    CAN_TYPE_SERVO     = 0b010,
    CAN_TYPE_ADC       = 0b011,
    CAN_TYPE_CMD       = 0b100,
    CAN_TYPE_STAT      = 0b101,
    CAN_TYPE_SERVO_POS = 0b110,
    CAN_TYPE_ADC_DATA  = 0b111,
} CAN_MessageType;

typedef struct {
    uint8_t priority   : 2;
    uint8_t destNode   : 3;
    uint8_t srcNode    : 3;
    uint8_t msgType    : 3;
} CAN_ID;

// Pack to 11-bit CAN ID
static inline uint16_t pack_can_id(CAN_ID id) {
    return ((id.msgType  & 0x07) << CAN_ID_TYPE_SHIFT) |
           ((id.srcNode  & 0x07) << CAN_ID_SRC_SHIFT) |
           ((id.destNode & 0x07) << CAN_ID_DEST_SHIFT) |
           ((id.priority & 0x03) << CAN_ID_PRIORITY_SHIFT);
}

// Unpack from 11-bit CAN ID
static inline CAN_ID unpack_can_id(uint16_t std_id) {
    return (CAN_ID) {
        .msgType  = (std_id >> CAN_ID_TYPE_SHIFT) & 0x07,
        .srcNode  = (std_id >> CAN_ID_SRC_SHIFT)  & 0x07,
        .destNode = (std_id >> CAN_ID_DEST_SHIFT) & 0x07,
        .priority = (std_id >> CAN_ID_PRIORITY_SHIFT) & 0x03,
    };
}

void can_init(void);
void can_test_send(void);

#endif // CAN_HANDLER_H