#include "manual_valve.h"
#include "can.h"
#include "main_FSM.h"
#include "debug_io.h"

valve_states_t valve_state = VALVE_DISARMED;
struct ServoFeedback servo_feedback = {false};

// Helpers
void manual_valve_send_arm(void)
{
    can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_ARM, 0xFF); // FIXME: confirm format
}

void manual_valve_send_disarm(void)
{
    can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_ARM, 0xF0);
}

static void manual_valve_send_positions(void){
    // Go through the servo_feedback and send a CAN command if different to switch position
    if(!servo_feedback.initialised) return;
    if(servo_feedback.servoPosCommandedVent != switch_snapshot.valve_discharge){
        can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 0 << 6 | switch_snapshot.valve_discharge);
    }
    if(servo_feedback.servoPosCommandedNitrogen != switch_snapshot.valve_nitrogen){
        can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 1 << 6 | switch_snapshot.valve_nitrogen);
    }
    if(servo_feedback.servoPosCommandedNosA != switch_snapshot.valve_nitrous_a){
        can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 2 << 6 | switch_snapshot.valve_nitrous_a);
    }
    if(servo_feedback.servoPosCommandedNosB != switch_snapshot.valve_nitrous_b){
        can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 3 << 6 | switch_snapshot.valve_nitrous_b);
    }
}

void valve_set_servo_feedback_position(bool servoSetPosition[4])
{
    servo_feedback.initialised = true;
    servo_feedback.servoPosCommandedVent = servoSetPosition[0];
    servo_feedback.servoPosCommandedNitrogen = servoSetPosition[1];
    servo_feedback.servoPosCommandedNosA = servoSetPosition[2];
    servo_feedback.servoPosCommandedNosB = servoSetPosition[3];
}

// bool valve_get_servo_position(void)
// {
//     //Array with servo posiotions (1 is open, 0 is closed). Order is vent, nitrogen, nos A and nos B
//     bool servoCurrentPos[4] = { 0 };

//     servoCurrentPos[0] = servo_feedback.servoPosCommandedVent;
//     servoCurrentPos[1] = servo_feedback.servoPosCommandedNitrogen;
//     servoCurrentPos[2] = servo_feedback.servoPosCommandedNosA;
//     servoCurrentPos[3] = servo_feedback.servoPosCommandedNosB;

//     return servoCurrentPos;
// }

void valve_state_decoder(void)
{
    bool override_on = switch_snapshot.sequencer_override;
    bool valve_master = switch_snapshot.master_valve && override_on; // require override

    if(!override_on){
        // Global collapse
        if(valve_state != VALVE_DISARMED) { 
            manual_valve_send_disarm(); 
            dbg_printf("Valve not disarmed, disarming\n");
        }
        valve_state = VALVE_DISARMED; 
        return;
    }

    switch(valve_state){
        case VALVE_DISARMED:

            if(valve_master) { 
                manual_valve_send_arm();
                dbg_printf("Valve master switch on, arming valves and moving to valve armed state\n");
                valve_state = VALVE_ARMED;
            }
            break;

        case VALVE_ARMED:
            if(!valve_master) { 
                manual_valve_send_disarm();
                dbg_printf("Valve master switch off, disarming valves and moving to valve disarmed state\n");
                valve_state = VALVE_DISARMED;
            }
            else { 
                manual_valve_send_positions();
                dbg_printf("Sending valve positions\n");
            }
            break;

        default: 
            valve_state = VALVE_DISARMED;
            dbg_printf("Valve state decoder defaulted, moving to valve disarmed state\n");
            break;
    }
}