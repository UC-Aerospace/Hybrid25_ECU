#ifndef CAN_HANDLERS_H
#define CAN_HANDLERS_H

#include "stm32g0xx_hal.h"
#include "frames.h"
#include "can.h"

#define CAN_RX_QUEUE_LENGTH 100

void handle_error_warning(CAN_ErrorWarningFrame* frame, CAN_ID id);
void handle_command(CAN_CommandFrame* frame, CAN_ID id);
void handle_servo_pos(CAN_ServoPosFrame* frame, CAN_ID id);
void handle_adc_data(CAN_ADCFrame* frame, CAN_ID id, uint8_t dataLength);
void handle_status(CAN_StatusFrame* frame, CAN_ID id);
void handle_heartbeat(CAN_HeartbeatFrame* frame, CAN_ID id, uint32_t timestamp);
void enqueue_can_frame(CAN_Frame_t* frame);
void can_handler_poll(void);
void handle_tx_error(uint32_t error);
void CAN_Error_Handler(void);

typedef struct {
    CAN_Frame_t frames[CAN_RX_QUEUE_LENGTH];
    uint8_t head; // Points to the next message to read
    uint8_t tail; // Points to the next message to write
    uint8_t count; // Number of messages in the queue
} CAN_RxQueue;

extern CAN_RxQueue can_rx_queue;

#endif // CAN_HANDLERS_H