#include "stager.h"
#include "heartbeat.h"

stager_state_t current_state;
stager_switch_state_t current_switch_state = {0};

bool stager_init(void) 
{
    // Initialize the state machine
    stager_set_state(STATE_INIT);
    // TODO: Maybe do some sort of safe checking here?
    return true;
}

stager_state_t stager_get_state(void) 
{
    // Get the current state
    return current_state;
}

void stager_set_state(stager_state_t state) 
{
    // Set the current state
    current_state = state;
}

void stager_process(void)
{
    switch (current_state) {
        case STATE_INIT:
            if (heartbeat_all_started()) {
                stager_set_state(STATE_READY);
            }
            break;
        case STATE_READY:
            if (current_switch_state.changed_since_last) {

                // Check if the sequencer is overridden. If so go into manual mode and wait.
                if (current_switch_state.sequencer_override) {
                    stager_set_state(STATE_MANUAL_MODE);
                }

                // Check if both the pyro and valve are armed
                if (current_switch_state.master_pyro && current_switch_state.master_valve) {
                    stager_set_state(STATE_SEQUENCER);
                }
            }
            break;
        case STATE_SEQUENCER:
            // Handle sequencer state
            break;
        case STATE_POST_FIRE:
            // Handle post-fire state
            break;
        case STATE_MANUAL_MODE:
            // Handle manual mode state
            break;
        case STATE_ERROR:
            // Handle error state
            break;
        case STATE_ABORT:
            // Handle abort state
            break;
    }
}

void stager_set_switches(uint16_t switches) {
    // Set the switch states based on the provided 16-bit value
    current_switch_state.master_power = (switches & (1 << 15)) != 0;
    current_switch_state.master_valve = (switches & (1 << 14)) != 0;
    current_switch_state.master_pyro = (switches & (1 << 13)) != 0;
    current_switch_state.sequencer_override = (switches & (1 << 12)) != 0;
    current_switch_state.valve_nitrous_a = (switches & (1 << 7)) != 0;
    current_switch_state.valve_nitrous_b = (switches & (1 << 6)) != 0;
    current_switch_state.valve_nitrogen = (switches & (1 << 5)) != 0;
    current_switch_state.valve_discharge = (switches & (1 << 4)) != 0;
    current_switch_state.solenoid = (switches & (1 << 3)) != 0;

    current_switch_state.changed_since_last = true;
}