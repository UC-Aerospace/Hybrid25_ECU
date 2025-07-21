#include "handlers.h"
#include "debug_io.h"

void handle_error_warning(CAN_ErrorWarningFrame* frame, CAN_ID id, uint8_t dataLength) {
    dbg_printf("Error Warning: what=%d, why=%d, timestamp=%02X%02X%02X\n",
               frame->what, frame->why, frame->timestamp[0], frame->timestamp[1], frame->timestamp[2]);
}

void handle_command(CAN_CommandFrame* frame, CAN_ID id, uint8_t dataLength) {
    dbg_printf("Command: cmd=%d, param=%d\n",
               frame->cmd, frame->param);
}

void handle_servo_pos(CAN_ServoFrame* frame, CAN_ID id, uint8_t dataLength) {
    dbg_printf("Servo Position: id=%d, position=%d\n",
               id.instance, frame->position);
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