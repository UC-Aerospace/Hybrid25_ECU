#include "main_FSM.h"
#include "debug_io.h"
#include "spicy.h"
#include "can.h"
#include "sequencer.h"
#include "manual_solenoid.h"
#include "manual_valve.h"
#include "servo.h"
#include "heartbeat.h"
#include "rs422.h"
#include "error_def.h"
#include "sensors.h"

//==============================
// Internal state variables
//==============================
static volatile main_states_t current_state = STATE_INIT;
static switch_state_t switch_snapshot = {0};
static uint8_t error_code = 0; // Last error code

typedef void (*st_on_enter_fn)(main_states_t prev_state);
typedef void (*st_on_exit_fn)(void);
typedef void (*st_on_tick_fn)(void);

typedef struct {
    st_on_enter_fn on_enter;
    st_on_exit_fn  on_exit;
    st_on_tick_fn  on_tick;
    const char    *name;
} state_parameters_t;

// Forward declarations of handlers
static void s_init_enter(main_states_t prev_state);      static void s_init_exit(void);   static void s_init_tick(void);
static void s_ready_enter(main_states_t prev_state);     static void s_ready_exit(void);  static void s_ready_tick(void);
static void s_seq_enter(main_states_t prev_state);       static void s_seq_exit(void);    static void s_seq_tick(void);
static void s_post_enter(main_states_t prev_state);      static void s_post_exit(void);   static void s_post_tick(void);
static void s_manual_enter(main_states_t prev_state);    static void s_manual_exit(void); static void s_manual_tick(void);
static void s_abort_enter(main_states_t prev_state);     static void s_abort_exit(void);  static void s_abort_tick(void);
static void s_generic_noop(void) {}

// State table
static const state_parameters_t st_table[] = {
    [STATE_INIT]        = { s_init_enter,   s_init_exit,    s_init_tick,   "INIT" },
    [STATE_READY]       = { s_ready_enter,  s_ready_exit,   s_ready_tick,  "READY" },
    [STATE_SEQUENCER]   = { s_seq_enter,    s_seq_exit,     s_seq_tick,    "SEQUENCER" },
    [STATE_POST_FIRE]   = { s_post_enter,   s_post_exit,    s_post_tick,   "POST_FIRE" },
    [STATE_MANUAL_MODE] = { s_manual_enter, s_manual_exit,  s_manual_tick, "MANUAL" },
    [STATE_ABORT]       = { s_abort_enter,  s_abort_exit,   s_abort_tick,  "ABORT" }
};

void fsm_set_state(main_states_t new_state)
{
    // If there is no state change, do nothing
    if (new_state == current_state) {
        return;
    } else if (new_state > STATE_ABORT) { // If the state requested is not a correct state, error out --- Potentially set to error state
        dbg_printf("MAINFSM: Invalid state\n");
        return;
    } else { // State change is valid, change state
        // const state_parameters_t *current = &st_table[current_state];
        // const state_parameters_t *new = &st_table[new_state];

        // Call exit function for previous (current at this point) state
        st_table[current_state].on_exit();
        // current -> on_exit();

        // Change current_state to new_state
        main_states_t prev_state = current_state;
        current_state = new_state;

        // Call entry function for the new current_state
        st_table[current_state].on_enter(prev_state);
        // new->on_enter();
    }
}

void fsm_tick(void)
{
    st_table[current_state].on_tick();
}

//==============================
// HELPERS
//==============================

void outputs_safe(void)
{
    dbg_printf("MAINFSM: OUTPUTS SAFE\n");
    spicy_off_ematch1();
    spicy_off_ematch2();
    spicy_close_solenoid();
    spicy_disarm();
    servo_disarm_all();
}

inline bool both_armed(void) 
{ 
    return switch_snapshot.master_pyro && switch_snapshot.master_valve; 
}

bool prefire_ok(void) 
{
    bool checks_good = true;
     // Check #1 - PYRO and VALVE switches must be on, override must be off
    if (!both_armed() || switch_snapshot.sequencer_override) {
        checks_good = false;
        dbg_printf("MAINFSM SEQ: Failed as both arm switches not enabled or override on\n");
    }
    // Check #2 - ESTOP must be released
    if (!comp_get_interlock()) {
        checks_good = false;
        dbg_printf("MAINFSM SEQ: Failed as ESTOP not released\n");
    }
    // Check #3 - Continuity on ignitor 1`
    #ifndef COLDFLOW_MODE
    if (!comp_get_ematch1()) {
        checks_good = false;
        dbg_printf("MAINFSM SEQ: Failed as ignitor 1 not continuous\n");
    }
    #endif
    // Check #4 - All valves closed
    if (!servo_helper_check_all_closed()) {
        checks_good = false;
        dbg_printf("MAINFSM SEQ: Failed as not all valves closed\n");
    }
    // Check #5 - >30 bar on mainline
    if (sensors_get_data(SENSOR_PT_MAINLINE) < 300) { // 10*bar, so 300 = 30.0 bar
        checks_good = false;
        dbg_printf("MAINFSM SEQ: Failed as mainline pressure < 30 bar\n");
    }

    return checks_good;
}

