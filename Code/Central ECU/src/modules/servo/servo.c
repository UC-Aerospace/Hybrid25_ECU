#include "servo.h"
#include "can.h"
#include "debug_io.h"

static servo_feedback_t states[4];
static servo_status_u servo_status;

void servo_update(servo_feedback_t feedback[4])
{
    for(int i=0; i<4; i++){
        states[i] = feedback[i];
    }
}

void servo_get_states(servo_feedback_t feedback[4])
{
    for(int i=0; i<4; i++){
        feedback[i] = states[i];
    }
}

void servo_set_position(uint8_t servo_id, servo_positions_t position)
{
    if(servo_id > 3) {
        dbg_printf("SERVO: ERROR, invalid servo ID %d\n", servo_id);
        return;
    }
    dbg_printf("SERVO: Set servo %d to position %d\n", servo_id, position);
    can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, servo_id << 6 | position);
}

void servo_arm_all(void)
{
    dbg_printf("SERVO: ARM ALL\n");
    can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_ARM, 0xFF);
}

void servo_disarm_all(void)
{
    dbg_printf("SERVO: DISARM ALL\n");
    can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_ARM, 0xF0);
}

void servo_print_current_state(void)
{
    dbg_printf("SERVO: Current States:\n");
    dbg_printf("  ID | SetPos | State   | AtSetPos | CurrPos\n");
    dbg_printf(" ----|--------|---------|----------|--------\n");
    dbg_printf("  %d |   %d    |   %d     |    %d     |   %d\n", 0, states[0].setPos, states[0].state, states[0].atSetPos, states[0].currentPos);
    dbg_printf("  %d |   %d    |   %d     |    %d     |   %d\n", 1, states[1].setPos, states[1].state, states[1].atSetPos, states[1].currentPos);
    dbg_printf("  %d |   %d    |   %d     |    %d     |   %d\n", 2, states[2].setPos, states[2].state, states[2].atSetPos, states[2].currentPos);
    dbg_printf("  %d |   %d    |   %d     |    %d     |   %d\n", 3, states[3].setPos, states[3].state, states[3].atSetPos, states[3].currentPos);
}

bool servo_helper_check_all_closed(void) 
{
    for (int i = 0; i < 4; i++) {
        if (states[i].setPos != SERVO_POSITION_CLOSE || states[i].atSetPos == false) {
            return false;
        }
    }
    return true;
}

void servo_status_update(uint8_t main_state, uint8_t substates)
{
    servo_status.raw = (servo_state_t) (main_state << 8 | substates);
}

void servo_status_get(servo_status_u* status)
{
    *status = servo_status;
}