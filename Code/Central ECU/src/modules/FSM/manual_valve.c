#include "manual_valve.h"

#include "debug_io.h"
#include "servo.h"

manual_valve_state_t valve_state = VALVE_DISARMED;

static servo_feedback_t servo_feedback[4];

static void manual_valve_send_positions(switch_state_t *switch_snapshot){
    // Go through the servo_feedback and send a CAN command if different to switch position
    servo_get_states(servo_feedback);
    if(servo_feedback[VALVE_VENT].setPos != (switch_snapshot->valve_discharge ? SERVO_POSITION_OPEN : SERVO_POSITION_CLOSE)){
        servo_set_position(VALVE_VENT, switch_snapshot->valve_discharge ? SERVO_POSITION_OPEN : SERVO_POSITION_CLOSE);
    }
    if(servo_feedback[VALVE_NITROGEN].setPos != (switch_snapshot->valve_nitrogen ? SERVO_POSITION_OPEN : SERVO_POSITION_CLOSE)){
        servo_set_position(VALVE_NITROGEN, switch_snapshot->valve_nitrogen ? SERVO_POSITION_OPEN : SERVO_POSITION_CLOSE);
    }
    if(servo_feedback[VALVE_NOS_A].setPos != (switch_snapshot->valve_nitrous_a ? SERVO_POSITION_OPEN : SERVO_POSITION_CLOSE)){
        servo_set_position(VALVE_NOS_A, switch_snapshot->valve_nitrous_a ? SERVO_POSITION_OPEN : SERVO_POSITION_CLOSE);
    }
    if(servo_feedback[VALVE_NOS_B].setPos != (switch_snapshot->valve_nitrous_b ? SERVO_POSITION_OPEN : SERVO_POSITION_CLOSE)){
        servo_set_position(VALVE_NOS_B, switch_snapshot->valve_nitrous_b ? SERVO_POSITION_OPEN : SERVO_POSITION_CLOSE);
    }
}

void manual_valve_tick(switch_state_t *switch_snapshot)
{
    bool override_on = switch_snapshot->sequencer_override;
    bool valve_master = switch_snapshot->master_valve && override_on; // require override

    switch(valve_state){
        case VALVE_DISARMED:

            if(valve_master) { 
                servo_arm_all();
                dbg_printf("MANUAL VALVE: ARM\n");
                valve_state = VALVE_ARMED;
            }
            break;

        case VALVE_ARMED:
            if(!valve_master) { 
                servo_disarm_all();
                dbg_printf("MANUAL VALVE: DISARM\n");
                valve_state = VALVE_DISARMED;
            }
            else { 
                manual_valve_send_positions(switch_snapshot);
            }
            break;

        default: 
            valve_state = VALVE_DISARMED;
            dbg_printf("MANUAL VALVE: ERROR, DEFAULT\n");
            break;
    }
}

void manual_valve_set_safe(void)
{
    valve_state = VALVE_DISARMED;
    servo_disarm_all();
    dbg_printf("MANUAL VALVE: SET SAFE\n");
}