void fsm_set_switch_states(uint16_t switches)
{
    static uint16_t last_switches = 0;
    switch_state_t ns = {0};
    ns.master_power       = (switches & (1u << 15)) != 0u;
    ns.master_valve       = (switches & (1u << 14)) != 0u;
    ns.master_pyro        = (switches & (1u << 13)) != 0u;
    ns.sequencer_override = (switches & (1u << 12)) != 0u;
    ns.valve_nitrous_a    = (switches & (1u << 7))  != 0u;
    ns.valve_nitrous_b    = (switches & (1u << 6))  != 0u;
    ns.valve_nitrogen     = (switches & (1u << 5))  != 0u;
    ns.valve_discharge    = (switches & (1u << 4))  != 0u;
    ns.solenoid           = (switches & (1u << 3))  != 0u;
    ns.changed            = (switches != last_switches);
    last_switches = switches;

    if (ns.changed) {
        switch_snapshot = ns;
    }
}

main_states_t fsm_get_state(void)
{
    return current_state;
}

void fsm_set_abort(uint8_t code)
{
    error_code = code;
    fsm_set_state(STATE_ABORT);
}

uint8_t fsm_get_error_code(void)
{
    return error_code;
}

// Depending on state and error code, will change the outcome
// TODO: Add all possible commands from CAN
void fsm_raise_error(uint8_t raise_code)
{
    rs422_send_error_warning(CAN_ERROR_ACTION_ERROR << 6 | BOARD_ID_ECU, raise_code);
    switch (raise_code) {
        case ECU_ERROR_HEARTBEAT_LOST:
            uint8_t hb_status = get_heartbeat_status();
            dbg_printf("MAINFSM RAISE: Heartbeat lost, status 0x%02X\n", hb_status);
            bool riu_lost = (hb_status & (1 << BOARD_ID_RIU)) == 0;
            bool servo_lost = (hb_status & (1 << BOARD_ID_SERVO)) == 0;
            bool adc_a_lost = (hb_status & (1 << BOARD_ID_ADC_A)) == 0;

            if (servo_lost) { // Always abort if servo lost
                error_code = ECU_ERROR_HEARTBEAT_LOST;
                fsm_set_state(STATE_ABORT);
                return;
            } 
            
            if (riu_lost) { // If RIU lost, abort unless firing. If firing continue and rely on ESTOP.
                if (sequencer_get_state() == SEQUENCER_FIRE) {
                    dbg_printf("MAINFSM RAISE: RIU heartbeat lost but firing, continuing...\n");
                } else {
                    error_code = ECU_ERROR_HEARTBEAT_LOST;
                    fsm_set_state(STATE_ABORT);
                    return;
                }
            }
            
            if (adc_a_lost) { // If ADC_A lost, as above abort unless firing already.
                if (sequencer_get_state() == SEQUENCER_FIRE) {
                    dbg_printf("MAINFSM RAISE: ADC_A heartbeat lost but firing, continuing...\n");
                } else {
                    error_code = ECU_ERROR_HEARTBEAT_LOST;
                    fsm_set_state(STATE_ABORT);
                    return;
                }
            }
            
            break;
            
        case ECU_ERROR_RS422_RX_RESTART_FAIL:
            // If RS422 RX fails, abort unless firing.
            if (sequencer_get_state() == SEQUENCER_FIRE) {
                dbg_printf("MAINFSM RAISE: RS422 RX restart failed but firing, continuing...\n");
            } else {
                error_code = ECU_ERROR_RS422_RX_RESTART_FAIL;
                fsm_set_state(STATE_ABORT);
                return;
            }
            break;

        default:
            // For all other errors, just go to error state from any state
            fsm_set_state(STATE_ABORT);
            break;
    }
}


//==============================
// INIT State
//==============================
static void s_init_enter(main_states_t prev_state)
{
    outputs_safe();
    dbg_printf("STATE ENTER: Init\n");
}

static void s_init_exit(void)
{
    outputs_safe();
    dbg_printf("STATE EXIT: Init\n");
}

static void s_init_tick(void)
{
    // VERIFY: Verify this
    if (heartbeat_all_started()) {
        dbg_printf("MAINFSM INIT: All heartbeats good, moving to ready state\n");
        fsm_set_state(STATE_READY);
    } else {
        // Still waiting for heartbeats
        return;
    }
    
    // Temporary just go to ready
    #ifdef TEST_MODE
    fsm_set_state(STATE_READY);
    #endif
}

