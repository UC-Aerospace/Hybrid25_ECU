#ifndef CAN_HANDLERS_H
#define CAN_HANDLERS_H

#include "stm32g0xx_hal.h"
#include "frames.h"
#include "can.h"

#define CAN_RX_QUEUE_LENGTH 5

void handle_error_warning(CAN_ErrorWarningFrame* frame, CAN_ID id);
void handle_command(CAN_CommandFrame* frame, CAN_ID id);
void handle_servo_pos(CAN_ServoFrame* frame, CAN_ID id);
void handle_adc_data(CAN_ADCFrame* frame, CAN_ID id, uint8_t dataLength);
void handle_status(CAN_StatusFrame* frame, CAN_ID id);
void handle_heatbeat(CAN_HeartbeatFrame* frame, CAN_ID id, uint32_t timestamp);
void enqueue_can_frame(CAN_Frame_t* frame);
void can_handler_poll(void);

typedef struct {
    CAN_Frame_t frames[CAN_RX_QUEUE_LENGTH];
    uint8_t head; // Points to the next message to read
    uint8_t tail; // Points to the next message to write
    uint8_t count; // Number of messages in the queue
} CAN_RxQueue;

// Command enum
typedef enum {
    CAN_CMD_SET_STATE = 0b0000,
    CAN_CMD_SET_SERVO_ARM = 0b0001,
    CAN_CMD_SET_SERVO_POS = 0b0010,
    CAN_CMD_GET_SERVO_POS = 0b0011,
    CAN_CMD_GET_VOLTAGE = 0b0100,
    CAN_CMD_RESTART_MCU = 0b0101,
    CAN_CMD_SET_SENSOR_RATE = 0b0110,
    CAN_CMD_SET_SENSOR_STATE = 0b0111
    // Add more commands as needed
} CommandType;

extern CAN_RxQueue can_rx_queue;

#endif // CAN_HANDLERS_H