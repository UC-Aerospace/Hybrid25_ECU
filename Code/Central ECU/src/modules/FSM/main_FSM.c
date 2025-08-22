#include "main_FSM.h"

typedef enum {
    STATE_INIT = 0b0000,
    STATE_READY = 0b0001,
    STATE_SEQUENCER = 0b0010,
    STATE_POST_FIRE = 0b0011,
    STATE_MANUAL_MODE = 0b1000,
    STATE_ERROR = 0b1110,
    STATE_ABORT = 0b1111
} main_states_t;

//==============================
// Internal state variables
//==============================
static volatile main_states_t current_state = STATE_INIT;
// static volatile main_states_t previous_state = STATE_INIT;

//Functions to write: ------ TODO ------
    //Set state
    //Get state
        //Maybe, only need this if other modules need access to the main states
    //Decode state
        //Switch case where actions are performed based on current state
    //FSM init

void set_state(main_states_t new_state)
{
    //Call exit function for previous (current at this point) state

    //Change current_state to new_state

    //Call entry function for current_state
}

void FSM_tick(void)
{
    //TODO: Write logic to run everytime main loop calls this function
}

void fsm_set_switch_states(uint16_t switches)
{
    //TODO: Write setting logic
}

