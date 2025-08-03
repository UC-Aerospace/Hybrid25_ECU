#include "can_handlers.h"
#include "debug_io.h"
#include "can.h"

static void handle_cmd_set_servo_arm(CAN_CommandFrame* frame, CAN_ID id);
static void handle_cmd_set_servo_pos(CAN_CommandFrame* frame, CAN_ID id);
static void handle_cmd_get_servo_pos(CAN_CommandFrame* frame, CAN_ID id);
static void handle_cmd_get_voltage(CAN_CommandFrame* frame, CAN_ID id);
static void handle_cmd_restart_mcu(CAN_CommandFrame* frame, CAN_ID id);

CAN_RxQueue can_rx_queue = {
    .frames = {0},
    .head = 0,
    .tail = 0,
    .count = 0
};

// =========================================================
// Helper functions
// =========================================================

void enqueue_can_frame(CAN_Frame_t* frame) {
    if (can_rx_queue.count >= CAN_RX_QUEUE_LENGTH) {
        dbg_printf("CAN RX Queue is full, dropping frame\n");
        return; // Queue is full, drop the frame
    }
    
    can_rx_queue.frames[can_rx_queue.tail] = *frame;
    can_rx_queue.tail = (can_rx_queue.tail + 1) % CAN_RX_QUEUE_LENGTH;
    can_rx_queue.count++;
    
    dbg_printf("Enqueued CAN frame with ID: %08lX, count: %d\n", frame->id, can_rx_queue.count);
}

void can_handler_poll(void) {
    if (can_rx_queue.count == 0) {
        dbg_printf("No CAN frames to process\n");
        return; // No frames to process
    }

    CAN_Frame_t *frame = &can_rx_queue.frames[can_rx_queue.head];
    can_rx_queue.head = (can_rx_queue.head + 1) % CAN_RX_QUEUE_LENGTH;
    can_rx_queue.count--;

    dbg_printf("Processing CAN frame with ID: %08lX, count: %d\n", frame->id, can_rx_queue.count);

    switch (frame->id.frameType) {
        case CAN_TYPE_ERROR:
            handle_error_warning((CAN_ErrorWarningFrame*)frame->data, frame->id);
            break;
        case CAN_TYPE_COMMAND:
            handle_command((CAN_CommandFrame*)frame->data, frame->id);
            break;
        case CAN_TYPE_SERVO_POS:
            handle_servo_pos((CAN_ServoFrame*)frame->data, frame->id);
            break;
        case CAN_TYPE_ADC_DATA:
            handle_adc_data((CAN_ADCFrame*)frame->data, frame->id, frame->length);
            break;
        case CAN_TYPE_STATUS:
            handle_status((CAN_StatusFrame*)frame->data, frame->id);
            break;
        default:
            dbg_printf("Unknown CAN frame type: %d\n", frame->id.frameType);
            break;
    }
}

// =========================================================
// Frame handlers
// =========================================================

void handle_error_warning(CAN_ErrorWarningFrame* frame, CAN_ID id) {
    dbg_printf("Error Warning: what=%d, why=%d, timestamp=%02X%02X%02X\n",
               frame->what, frame->why, frame->timestamp[0], frame->timestamp[1], frame->timestamp[2]);

    uint8_t actionType = frame->what >> 6; // Bits 6-7 for action type
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for initiator
    
    switch (actionType) {
        case 0b00: // Immediate shutdown
            dbg_printf("Immediate Shutdown: initiator=%d, why=%d\n", initiator, frame->why);
            // Handle immediate shutdown
            break;
        case 0b01: // Error Notification
            dbg_printf("Error Notification: initiator=%d, why=%d\n", initiator, frame->why);
            // Handle error notification
            break;
        case 0b10: // Warning Notification
            dbg_printf("Warning Notification: initiator=%d, why=%d\n", initiator, frame->why);
            // Handle warning notification
            break;
        case 0b11: // Reserved
            dbg_printf("Error Warning 0b11: initiator=%d, why=%d\n", initiator, frame->why);
            // Handle reserved message
            break;
    }
    
}

void handle_command(CAN_CommandFrame* frame, CAN_ID id) {
    dbg_printf("Command: cmd=%d, param=%d\n", frame->what, frame->options);

    uint8_t command = frame->what >> 3;
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who

    switch (command) {
        case CAN_CMD_SET_STATE:
            dbg_printf("Update State Command: initiator=%d, options=%d\n",
                       initiator, frame->options);
            // Handle update state command
            break;
        
        case CAN_CMD_SET_SERVO_ARM:
            handle_cmd_set_servo_arm(frame, id);
            break;
        
        case CAN_CMD_SET_SERVO_POS:
            handle_cmd_set_servo_pos(frame, id);
            break;

        case CAN_CMD_GET_SERVO_POS:
            handle_cmd_get_servo_pos(frame, id);
            break;

        case CAN_CMD_GET_VOLTAGE:
            handle_cmd_get_voltage(frame, id);
            break;

        case CAN_CMD_RESTART_MCU:
            handle_cmd_restart_mcu(frame, id);
            break;

        case CAN_CMD_SET_SENSOR_RATE:
            dbg_printf("Set Sensor Rate Command: initiator=%d, options=%d\n",
                       initiator, frame->options);
            break;

        case CAN_CMD_SET_SENSOR_STATE:
            dbg_printf("Set Sensor State Command: initiator=%d, options=%d\n",
                       initiator, frame->options);
            break;
    }
}

void handle_servo_pos(CAN_ServoFrame* frame, CAN_ID id) {
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who
    //TODO: Handle this
}

void handle_adc_data(CAN_ADCFrame* frame, CAN_ID id, uint8_t dataLength) {
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who
    //TODO: Handle this
}

void handle_status(CAN_StatusFrame* frame, CAN_ID id) {
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who
    //TODO: Handle this
}

void handle_heatbeat(CAN_HeartbeatFrame* frame, CAN_ID id, uint32_t timestamp) {
    // Handle heartbeat messages
    // RxTimestamp not used at this stage as far more accurate than SysTick and rolls over often
    // Remote timestamp good upto ~4 hours
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who
    uint32_t remote_timestamp = (frame->timestamp[0] << 16) | (frame->timestamp[1] << 8) | frame->timestamp[2];
    uint32_t local_timestamp = SysTick->VAL;
    dbg_printf("Heartbeat Frame: initiator=%d, remote timestamp=%lu, local timestamp=%lu\n", initiator, remote_timestamp, local_timestamp);
    
}

// ==========================================================
// Command handlers
// ==========================================================

static void handle_cmd_set_servo_arm(CAN_CommandFrame* frame, CAN_ID id) {
    // ECU has no reason to handle this
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who
    dbg_printf("Set Servo Arm Command: initiator=%d, options=%d\n", initiator, frame->options);
}

static void handle_cmd_set_servo_pos(CAN_CommandFrame* frame, CAN_ID id) {
    // Also no reason to handle this
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who
    dbg_printf("Set Servo Position Command: initiator=%d, options=%d\n", initiator, frame->options);
}

static void handle_cmd_get_servo_pos(CAN_CommandFrame* frame, CAN_ID id) {
    // Likewise
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who
    dbg_printf("Get Servo Position Command: initiator=%d\n", initiator);
}

static void handle_cmd_get_voltage(CAN_CommandFrame* frame, CAN_ID id) {
    // For now this will be unused on all boards
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who
    dbg_printf("Get Voltage Command: initiator=%d\n", initiator);
}