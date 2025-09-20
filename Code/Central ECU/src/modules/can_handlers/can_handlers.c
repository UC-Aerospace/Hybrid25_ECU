#include "can_handlers.h"
#include "debug_io.h"
#include "can.h"
#include "sd_log.h"
#include "rs422.h"
#include "manual_valve.h"
#include "config.h"
#include "servo.h"
#include "heartbeat.h"
#include "error_def.h"
#include "sensors.h"

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

void CAN_Error_Handler(void)
{
    static uint32_t last_notify = 0;
    uint32_t now = HAL_GetTick();
    dbg_printf("CAN: Error occurred!\n");
    if (now - last_notify > 2000) { // Notify at most once every 2 seconds, prevent flooding bus
        last_notify = now;
        can_send_error_warning(CAN_NODE_TYPE_BROADCAST, CAN_NODE_ADDR_BROADCAST, CAN_ERROR_ACTION_ERROR, GENERIC_CAN_ERROR);
        // Also notify over RS422
        uint8_t action = CAN_ERROR_ACTION_ERROR << 6 | BOARD_ID_ECU;
        rs422_send_error_warning(action, GENERIC_CAN_ERROR);
    }
}

void handle_tx_error(uint32_t error_code) {
    static uint32_t last_notify = 0;
    uint32_t now = HAL_GetTick();
    if (now - last_notify > 2000) { // Notify at most once every 2 seconds, prevent flooding bus
        last_notify = now;
        can_send_error_warning(CAN_NODE_TYPE_BROADCAST, CAN_NODE_ADDR_BROADCAST, CAN_ERROR_ACTION_ERROR, GENERIC_CAN_ERROR);
    }
    // Log the error
    dbg_printf("CAN TX Error: 0x%08lX\n", error_code);

    // Also notify over RS422
    uint8_t action = CAN_ERROR_ACTION_ERROR << 6 | BOARD_ID_ECU;
    rs422_send_error_warning(action, ECU_ERROR_CAN_TX_FAIL);
}

void enqueue_can_frame(CAN_Frame_t* frame) {
    if (can_rx_queue.count >= CAN_RX_QUEUE_LENGTH) {
        dbg_printf("CAN RX Queue is full, dropping frame\n");
        return; // Queue is full, drop the frame
    }
    
    can_rx_queue.frames[can_rx_queue.tail] = *frame;
    can_rx_queue.tail = (can_rx_queue.tail + 1) % CAN_RX_QUEUE_LENGTH;
    can_rx_queue.count++;
}

