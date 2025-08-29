#include "sequencer.h"
#include "main_FSM.h"
#include "stm32g0xx_hal.h"
#include "debug_io.h"
#include "spicy.h"
#include "can.h"
#include "heartbeat.h"
#include "manual_valve.h"

#define DEFAULT_BURN_TIME 6000u

//Countdown times
#define COUNTDOWN_MS 10000u //Must be greater than or equal to 10000u (ten seconds). Change this to change the countdown time
#define COUNTDOWN_TM10_MS COUNTDOWN_MS - 9999u //If COUNTDOWN_MS is 20000u -> 20000u - 10000u = 10000u from fire button pressed, or 10 seconds from ignition. 9999 used otherwise you get an always true error
#define COUNTDOWN_TM6_MS COUNTDOWN_MS - 6000u
#define COUNTDOWN_TM4_MS COUNTDOWN_MS - 4000u
#define COUNTDOWN_TM1_MS COUNTDOWN_MS - 1000u

//Countup times
uint32_t burn_time = DEFAULT_BURN_TIME; //Default burn time of 6 seconds
#define COUNTUP_T25_MS 19000u //25 seconds for 6 second burn
#define COUNTUP_T16_MS 10000u //16 seconds for 6 second burn
#define COUNTUP_T15_MS 9000u //15 seconds for 6 second burn
#define COUNTUP_T10_MS 4000u //10 seconds for 6 second burn
#define COUNTUP_T9_MS 9000u //9 seconds for 6 second burn
#define COUNTUP_T8_MS 2000u //8 seconds for 6 second burn
// uint32_t countup_t25_ms = 19000u; //Default post burn times 
// uint32_t countup_t16_ms = 10000u;
// uint32_t countup_t15_ms = 9000u;
// uint32_t countup_t10_ms = 4000u;
// uint32_t countup_t9_ms = 3000u;
// uint32_t countup_t8_ms = 2000u;

bool fire = false;
static bool has_burn_time_been_set = false;
static bool checks_good = true;

typedef enum {
    SEQUENCER_READY = 0b0000,
    SEQUENCER_COUNTDOWN = 0b0001,
    SEQUENCER_FIRE = 0b0010,
    SEQUENCER_FAILED_START = 0b1111,
    SEQUENCER_UNINITIALISED = 0b1110
} sequencer_states_t;

static sequencer_states_t sequencer_state = SEQUENCER_UNINITIALISED;
static uint32_t sequencer_start_tick = 0u;
static uint32_t fire_in_one_start_tick = 0u;
static uint32_t fire_start_tick = 0u;
static bool first_time_flag = true;
static bool first_time_set = true;

/*
// Declerations
*/
static void countdown_valve_set(void);
static void close_both_nitrous(void);
static void open_nitrogen(void);
static void close_nitrogen(void);
static void open_vent(void);
static void close_vent(void);

bool set_burn_time(uint32_t new_burn_time)
{
    burn_time = new_burn_time;
    return true;
}

void uninitialise_sequencer(void)
{
    fire = false;
    sequencer_state = SEQUENCER_UNINITIALISED;
}

void set_sequencer_ready(void)
{
    sequencer_start_tick = 0u;
    fire_in_one_start_tick = 0u;
    fire_start_tick = 0u;
    first_time_flag = true;
    checks_good = true;
    first_time_set = true;

    // Arm all servos
    can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_ARM, 0xFF);

    // Arm pyro
    spicy_arm();

    //If burn time has not been set externally, set burntime to avoid very large problems -> not shutting off burn
    if (!has_burn_time_been_set) {
        has_burn_time_been_set = set_burn_time(DEFAULT_BURN_TIME);
    }

    sequencer_state = SEQUENCER_READY;
    dbg_printf("Sequencer ready\n");
}

