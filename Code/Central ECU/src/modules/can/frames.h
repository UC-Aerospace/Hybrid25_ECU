#ifndef FRAMES_H
#define FRAMES_H

#include <stdint.h>

// CAN Identifier bit positions
#define CAN_ID_FRAME_TYPE_SHIFT  0
#define CAN_ID_NODE_ADDR_SHIFT   3
#define CAN_ID_NODE_TYPE_SHIFT   6
#define CAN_ID_PRIORITY_SHIFT    9

typedef enum {
    CAN_TYPE_ERROR = 0b000,
    CAN_TYPE_COMMAND = 0b001,
    CAN_TYPE_STATUS = 0b010,
    CAN_TYPE_SERVO_POS = 0b011,
    CAN_TYPE_HEARTBEAT = 0b100,
    // 0b101
    // 0b110
    CAN_TYPE_ADC_DATA = 0b111
} CAN_MessageType;

typedef enum {
    CAN_PRIORITY_CRITICAL = 0b00, // Priority 0
    CAN_PRIORITY_HEARTBEAT = 0b01, // Priority 1
    CAN_PRIORITY_COMMAND = 0b10, // Priority 2
    CAN_PRIORITY_DATA = 0b11  // Priority 3
} CAN_Priority;

typedef enum {
    CAN_NODE_TYPE_BROADCAST = 0b000, // Central node
    CAN_NODE_TYPE_CENTRAL = 0b001, // Servo node
    CAN_NODE_TYPE_SERVO = 0b010, // ECU node
    CAN_NODE_TYPE_ADC = 0b011, // Sensor node
} CAN_NodeType;

typedef enum {
    CAN_NODE_ADDR_CENTRAL = 0b001, // Central node address
    CAN_NODE_ADDR_SERVO = 0b010, // Servo node address
    CAN_NODE_ADDR_ADC_1 = 0b011, // ADC1 node address
    CAN_NODE_ADDR_ADC_2 = 0b100, // ADC2 node address
} CAN_NodeAddr;

typedef enum {
    CAN_ERROR_ACTION_SHUTDOWN = 0b00, // Shutdown action
    CAN_ERROR_ACTION_ERROR = 0b01, // Error action
    CAN_ERROR_ACTION_WARNING = 0b10, // Warning action
} CAN_ErrorAction;

typedef struct {
    uint8_t priority    : 2;
    uint8_t nodeType    : 3;
    uint8_t nodeAddr    : 3;
    uint8_t frameType   : 3;
} CAN_ID;

typedef struct {
    CAN_ID id;
    uint8_t length;
    uint8_t data[64]; // 64 bytes per message
} CAN_Frame_t;

typedef struct __attribute__((packed)) {
    uint8_t what;
    uint8_t why;
    uint8_t timestamp[3];
} CAN_ErrorWarningFrame;

typedef struct __attribute__((packed)) {
    uint8_t what;
    uint8_t options;
    uint8_t timestamp[3];
} CAN_CommandFrame;

typedef struct __attribute__((packed)) {
    uint8_t what;
    uint8_t state;
    uint8_t substate;
    uint8_t timestamp[3];
} CAN_StatusFrame;

typedef struct __attribute__((packed)) {
    uint8_t what;
    uint8_t pos[4];      // Up to 4 servos
    uint8_t timestamp[3];
} CAN_ServoPosFrame;

typedef struct __attribute__((packed)) {
    uint8_t what;      
    uint8_t timestamp[3];
} CAN_HeartbeatFrame;

typedef struct __attribute__((packed)) {
    uint8_t what;
    uint8_t timestamp[3];
    uint8_t data[60];     // 60 bytes of ADC data
} CAN_ADCFrame;

// Pack to 11-bit CAN ID
static inline uint16_t pack_can_id(CAN_ID id) {
    return ((id.frameType  & 0x07) << CAN_ID_FRAME_TYPE_SHIFT) |
           ((id.nodeAddr  & 0x07) << CAN_ID_NODE_ADDR_SHIFT) |
           ((id.nodeType & 0x07) << CAN_ID_NODE_TYPE_SHIFT) |
           ((id.priority & 0x03) << CAN_ID_PRIORITY_SHIFT);
}

// Unpack from 11-bit CAN ID
static inline CAN_ID unpack_can_id(uint16_t std_id) {
    return (CAN_ID) {
        .frameType  = (std_id >> CAN_ID_FRAME_TYPE_SHIFT) & 0x07,
        .nodeAddr  = (std_id >> CAN_ID_NODE_ADDR_SHIFT)  & 0x07,
        .nodeType = (std_id >> CAN_ID_NODE_TYPE_SHIFT) & 0x07,
        .priority = (std_id >> CAN_ID_PRIORITY_SHIFT) & 0x03,
    };
}

#endif // FRAMES_H