void can_handler_poll(void) {
    for (uint8_t i = 0; i < CAN_MAX_QUEUE_PROCESS; i++) {
        
        if (can_rx_queue.count == 0) {
            return; // No frames to process
        }

        CAN_Frame_t *frame = &can_rx_queue.frames[can_rx_queue.head];
        can_rx_queue.head = (can_rx_queue.head + 1) % CAN_RX_QUEUE_LENGTH;
        can_rx_queue.count--;

        switch (frame->id.frameType) {
            case CAN_TYPE_ERROR:
                handle_error_warning((CAN_ErrorWarningFrame*)frame->data, frame->id);
                break;
            case CAN_TYPE_COMMAND:
                handle_command((CAN_CommandFrame*)frame->data, frame->id);
                break;
            case CAN_TYPE_SERVO_POS:
                handle_servo_pos((CAN_ServoPosFrame*)frame->data, frame->id);
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
}

// =========================================================
// Frame handlers
// =========================================================

void handle_error_warning(CAN_ErrorWarningFrame* frame, CAN_ID id) {
    dbg_printf("Error Warning: what=%d, why=%d, timestamp=%02X%02X%02X\n",
               frame->what, frame->why, frame->timestamp[0], frame->timestamp[1], frame->timestamp[2]);

    uint8_t actionType = frame->what >> 6; // Bits 6-7 for action type
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for initiator

    // Send through RS422 as well
    rs422_send_error_warning(frame->what, frame->why);
    
    switch (actionType) {
        case 0b00: // Immediate shutdown
            dbg_printf("Immediate Shutdown: initiator=%d, why=%d\n", initiator, frame->why);
            fsm_set_abort(frame->why); // Set FSM to error state with code
            break;
        case 0b01: // Error Notification
            dbg_printf("Error Notification: initiator=%d, why=%d\n", initiator, frame->why);
            fsm_raise_error(frame->why); // Raise error in FSM
            break;
        case 0b10: // Warning Notification
            dbg_printf("Warning Notification: initiator=%d, why=%d\n", initiator, frame->why);
            rs422_send_error_warning(frame->what, frame->why); // Just forward warning
            // Just log, unless it is the servo board restarting while we are in sequence, and then scrub
            if (initiator == BOARD_ID_SERVO && fsm_get_state() == STATE_SEQUENCER && frame->why == SERVO_WARNING_STARTUP) {
                dbg_printf("Scrubbing servo state due to power cycle on servo board\n");
                fsm_set_abort(frame->why);
            }
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

void handle_servo_pos(CAN_ServoPosFrame* frame, CAN_ID id) {
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who
    // Ignore connected servos, assume 4

    servo_feedback_t servo_feedback[4];

    servo_positions_t servoSetList[4];
    uint8_t servoSetPos[4] = { 0 };
    uint8_t servoState[4] = { 0 };
    bool atSetPos[4] = { false };
    uint8_t servoCurrentPos[4] = { 0 };

    for (int i = 0; i < 4; i++) {
        servoSetList[i] = ((frame->set_pos[i] >> 6) & 0x03); // Bit 6-7 for pos
        servoSetPos[i] = frame->set_pos[i] & 0x1F; // Bits 0-4 for fine set position

        servo_feedback[i].setPos = servoSetList[i];
    }

    for (int i = 0; i < 4; i++) {
        servoState[i] = (frame->current_pos[i] >> 6);
        atSetPos[i] = (frame->current_pos[i] & 0x20) != 0; // Bit 5 for at set position
        servoCurrentPos[i] = (frame->current_pos[i] & 0x1F);

        servo_feedback[i].state = servoState[i];
        servo_feedback[i].currentPos = servoCurrentPos[i];
        servo_feedback[i].atSetPos = atSetPos[i];
    }

    servo_update(servo_feedback);

    uint8_t valve_pos = 0;
    bool any_servo_error = false;
    bool any_servo_armed = false;
    for (int i = 0; i < 4; i++) {
        valve_pos |= (uint8_t)(servoSetList[i] != 0 ? 1 : 0) << (7 - i);
        any_servo_armed |= (servoState[i] == 1);
        any_servo_error |= (servoState[i] == 3);
    }
    valve_pos |= any_servo_armed << 3 | any_servo_error << 2;

    rs422_send_valve_position(valve_pos);
}

static uint8_t can_dlc_to_bytes(uint8_t dlc_code) {
    // Convert CAN-FD DLC code (0..15) to number of bytes
    if (dlc_code <= 8) return dlc_code;
    switch (dlc_code) {
        case 9:  return 12;
        case 10: return 16;
        case 11: return 20;
        case 12: return 24;
        case 13: return 32;
        case 14: return 48;
        case 15: return 64;
        default: return 0;
    }
}

void handle_adc_data(CAN_ADCFrame* frame, CAN_ID id, uint8_t dataLength) {
    // Bits 3-7 for sensor ID. Bits 6-7 are sensor type and bits 3-5 for sub ID.
    // Sensor types:
    //  00 - Pressure
    //  01 - Temperature
    //  10 - Pressure + Temperature
    //  11 - Load cell
    uint8_t sensorID = frame->what >> 3;
    // Sample rate, 3 bits. (0-7) = 1, 10, 20, 50, 100, 200, 500, 1000Hz
    uint8_t sampleRate = frame->what & 0x07;

    
    if (sensorID <= SENSOR_P_MANIFOLD) { // Pressure sensor
        uint16_t first_sample = (frame->data[0]) | (frame->data[1] << 8);
        uint16_t reference = (frame->data[56]) | (frame->data[57] << 8);
        sensors_add_pressure(sensorID, first_sample, reference);
    } else if ((sensorID >= SENSOR_THERMO_A) && (sensorID <= SENSOR_THERMO_C)) { // Thermocouples
        int16_t first_sample = (frame->data[0]) | (frame->data[1] << 8);
        sensors_add_temperature(sensorID, first_sample);
    } else if ((sensorID >= SENSOR_PT_MAINLINE) && (sensorID <= SENSOR_PT_BRANCH_B)) { // PT sensors
        int16_t first_sample = (frame->data[0]) | (frame->data[1] << 8);
        sensors_add_pt(sensorID, first_sample);
    }
    

    // Write chunk to per-sensor file with delimiter header
    bool status = sd_log_write_sensor_chunk(frame, frame->length + 5);
    if (!status) {
        dbg_printf("SD Card failed write for sensor %d\n", sensorID);
        uint8_t action = CAN_ERROR_ACTION_ERROR << 6 | BOARD_ID_ECU;
        rs422_send_error_warning(action, ECU_ERROR_SD_DATA_WRITE_FAIL);
        fsm_raise_error(ECU_ERROR_SD_DATA_WRITE_FAIL);
        return;
    }

    // Send to the RIU
    rs422_send_data((uint8_t*)frame, frame->length+5, RS422_FRAME_SENSOR);

}

void handle_status(CAN_StatusFrame* frame, CAN_ID id) {
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who
    if (initiator == BOARD_ID_SERVO) {
        // Update servo status
        servo_status_update(frame->state, frame->substate);
        dbg_printf("Status Frame: initiator=SERVO, main_state=%d, substate=%d, remote_timestamp=%lu\n",
                   frame->state, frame->substate, frame->timestamp[0] << 16 | frame->timestamp[1] << 8 | frame->timestamp[2]);
    } else if (initiator == BOARD_ID_ADC_A) {
        // Could update ADC board status here if needed
    } else {
        dbg_printf("Status Frame: initiator=%d, main_state=%d, substate=%d\n",
                   initiator, frame->state, frame->substate);
    }
}

void handle_heartbeat(CAN_HeartbeatFrame* frame, CAN_ID id, uint32_t local_timestamp) {
    // Handle heartbeat messages
    // RxTimestamp not used at this stage as far more accurate than SysTick and rolls over often
    // Remote timestamp good upto ~4 hours
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who
    uint32_t remote_timestamp = (frame->timestamp[0] << 16) | (frame->timestamp[1] << 8) | frame->timestamp[2];

    if (initiator == BOARD_ID_ADC_A) {
        static uint32_t last_adc_a_heartbeat = 0;
        if (HAL_GetTick() - last_adc_a_heartbeat > 10000) {
            dbg_printf("Heartbeat Frame: initiator=ADC_A, remote timestamp=%lu, local timestamp=%lu\n", remote_timestamp, local_timestamp);
            last_adc_a_heartbeat = HAL_GetTick();
        }
    } else if (initiator == BOARD_ID_SERVO) {
        static uint32_t last_servo_heartbeat_print = 0;
        if (HAL_GetTick() - last_servo_heartbeat_print > 10000) {
            dbg_printf("Heartbeat Frame: initiator=SERVO, remote timestamp=%lu, local timestamp=%lu\n", remote_timestamp, local_timestamp);
            last_servo_heartbeat_print = HAL_GetTick();
        }
    } else {
        dbg_printf("Heartbeat Frame: initiator=%u, remote timestamp=%lu, local timestamp=%lu\n", initiator, remote_timestamp, local_timestamp);
    }

    heartbeat_reload(initiator);

}

// ==========================================================
// Command handlers
// ==========================================================

static void handle_cmd_set_servo_arm(CAN_CommandFrame* frame, CAN_ID id) {
    // ECU has no reason to handle this
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who
    dbg_printf("CAN: Set Servo Arm Command, initiator=%d, options=%d\n", initiator, frame->options);
}

static void handle_cmd_set_servo_pos(CAN_CommandFrame* frame, CAN_ID id) {
    // Also no reason to handle this
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who
    dbg_printf("CAN: Set Servo Position Command, initiator=%d, options=%d\n", initiator, frame->options);
}

static void handle_cmd_get_servo_pos(CAN_CommandFrame* frame, CAN_ID id) {
    // Likewise
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who
    dbg_printf("CAN: Get Servo Position Command, initiator=%d\n", initiator);
}

static void handle_cmd_get_voltage(CAN_CommandFrame* frame, CAN_ID id) {
    // For now this will be unused on all boards
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who
    dbg_printf("CAN: Get Voltage Command, initiator=%d\n", initiator);
}

static void handle_cmd_restart_mcu(CAN_CommandFrame* frame, CAN_ID id) {
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who
    dbg_printf("CAN: Restart MCU Command, initiator=%d\n", initiator);
}