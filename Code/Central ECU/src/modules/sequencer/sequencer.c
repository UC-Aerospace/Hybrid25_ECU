#include "sequencer.h"
#include "main_FSM.h"
#include "debug_io.h"
#include "spicy.h"
#include "can.h"
#include "heartbeat.h"
#include "manual_valve.h"
#include "rs422.h"
#include "servo.h"
#include "rs422.h"
#include "error_def.h"
#include "sensors.h"

#define COUNTDOWN_START_MS 20000

static sequencer_states_t sequencer_state = SEQUENCER_UNINITIALISED;
static uint32_t sequencer_start_tick = 0u;
static uint8_t task_list_index = 0;
static uint16_t burn_time = 6000; // Default burn time if not set by RS422 command

// Static declarations
static void countdown_tasks(void);
static void fire_tasks(void);

static int32_t get_countdown(void)
{
    return HAL_GetTick() - (sequencer_start_tick + COUNTDOWN_START_MS);
}

void sequencer_set_state(sequencer_states_t new_state)
{
    dbg_printf("SEQ: State change %d -> %d\n", sequencer_state, new_state);
    sequencer_state = new_state;
    task_list_index = 0;
    if (new_state == SEQUENCER_COUNTDOWN) {
        sequencer_start_tick = HAL_GetTick();
    }
}

sequencer_states_t sequencer_get_state(void)
{
    return sequencer_state;
}

void sequencer_fire(uint8_t length)
{
    if (length == 0) length = 6;
    if (sequencer_state == SEQUENCER_READY && length <= 10) {
        char msg[6];
        snprintf(msg, sizeof(msg), "CFL:%d", length);
        rs422_send_string_message(msg, strlen(msg));
        dbg_printf("SEQ: Fire button pushed, countdown begun (burn len = %ds)\n", length);
        sequencer_set_state(SEQUENCER_COUNTDOWN);
        burn_time = length * 1000; // Convert to ms
    }
}

void sequencer_tick(void)
{
    switch(sequencer_state){

        case SEQUENCER_READY: // Ready and waiting for fire
            // Currently do nothing
            break;
        case SEQUENCER_COUNTDOWN: // Fun things are about to happen, make sure we are ready
            rs422_send_countdown(get_countdown()/1000);
            countdown_tasks();
            break;
        case SEQUENCER_FIRE: // We are cooking now!
            rs422_send_countdown(get_countdown()/1000);
            fire_tasks();
            break;
        case SEQUENCER_FAILED_START: // Something went wrong
            // Also do nothing here, just waiting for a switch change to pull us back to ready in main FSM
            break;
        case SEQUENCER_UNINITIALISED: // Not ready to fire
            // Again, whole lot of nothing. Shouldn't get here now
            break;
        default: // How did we even get here???
            sequencer_state = SEQUENCER_FAILED_START;
            dbg_printf("Sequencer defaulted, moving to failed start\n");
            break;
    }
}

typedef void (*sequencer_task_fn)(void);

typedef struct sequencer_task {
    sequencer_task_fn task;
    int32_t execution_ms;
} sequencer_task_t;

static void seq_task_tm10_arm(void);
static void seq_task_tm9_nos_a_b(void);
static void seq_task_tm6_checks(void);
static void seq_task_tm5_softarm(void);
static void seq_task_tm3_ignition(void);
static void seq_task_tm2_ign_off(void);
static void seq_task_tm0_solenoid(void);

// Tasks must be in order they will be executed in
static const sequencer_task_t countdown_task_list[] = {
    { seq_task_tm10_arm, -10000 },
    { seq_task_tm9_nos_a_b, -9000 },
    { seq_task_tm6_checks, -6000 },
    { seq_task_tm5_softarm, -5000 },
    { seq_task_tm3_ignition, -3000 },
    { seq_task_tm2_ign_off, -2000 },
    { seq_task_tm0_solenoid, 0 },
};

static void countdown_tasks(void) 
{
    if (get_countdown() >= countdown_task_list[task_list_index].execution_ms) {
        countdown_task_list[task_list_index++].task();
    }
}