//==============================
// READY State
//==============================
static void s_ready_enter(main_states_t prev_state)
{
    dbg_printf("STATE ENTER: Ready\n");
}

static void s_ready_exit(void)
{
    dbg_printf("STATE EXIT: Ready\n");
}

static void s_ready_tick(void)
{
    // Check state of switches
    if (switch_snapshot.sequencer_override) { // Switch to manual mode if override active
        fsm_set_state(STATE_MANUAL_MODE);
        // Exit the function
        return;
    }

    if (both_armed() && prefire_ok()) { // Switch to sequencer if both master switches are armed and prefire checks pass
        fsm_set_state(STATE_SEQUENCER);
        // Exit the function
        return;
    }
}

//==============================
// SEQUENCER State
//==============================
static void s_seq_enter(main_states_t prev_state) 
{
    // Do conditional checks here, can either enter sequencer ready or sequencer failed start
    bool checks_good = prefire_ok();

    if (checks_good) {
        dbg_printf("STATE ENTER: Sequencer (READY)\n");
        sequencer_set_state(SEQUENCER_READY);
    } else {
        dbg_printf("STATE ENTER: Sequencer (FAILED START)\n");
        sequencer_set_state(SEQUENCER_FAILED_START);
    }
}

static void s_seq_exit(void) 
{
    dbg_printf("STATE EXIT: Sequencer\n");
    outputs_safe();
    sequencer_set_state(SEQUENCER_UNINITIALISED);
}

static void s_seq_tick(void)
{
    // Check that the switches are still correct
    if (!both_armed() || switch_snapshot.sequencer_override || !comp_get_interlock()) { // If not still armed or override active or estop pressed then exit sequencer
        dbg_printf("MAINFSM SEQ: Switches changed, exiting...\n");
        if (sequencer_get_state() == SEQUENCER_READY || sequencer_get_state() == SEQUENCER_UNINITIALISED || sequencer_get_state() == SEQUENCER_FAILED_START) {
            fsm_set_state(STATE_READY);
        } else {
            fsm_set_state(STATE_ABORT);
        }
        return;
    }
    sequencer_tick();
}

//==============================
// POST_FIRE State
//==============================
static void s_post_enter(main_states_t prev_state)
{ 
    dbg_printf("STATE ENTER: Post Fire\n");
}

static void s_post_exit(void)
{
    dbg_printf("STATE EXIT: Post Fire\n");
}

static void s_post_tick(void)
{ 
    if(!both_armed() && !comp_get_interlock()) { // Transition when arm switches are changed and estop pressed
        fsm_set_state(STATE_READY);
        dbg_printf("MAINFSM SEQ: Post fire, both arm switches disabled - moving to ready state\n");
    }
}

//==============================
// MANUAL_MODE State
//==============================

static void s_manual_enter(main_states_t prev_state)
{
    dbg_printf("STATE ENTER: Manual\n");
    // Do not arm anything until operator asserts specific switches.
}

static void s_manual_exit(void)
{
    manual_valve_set_safe();
    manual_solenoid_set_safe();
    dbg_printf("STATE EXIT: Manual\n");
}

static void s_manual_tick(void)
{
    // Check that the manual override is still active
    if (!switch_snapshot.sequencer_override) {// If not still overriding then return to ready
        fsm_set_state(STATE_READY);
        return;
    }

    if (switch_snapshot.changed) {
        // If the switch states have changed, update the state
        manual_valve_tick(&switch_snapshot);
        manual_solenoid_tick(&switch_snapshot);
        switch_snapshot.changed = false;
    }
}

//==============================
// ABORT State
//==============================
static void s_abort_enter(main_states_t prev_state)
{
    outputs_safe();
    switch_snapshot.changed = false; // Prevent any changes to manual mode until we leave abort state
    rs422_send_abort(error_code); // Send abort code over RS422
    can_send_error_warning(CAN_NODE_TYPE_BROADCAST, CAN_NODE_ADDR_BROADCAST, CAN_ERROR_ACTION_SHUTDOWN, error_code); // Send abort code over CAN
    dbg_printf("STATE ENTER: Abort\n");
}

static void s_abort_exit(void)
{
    error_code = 0; // Clear error code on exit
    dbg_printf("STATE EXIT: Abort\n");
}

static void s_abort_tick(void)
{
    if (!switch_snapshot.master_pyro && !switch_snapshot.master_valve && !comp_get_interlock() && !switch_snapshot.sequencer_override && switch_snapshot.changed) { // Transition when both arm switches are off and estop pressed
        dbg_printf("MAINFSM ABORT: Moving to ready state\n");
        fsm_set_state(STATE_READY);
    }
}
