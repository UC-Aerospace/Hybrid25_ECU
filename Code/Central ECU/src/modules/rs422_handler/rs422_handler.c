#include "rs422_handler.h"
#include "debug_io.h"
#include "heartbeat.h"
#include "main_FSM.h"
#include "sequencer.h"

void rs422_handler_init(void) {
    // Initialize RS422 handler
    return;
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
                static uint32_t last_heartbeat_time = 0;
                uint16_t heartbeat_data = frame.data[0] << 8 | (frame.data[1]);
                if (HAL_GetTick() - last_heartbeat_time > 10000) {
                    dbg_printf("RS422: RECV HEARTBEAT (%04X)\r\n", heartbeat_data);
                    last_heartbeat_time = HAL_GetTick();
                }
                heartbeat_reload(BOARD_ID_RIU);
                break;
            case RS422_FRAME_SWITCH_CHANGE:
                // Handle switch change frame
                uint16_t switches = frame.data[0] << 8 | (frame.data[1]);
                dbg_printf("RS422: RECV SWITCH CHANGE (%04X)\r\n", switches);
                fsm_set_switch_states(switches);
                break;
            case RS422_FRAME_VALVE_UPDATE:
                // Handle valve update frame
                dbg_printf("RS422: RECV VALVE UPDATE (%d)\r\n", frame.data[0]);
                break;
            case RS422_BATTERY_VOLTAGE_FRAME:
                // Handle battery voltage frame
                dbg_printf("RS422: RECV BATTERY VOLTAGE (%d)\r\n", frame.data[0]);
                break;
            case RS422_FRAME_SENSOR:
                // Handle sensor frame
                dbg_printf("RS422: RECV SENSOR (%d)\r\n", frame.data[0]);
                break;
            case RS422_FRAME_COUNTDOWN:
                // Handle countdown frame
                dbg_printf("RS422: RECV COUNTDOWN\r\n");
                break;
            case RS422_FRAME_ABORT:
                // Handle abort frame
                dbg_printf("RS422: RECV ABORT (%d)\r\n", frame.data[0]);
                fsm_set_abort(frame.data[0]);
                break;
            case RS422_FRAME_FIRE:
                // Handle fire frame
                static uint32_t last_fire_time = 0;
                if (HAL_GetTick() - last_fire_time < 5000) {
                    dbg_printf("RS422: RECV FIRE - TOO SOON\r\n");
                    break;
                }
                dbg_printf("RS422: RECV FIRE (%d)\r\n", frame.data[0]);
                // Check that packet is correctly formed.
                uint8_t fire_command = frame.data[0];
                if ((fire_command >> 4) == 0b1100) {
                    sequencer_fire(fire_command & 0x0F);
                    last_fire_time = HAL_GetTick();
                } else {
                    dbg_printf("RS422: RECV FIRE - INVALID FIRE COMMAND (%d)\r\n", fire_command);
                }
                break;
            default:
                // Handle unknown frame type
                dbg_printf("RS422: Received unknown frame type %d with size %d\r\n", frame.frame_type, frame.size);
                break;
        }
    }
}
