#include "can_handlers.h"
#include "debug_io.h"
#include "can.h"
#include "sd_log.h"
#include "rs422.h"
#include "stager.h"
#include "config.h"

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

void handle_servo_pos(CAN_ServoPosFrame* frame, CAN_ID id) {
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who
    // Ignore connected servos, assume 4

    bool servoSetBinary[4] = { false };
    uint8_t servoSetPos[4] = { 0 };
    uint8_t servoState[4] = { 0 };
    bool atSetPos[4] = { false };
    uint8_t servoCurrentPos[4] = { 0 };

    for (int i = 0; i < 4; i++) {
        servoSetBinary[i] = (frame->set_pos[i] >> 6) & 0x01; // Bit 6 for open/close
        servoSetPos[i] = frame->set_pos[i] & 0x1F; // Bits 0-4 for position
    }

    stager_set_servo_feedback_position(servoSetBinary);

    for (int i = 0; i < 4; i++) {
        servoState[i] = (frame->current_pos[i] >> 6);
        atSetPos[i] = (frame->current_pos[i] & 0x20) != 0; // Bit 5 for at set position
        servoCurrentPos[i] = (frame->current_pos[i] & 0x1F);
    }

    uint8_t valve_pos = 0;
    bool any_servo_error = false;
    bool any_servo_armed = false;
    for (int i = 0; i < 4; i++) {
        valve_pos |= servoSetPos[i] << (7 - i);
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

    // Serial print some stuff
    
    if (sensorID < 2) {
        uint16_t first_sample = (frame->data[0]) | (frame->data[1] << 8);
        // output =>
        // map first_sample such that 0.1 * reference is the zero output level and 0.9 * reference is 0xFFFF
        uint16_t reference = (frame->data[56]) | (frame->data[57] << 8);
        uint32_t in_min = reference / 10;         // 0.1 * reference
        uint32_t in_max = (reference * 9) / 10;   // 0.9 * reference

        if (first_sample <= in_min) first_sample = 0x0000;
        else if (first_sample >= in_max) first_sample = 0xFFFF;
        else {
            uint32_t range = in_max - in_min;
            uint32_t scaled = (first_sample - in_min) * 65535UL / range;
            first_sample = (uint16_t)scaled;
        }
        dbg_printf_nolog("%d %d\n", sensorID, first_sample);
    } else if (sensorID == 16 | sensorID == 17 | sensorID == 18) {
        int16_t first_sample = (frame->data[0]) | (frame->data[1] << 8);
        dbg_printf_nolog("%d %d\n", sensorID, first_sample);
    }
    

    // Write chunk to per-sensor file with delimiter header
    bool status = sd_log_write_sensor_chunk(sensorID, sampleRate, frame->data, frame->length, frame->timestamp);
    if (!status) {
        dbg_printf("SD Card failed write for sensor %d\n", sensorID);
        HAL_GPIO_WritePin(LED_IND_ERROR_GPIO_Port, LED_IND_ERROR_Pin, GPIO_PIN_SET); // Set error LED
        // TODO: Issue a warning over RS422
        return;
    }
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
    uint32_t local_timestamp = HAL_GetTick();
    // TODO: Reenable the prints here for synchronisation
    //dbg_printf("Heartbeat Frame: initiator=%u, remote timestamp=%lu, local timestamp=%lu\n", initiator, remote_timestamp, local_timestamp);
    
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

static void handle_cmd_restart_mcu(CAN_CommandFrame* frame, CAN_ID id) {
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who
    dbg_printf("Restart MCU Command: initiator=%d\n", initiator);
    
    // Perform a software reset
    // TODO: To implement this must tie in with state machine
    // Should flush the SD card and issue an abort to peripheral boards
    // Also really the central ECU shouldn't be being reset by peripheral boards
    // Should implement on RS422 though
    //NVIC_SystemReset();
}