static void seq_task_tm10_arm(void)
{
    dbg_printf("SEQ: T-10 Arming all systems\n");
    servo_arm_all();
}

static void seq_task_tm9_nos_a_b(void)
{
    dbg_printf("SEQ: T-9 Opening NOS A and B\n");
    servo_set_position(VALVE_NOS_A, SERVO_POSITION_OPEN);
    servo_set_position(VALVE_NOS_B, SERVO_POSITION_OPEN);
}

static void seq_task_tm6_checks(void)
{
    bool checks_good = true;

    // Should check:
    // 1) Reported valve positions are as expected
    //    NOS A and B open
    //    Nitrogen closed
    //    Vent closed
    //    Discharge closed
    servo_feedback_t servos[4];
    servo_get_states(servos);
    if (servos[VALVE_NOS_A].setPos != SERVO_POSITION_OPEN || !servos[VALVE_NOS_A].atSetPos) {
        dbg_printf("SEQ: T-6 Pre-ignition check failed, NOS A not open\n");
        checks_good = false;
    }
    if (servos[VALVE_NOS_B].setPos != SERVO_POSITION_OPEN || !servos[VALVE_NOS_B].atSetPos) {
        dbg_printf("SEQ: T-6 Pre-ignition check failed, NOS B not open\n");
        checks_good = false;
    }
    if (servos[VALVE_NITROGEN].setPos != SERVO_POSITION_CLOSE || !servos[VALVE_NITROGEN].atSetPos) {
        dbg_printf("SEQ: T-6 Pre-ignition check failed, Nitrogen not closed\n");
        checks_good = false;
    }
    if (servos[VALVE_VENT].setPos != SERVO_POSITION_CLOSE || !servos[VALVE_VENT].atSetPos) {
        dbg_printf("SEQ: T-6 Pre-ignition check failed, Vent not closed\n");
        checks_good = false;
    }
    // 2) ESTOP is released
    if (comp_get_interlock() == false) {
        dbg_printf("SEQ: T-6 Pre-ignition check failed, ESTOP pressed\n");
        checks_good = false;
    }
    // 3) Continuity on ignitor 1
    #ifndef COLDFLOW_MODE
    if (comp_get_ematch1() == false) {
        dbg_printf("SEQ: T-6 Pre-ignition check failed, Ignitor 1 continuity bad\n");
        checks_good = false;
    }
    #endif
    // 4) Mainline pressure > 30 bar
    #ifndef TEST_MODE
    int32_t mainline_pressure = sensors_get_data(SENSOR_PT_MAINLINE);
    if (mainline_pressure < 300) {
        dbg_printf("SEQ: T-6 Pre-ignition check failed, Mainline pressure low (%ld mbar)\n", mainline_pressure);
        checks_good = false;
    }
    #endif

    // If successful, move on, otherwise abort
    if (checks_good) {
        dbg_printf("SEQ: T-6 Pre-ignition checks good, go for flamey stuff\n");
    } else {
        dbg_printf("SEQ: T-6 Pre-ignition checks failed, aborting\n");
        fsm_set_abort(ECU_ERROR_PREFIRE_CHECKS_FAIL);
    }
}

static void seq_task_tm5_softarm(void)
{
    bool stat = spicy_arm();
    if (!stat) {
        fsm_set_abort(ECU_ERROR_ARM_FAIL);
    }
    dbg_printf("SEQ: T-5, Armed\n");
}

static void seq_task_tm3_ignition(void)
{
    spicy_fire_ematch1();
    dbg_printf("SEQ: T-3, ignition sequence started\n");
}

static void seq_task_tm2_ign_off(void)
{
    spicy_off_ematch1();
    dbg_printf("SEQ: T-2, ignition end\n");
}

static void seq_task_tm0_solenoid(void)
{
    spicy_open_solenoid();
    dbg_printf("SEQ: T-0, Solenoid opened, we are go!\n");
    sequencer_set_state(SEQUENCER_FIRE);
}

