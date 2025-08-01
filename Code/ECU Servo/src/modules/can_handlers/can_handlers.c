#include "can_handlers.h"
#include "debug_io.h"

void handle_error_warning(CAN_ErrorWarningFrame* frame, CAN_ID id, uint8_t dataLength) {
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

void handle_command(CAN_CommandFrame* frame, CAN_ID id, uint8_t dataLength) {
    dbg_printf("Command: cmd=%d, param=%d %d %d %d\n", frame->what, frame->options[0], frame->options[1],
               frame->options[2], frame->options[3]);

    uint8_t command = frame->what >> 3;
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who

    switch (command) {
        case CAN_CMD_UPDATE_STATE:
            dbg_printf("Update State Command: initiator=%d, options=%d %d %d %d\n",
                       initiator, frame->options[0], frame->options[1], frame->options[2], frame->options[3]);
            // Handle update state command
            break;
        case CAN_CMD_SERVO_ARM:
            dbg_printf("Servo Arm Command: initiator=%d, bitwise arm=%d\n",
                       initiator, frame->options[0]);
            // Handle servo arm command
            break;
        case CAN_CMD_SERVO_MOVE:
            uint8_t servoID = frame->options[0] >> 6; // Bits 6-7 for servo ID
            bool setFine = frame->options[0] >> 5 & 0x01; // Bit 5 for fine control
            uint8_t position = frame->options[0] & 0x1F; // Bits 0-4 for position
            dbg_printf("Servo Move Command: initiator=%d, servoID=%d, setFine=%d, position=%d\n",
                       initiator, servoID, setFine, position);
            // Handle servo move command
        
    }
    
}

#ifdef BOARD_TYPE_SERVO

void handle_error_warning(CAN_ErrorWarningFrame* frame, CAN_ID id, uint8_t dataLength) {
    uint8_t action_type = frame->what >> 6; // Bits 6-7 for action type
    uint8_t initiator = frame->what & 0x07; // Bits 0-2 for who
    dbg_printf("Error Warning: action_type=%d, initiator=%d, why=%d, timestamp=%02X%02X%02X\n",
               action_type, initiator, frame->why, frame->timestamp[0], frame->timestamp[1], frame->timestamp[2]);
}

void handle_servo_pos(CAN_ServoFrame* frame, CAN_ID id, uint8_t dataLength) {
    dbg_printf("Servo Position: who = %d, pos1 = %d, pos2 = %d, pos3 = %d, pos4 = %d\n",
               frame->what & 0x07, frame->pos[0], frame->pos[1], frame->pos[2], frame->pos[3]);
}

void handle_status(CAN_StatusFrame* frame, CAN_ID id, uint8_t dataLength) {
    dbg_printf("Status: who=%d, global state=%d, sub state=%d\n",
               frame->what & 0x07, frame->state >> 4, frame->state & 0x0F);
}

void handle_adc_data(CAN_ADCFrame* frame, CAN_ID id, uint8_t dataLength) {
    dbg_printf("CAN got ADC data: what=%d", frame->what);
}

void handle_cmd_servo_arm(CAN_CommandFrame* frame, CAN_ID id, uint8_t dataLength) {

    /*
    Frame format:
    BYTE 2:
        Arm change and state

        XXXXYYYY

        X: Bitwise change arm
        Y: Bitwise arm state
    */

    uint8_t which_servo = frame->options[0] >> 4; // Bits 0-2 for which servo
    uint8_t servo_action = frame->options[0] & 0x0F; // Bits 3-5 for action
    for (int i = 0; i < 4; i++) {
        if (which_servo & (3-i)) {
            if (servo_action & (3-i)) {
                dbg_printf("Servo %d: Arm\n", i+1);
                servo_arm(servoByIndex[i]);
            } else {
                dbg_printf("Servo %d: Disarm\n", i+1);
                servo_disarm(servoByIndex[i]);
            }
        }
    }
}

void handle_cmd_servo_move(CAN_CommandFrame* frame, CAN_ID id, uint8_t dataLength) {
    
    /*
    Frame format:
    BYTE 2:
    Servo and position

    XXYZZZZZ

    X: Which servo (0-3)
    Y: How to set (0 by list, 1 manual)
    Z: If by List [0-3]
        0 -> Close
        1 -> Open
        2 -> Crack
        3 -> Safe
    Z: If by manual [0-20]
        0 = 0 deg, 10 = 90 deg, 20 = 180 deg
        Formula will be Z*50 to get 0-1000
    */

    uint8_t servo_index = frame->options[0] >> 6; // Bits 0-1 for which servo
    uint8_t set_type = (frame->options[0] >> 5) & 0x01; // Bits 2 for how to set
    uint8_t position = frame->options[0] & 0x1F; // Bits 3-7 for position
    dbg_printf("Servo Move Command: servo_index=%d, set_type=%d, position=%d\n",
               servo_index, set_type, position);
    
    if (set_type == 0) {
        // Set by list
        ServoPosition servo_position;
        switch (position) {
            case 0:
                servo_position = CLOSE;
                break;
            case 1:
                servo_position = OPEN;
                break;
            case 2:
                servo_position = CRACK;
                break;
            case 3:
                servo_set_safe(servoByIndex[servo_index]);
                return; // Safe position, no need to set
            default:
                dbg_printf("Invalid position for servo %d: %d\n", servo_index, position);
                return; // Invalid position
        }
        servo_set_position(servoByIndex[servo_index], servo_position);
    } else {
        // Set by manual angle
        uint16_t angle = position * 50; // Convert to angle
        servo_set_angle(servoByIndex[servo_index], angle);
    }
    
}

#endif

#ifdef BOARD_TYPE_CENTRAL

void handle_error_warning(CAN_ErrorWarningFrame* frame, CAN_ID id, uint8_t dataLength) {
    dbg_printf("Error Warning: what=%d, why=%d, timestamp=%02X%02X%02X\n",
               frame->what, frame->why, frame->timestamp[0], frame->timestamp[1], frame->timestamp[2]);
}



void handle_servo_pos(CAN_ServoFrame* frame, CAN_ID id, uint8_t dataLength) {
    dbg_printf("Servo Position: id=%d, position=%d\n",
               frame->what, frame->pos[0]);
}

void handle_status(CAN_StatusFrame* frame, CAN_ID id, uint8_t dataLength) {
    dbg_printf("Status: what=%d, state=%d\n",
               frame->what, frame->state);
}

void handle_adc_data(CAN_ADCFrame* frame, CAN_ID id, uint8_t dataLength) {

    typedef struct {
        uint8_t sensorType; // 0 = pressure, 1 = temperature, 2 = pressure+temperature, 3 = load cell
        uint8_t sensorSubID; // Together with type makes UID
        uint8_t sensorUID;
        uint16_t sampleRate;
        uint16_t dataLength
    } CAN_ADC_FrameInfo;

    // Extract information from the ADC frame info
    CAN_ADC_FrameInfo info;
    info.sensorType = frame->what >> 6; // Bits 6-7 for sensor type
    info.sensorSubID = frame->what >> 3 & 0x07; // Bits 3-5 for sensor sub ID
    info.sensorUID = frame->what >> 3; // Both sensor type and sub ID
    static const uint16_t sampleRateLookup[8] = {1, 10, 20, 50, 100, 200, 500, 1000};
    info.sampleRate = sampleRateLookup[frame->what & 0x07]; // Bits 0-2 for sample rate


    dbg_printf("ADC Data: what=%d, value=%d\n",
               frame->what, frame->data[0]); // Assuming data[0] holds the ADC value
}

void handle_cmd_servo_move(CAN_CommandFrame* frame, CAN_ID id, uint8_t dataLength)
{
    return;
}

void handle_cmd_servo_arm(CAN_CommandFrame* frame, CAN_ID id, uint8_t dataLength)
{
    return;
}

#endif