void sequencer_tick(void)
{
    switch(sequencer_state){
        case SEQUENCER_READY: // Ready and waiting for fire
        fire = true; //#TODO: For testing ----- remove otherwise bad things will happen!!!!!!!!!! -----
            if (!both_armed()) {
                sequencer_state = SEQUENCER_FAILED_START;
                dbg_printf("Both switches disarmed, moving to sequencer failed start\n");

            } else if (fire) {
                sequencer_state = SEQUENCER_COUNTDOWN;
                sequencer_start_tick = HAL_GetTick();
                dbg_printf("Fire button pushed, contdown begun\n");
            }
            break;
        
        case SEQUENCER_COUNTDOWN: // Fun things are about to happen, make sure we are ready
            if(!prefire_ok()) {
                sequencer_state = SEQUENCER_FAILED_START;
                dbg_printf("Sequencer not safe, failed start\n");

            } else if (HAL_GetTick() - fire_in_one_start_tick >= 1000u && first_time_flag == false) { //I know this is a magic number but fire_in_one should always be 1 second
                spicy_open_solenoid(); // Start burn
                fire_start_tick = HAL_GetTick();
                sequencer_state = SEQUENCER_FIRE;
                dbg_printf("Sequencer fire, candle lit\n");

            } else if (HAL_GetTick() - sequencer_start_tick >= COUNTDOWN_TM1_MS) { //T-1
                //If heartbeat good #TODO: check logging and ematch continuity
                // if (!heartbeat_all_started()) {
                //     checks_good = false;
                //     dbg_printf("T-1, aborted due to bad heartbeat\n");
                //     fsm_set_state(STATE_ABORT);
                // }

                if (first_time_flag) {
                    fire_in_one_start_tick = HAL_GetTick();
                    first_time_flag = false;
                    dbg_printf("T-1, fire in one tick set\n");
                }

                if (checks_good) {
                    dbg_printf("T-1, all checks good\n");
                }

            } else if (HAL_GetTick() - sequencer_start_tick >= COUNTDOWN_TM4_MS) { //T-4
                //Fire ematch 1
                spicy_fire_ematch1();
                dbg_printf("T-4, ematch 1 fired\n");

            } else if (HAL_GetTick() - sequencer_start_tick >= COUNTDOWN_TM6_MS) { //T-6
                //Check system states good (sorry for the immense amount of if statements, will clean into functions once working)
                //Heartbeat
                // if (!heartbeat_all_started()) {
                //     checks_good = false;
                //     dbg_printf("T-6, aborted due to bad heartbeat\n");
                //     fsm_set_state(STATE_ABORT);
                // }

                // //Solenoid position - should be closed at this point
                // if (spicy_get_solenoid()) {
                //     checks_good = false;
                //     dbg_printf("T-6, aborted due to open solenoid\n");
                //     fsm_set_state(STATE_ABORT);
                // }

                // //Valve positions - should be vent closed, nitrogen closed, nos A open, nos B open
                // if (servo_feedback.servoPosCommandedVent != 0) {
                //     checks_good = false;
                //     dbg_printf("T-6, aborted due to open vent\n");
                //     fsm_set_state(STATE_ABORT);
                // }

                // if (servo_feedback.servoPosCommandedNitrogen != 0) {
                //     checks_good = false;
                //     dbg_printf("T-6, aborted due to open nitrogen\n");
                //     fsm_set_state(STATE_ABORT);
                // }

                // if (servo_feedback.servoPosCommandedNosA != 1) {
                //     checks_good = false;
                //     dbg_printf("T-6, aborted due to closed nos A\n");
                //     fsm_set_state(STATE_ABORT);
                // }

                // if (servo_feedback.servoPosCommandedNosB != 1) {
                //     checks_good = false;
                //     dbg_printf("T-6, aborted due to closed nos B\n");
                //     fsm_set_state(STATE_ABORT);
                // }

                //#TODO: Pressures high enough and within range of each other

                //#TODO: SD card logging good

                if (checks_good) {
                    dbg_printf("T-6, all checks good\n");
                }

                dbg_printf("Valves: %d %d %d %d\n", servo_feedback.servoPosCommandedVent, servo_feedback.servoPosCommandedNitrogen, servo_feedback.servoPosCommandedNosA, servo_feedback.servoPosCommandedNosB);
                // sequencer_state = SEQUENCER_UNINITIALISED;

            } else if (HAL_GetTick() - sequencer_start_tick >= COUNTDOWN_TM10_MS) { //T-10
                //Open the nitrous valves, close the vent and close the nitrogen
                if (first_time_set) {
                    countdown_valve_set();
                    // can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 2 << 6 | 1);
                    // can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 3 << 6 | 1);
                    spicy_arm();
                
                    dbg_printf("T-10, opened nitrous valves, closed nitrogen and vent, armed spicy\n");
                    first_time_set = false;
                }
            }
            break;
        
        case SEQUENCER_FIRE: // We are cooking now!
            if (HAL_GetTick() - fire_start_tick >= COUNTUP_T25_MS + burn_time) { //T+25
                //Vent close
                if (servo_feedback.servoPosCommandedVent != 0) {
                    close_vent();
                }
                // close_vent();
                // can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 0 << 6 | 0);

                //Pyro disarm
                spicy_disarm();

                // Disarm all servos
                can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_ARM, 0xF0);

                dbg_printf("T+25, vent closed, pyro disarmed, all servos disarmed, moving to post fire state\n");

                fsm_set_state(STATE_POST_FIRE);

            } else if (HAL_GetTick() - fire_start_tick >= COUNTUP_T16_MS + burn_time) { //T+16
                //Vent open
                if (servo_feedback.servoPosCommandedVent != 1) {
                    open_vent();
                }
                // open_vent();
                // can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 0 << 6 | 1);

                dbg_printf("T+16, vent opened\n");

            } else if (HAL_GetTick() - fire_start_tick >= COUNTUP_T15_MS + burn_time) { //T+15
                spicy_close_solenoid();
                //Nitrogen closed
                if (servo_feedback.servoPosCommandedNitrogen != 0) {
                    close_nitrogen();
                }
                // close_nitrogen();
                // can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 1 << 6 | 0);

                dbg_printf("T+15, nitrogen and solenoid closed\n");

            } else if (HAL_GetTick() - fire_start_tick >= COUNTUP_T10_MS + burn_time) { //T+10
                //Vent close
                if (servo_feedback.servoPosCommandedVent != 0) {
                    close_vent();
                }
                // close_vent();
                // can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 0 << 6 | 0);

                dbg_printf("T+10, vent closed\n");

            } else if (HAL_GetTick() - fire_start_tick >= COUNTUP_T9_MS + burn_time) { //T+9
                //Nitrogen open
                if (servo_feedback.servoPosCommandedNitrogen != 1) {
                    open_nitrogen();
                }
                // open_nitrogen();
                // can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 1 << 6 | 1);

                dbg_printf("T+9, nitrogen open\n");

            } else if (HAL_GetTick() - fire_start_tick >= COUNTUP_T8_MS + burn_time) { //T+8
                //Vent open
                if (servo_feedback.servoPosCommandedVent != 1) {
                    open_vent();
                }
                // can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 0 << 6 | 1);

                dbg_printf("T+8, vent open\n");

            } else if (HAL_GetTick() - fire_start_tick >= burn_time) { //T+6
                //Close solenoid
                spicy_close_solenoid();

                //Close both nitrous bottles
                if (servo_feedback.servoPosCommandedNosA != 0 || servo_feedback.servoPosCommandedNosB != 0) {
                    close_both_nitrous();
                }
                // can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 2 << 6 | 0);
                // can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 3 << 6 | 0);

                dbg_printf("T+6, burn finished, closing solenoid and both nitrous bottles\n");
            } else {
                //Check heartbeat
                // if (!heartbeat_all_started()) {
                //     checks_good = false;
                //     dbg_printf("Aborted burn due to bad heartbeat\n");
                //     fsm_set_state(STATE_ABORT);
                // }

                //#TODO: Check chamber pressures

                if (checks_good) {
                    dbg_printf("Combustion is happening! Time since ignition: %d seconds\n", (HAL_GetTick() - fire_start_tick) / 1000);
                }
            }
            break;
        
        case SEQUENCER_FAILED_START: // Something went wrong
            outputs_safe();
            fsm_set_state(STATE_ERROR);
            dbg_printf("Sequencer failed start, secured system and moved to error state\n");
            break;

        case SEQUENCER_UNINITIALISED: // Not ready to fire
            dbg_printf("Sequencer uninitialised\n");
            break;

        default: // How did we even get here???
            sequencer_state = SEQUENCER_READY;
            dbg_printf("Sequencer defaulted, moving to sequencer ready\n");
            break;
    }
}

static void countdown_valve_set(void)
{
    //Set the valve positions at the start of the countdown sequence
    //Vent - Closed
    can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 0 << 6 | 0);

    //Nitrogen - Closed
    can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 1 << 6 | 0);

    //Nitrous A - Open
    can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 2 << 6 | 1);

    //Nitrous B - Open
    can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 3 << 6 | 1);
}

static void close_both_nitrous(void)
{
    can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 2 << 6 | 0);
    can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 3 << 6 | 0);
}

static void open_nitrogen(void)
{
    can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 1 << 6 | 1);
}

static void close_nitrogen(void)
{
    can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 1 << 6 | 0);
}

static void open_vent(void)
{
    can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 0 << 6 | 1);
}

static void close_vent(void)
{
    can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_POS, 0 << 6 | 0);
}