static void seq_task_tp0_solenoid_nos_close(void);
static void seq_task_tp1_open_vent(void);
static void seq_task_tp3_open_nitrogen(void);
static void seq_task_tp5_close_vent_open_solenoid(void);
static void seq_task_tp10_close_solenoid_nitrogen(void);
static void seq_task_tp12_open_vent(void);
static void seq_task_tp20_close_vent(void);

// Tasks must be in order they will be executed in. Note that the times DO NOT include the burn time, times are after burn completion
static const sequencer_task_t fire_task_list[] = {
    { seq_task_tp0_solenoid_nos_close, 0 },
    { seq_task_tp1_open_vent, 1000 },
    { seq_task_tp3_open_nitrogen, 3000 },
    { seq_task_tp5_close_vent_open_solenoid, 5000 },
    { seq_task_tp10_close_solenoid_nitrogen, 10000 },
    { seq_task_tp12_open_vent, 12000 },
    { seq_task_tp20_close_vent, 20000 }
};

static void fire_tasks(void)
{
    if (get_countdown() <= burn_time) {
        // While burning perform checks to ensure nominal conditions.
        // 1) Check combustion chamber stays below 50 bar
        int32_t cc_pressure = sensors_get_data(SENSOR_P_CHAMBER);
        if (cc_pressure > 500) {
            dbg_printf("SEQ: During burn, combustion chamber overpressure (%ld.%02ld bar), aborting\n", cc_pressure / 10, cc_pressure % 10);
            fsm_set_abort(ECU_ERROR_CHAMBER_OVERPRESSURE);
        }
    }

    if (get_countdown() >= fire_task_list[task_list_index].execution_ms + burn_time) {
        fire_task_list[task_list_index++].task();
    }
}

static void seq_task_tp0_solenoid_nos_close(void)
{
    spicy_close_solenoid();
    servo_set_position(VALVE_NOS_A, SERVO_POSITION_CLOSE);
    servo_set_position(VALVE_NOS_B, SERVO_POSITION_CLOSE);
    dbg_printf("SEQ: T+%d(0) Burn complete, NOS closed\n", burn_time/1000);
}

static void seq_task_tp1_open_vent(void)
{
    servo_set_position(VALVE_VENT, SERVO_POSITION_OPEN);
    dbg_printf("SEQ: T+%d(2) Vent opened\n", (burn_time + 2000)/1000);
}

static void seq_task_tp3_open_nitrogen(void)
{
    servo_set_position(VALVE_NITROGEN, SERVO_POSITION_OPEN);
    dbg_printf("SEQ: T+%d(3) Nitrogen opened\n", (burn_time + 3000)/1000);
}

static void seq_task_tp5_close_vent_open_solenoid(void)
{
    servo_set_position(VALVE_VENT, SERVO_POSITION_CLOSE);
    spicy_open_solenoid();
    dbg_printf("SEQ: T+%d(4) Vent closed, Solenoid opened\n", (burn_time + 4000)/1000);
}

static void seq_task_tp10_close_solenoid_nitrogen(void)
{
    spicy_close_solenoid();
    spicy_disarm();
    servo_set_position(VALVE_NITROGEN, SERVO_POSITION_CLOSE);
    dbg_printf("SEQ: T+%d(10) Nitrogen and Solenoid closed\n", (burn_time + 10000)/1000);
}

static void seq_task_tp12_open_vent(void)
{
    servo_set_position(VALVE_VENT, SERVO_POSITION_OPEN);
    dbg_printf("SEQ: T+%d(12) Vent opened\n", (burn_time + 12000)/1000);
}

static void seq_task_tp20_close_vent(void)
{
    servo_set_position(VALVE_VENT, SERVO_POSITION_CLOSE);
    dbg_printf("SEQ: T+%d(20) Vent closed, sequencer complete\n", (burn_time + 20000)/1000);
    dbg_printf("SEQ: Time for the pub?\n");
    fsm_set_state(STATE_POST_FIRE);
}