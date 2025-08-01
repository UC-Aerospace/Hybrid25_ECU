#include "rs422_handler.h"
#include "debug_io.h"

void rs422_handler_init(void) {
    // Initialize RS422 handler
}

void rs422_handler_rx_poll(void) {
    // Poll for RS422 reception
    // This function should be called periodically to process incoming RS422 frames
    // It can be used to check the RX buffer and handle received frames accordingly
    // For example, you might want to read from the RS422_RX_BUFFER and process complete frames
    RS422_RxFrame_t frame;
    uint16_t bytes_read = rs422_read(&frame);
    if (bytes_read > 0) {
        // Process the received frame based on its type
        switch (frame.frame_type) {
            case RS422_FRAME_HEARTBEAT:
                // Handle heartbeat frame
                dbg_printf("Received heartbeat frame with size %d\r\n", frame.size);
                break;
            case RS422_FRAME_ARM_UPDATE:
                // Handle arm update frame
                dbg_printf("Received arm update frame with size %d\r\n", frame.size);
                break;
            case RS422_FRAME_VALVE_UPDATE:
                // Handle valve update frame
                dbg_printf("Received valve update frame with size %d\r\n", frame.size);
                break;
            case RS422_FRAME_TIME_SYNC:
                // Handle time sync frame
                dbg_printf("Received time sync frame with size %d\r\n", frame.size);
                break;
            case RS422_FRAME_PRESSURE:
                // Handle pressure frame
                dbg_printf("Received pressure frame with size %d\r\n", frame.size);
                break;
            case RS422_FRAME_PRESSURE_TEMPERATURE:
                // Handle pressure and temperature frame
                dbg_printf("Received pressure and temperature frame with size %d\r\n", frame.size);
                break;
            case RS422_FRAME_TEMPERATURE:
                // Handle temperature frame
                dbg_printf("Received temperature frame with size %d\r\n", frame.size);
                break;
            case RS422_FRAME_LOAD_CELL:
                // Handle load cell frame
                dbg_printf("Received load cell frame with size %d\r\n", frame.size);
                break;
            case RS422_FRAME_ABORT:
                // Handle abort frame
                dbg_printf("Received abort frame with size %d\r\n", frame.size);
                break;
            case RS422_FRAME_WARNING:
                // Handle warning frame
                dbg_printf("Received warning frame with size %d\r\n", frame.size);
                break;
            case RS422_FRAME_FIRE:
                // Handle fire frame
                dbg_printf("Received fire frame with size %d\r\n", frame.size);
                break;
            default:
                // Handle unknown frame type
                dbg_printf("Received unknown frame type %d with size %d\r\n", frame.frame_type, frame.size);
                break;
        }
    } 
}

void rx422_handler_parse_heartbeat(RS422_RxFrame_t *frame) {
    // Parse heartbeat frame
    // This function can be used to extract specific data from the heartbeat frame
    // For example, you might want to extract a timestamp or status information
    dbg_printf("Parsing heartbeat frame with size %d\r\n", frame->size);
}
