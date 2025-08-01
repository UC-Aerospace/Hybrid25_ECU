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