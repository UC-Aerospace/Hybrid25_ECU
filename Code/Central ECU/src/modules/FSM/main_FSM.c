#include "main_FSM.h"
#include "debug_io.h"
#include "spicy.h"
#include "can.h"
#include "sequencer.h"
#include "manual_solenoid.h"
#include "manual_valve.h"
#include "servo.h"

//==============================
// Internal state variables
//==============================
static volatile main_states_t current_state = STATE_INIT;
static switch_state_t switch_snapshot = {0};

//Functions to write: ------ TODO ------
    //Set state
    //Get state
        //Maybe, only need this if other modules need access to the main states
    //Decode state
        //Switch case where actions are performed based on current state
    //FSM init

typedef void (*st_on_enter_fn)(void);
typedef void (*st_on_exit_fn)(void);
typedef void (*st_on_tick_fn)(void);

typedef struct {
    st_on_enter_fn on_enter;
    st_on_exit_fn  on_exit;
    st_on_tick_fn  on_tick;
    const char    *name;
} state_parameters_t;

// Forward declarations of handlers
static void s_init_enter(void);      static void s_init_exit(void);   static void s_init_tick(void);
static void s_ready_enter(void);     static void s_ready_exit(void);  static void s_ready_tick(void);
static void s_seq_enter(void);       static void s_seq_exit(void);    static void s_seq_tick(void);
static void s_post_enter(void);      static void s_post_exit(void);   static void s_post_tick(void);
static void s_manual_enter(void);    static void s_manual_exit(void); static void s_manual_tick(void);
static void s_error_enter(void);     static void s_error_exit(void);  static void s_error_tick(void);
static void s_abort_enter(void);     static void s_abort_exit(void);  static void s_abort_tick(void);
static void s_generic_noop(void) {}

// State table
static const state_parameters_t st_table[] = {
    [STATE_INIT]        = { s_init_enter,   s_init_exit,    s_init_tick,   "INIT" },
    [STATE_READY]       = { s_ready_enter,  s_ready_exit,   s_ready_tick,  "READY" },
    [STATE_SEQUENCER]   = { s_seq_enter,    s_seq_exit,     s_seq_tick,    "SEQUENCER" },
    [STATE_POST_FIRE]   = { s_post_enter,   s_post_exit,    s_post_tick,   "POST_FIRE" },
    [STATE_MANUAL_MODE] = { s_manual_enter, s_manual_exit,  s_manual_tick, "MANUAL" },
    [STATE_ERROR]       = { s_error_enter,  s_error_exit,   s_error_tick,  "ERROR" },
    [STATE_ABORT]       = { s_abort_enter,  s_abort_exit,   s_abort_tick,  "ABORT" }
};

void fsm_set_state(main_states_t new_state)
{
    // If there is no state change, do nothing
    if (new_state == current_state) {
        return;
    } else if (new_state > STATE_ABORT) { // If the state requested is not a corret state, error out --- Potentially set to error state
        dbg_printf("Error: Invalid state\n");
        return;
    } else { // State change is valid, change state
        // const state_parameters_t *current = &st_table[current_state];
        // const state_parameters_t *new = &st_table[new_state];

        // Call exit function for previous (current at this point) state
        st_table[current_state].on_exit();
        // current -> on_exit();

        // Change current_state to new_state
        current_state = new_state;

        // Call entry function for the new current_state
        st_table[current_state].on_enter();
        // new->on_enter();
    }
}

void fsm_tick(void)
{
    //TODO: Write logic to run everytime main loop calls this function
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
    // PLACEHOLDER

    // Check #6 - Status good on all peripherals
    // PLACEHOLDER

    return checks_good;
}

void fsm_set_switch_states(uint16_t switches)
{
    // switch_state_t old = switch_snapshot;
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
    ns.changed            = (switch_snapshot.master_power       != ns.master_power)       ||
                            (switch_snapshot.master_valve       != ns.master_valve)       ||
                            (switch_snapshot.master_pyro        != ns.master_pyro)        ||
                            (switch_snapshot.sequencer_override != ns.sequencer_override) ||
                            (switch_snapshot.valve_nitrous_a    != ns.valve_nitrous_a)    ||
                            (switch_snapshot.valve_nitrous_b    != ns.valve_nitrous_b)    ||
                            (switch_snapshot.valve_nitrogen     != ns.valve_nitrogen)     ||
                            (switch_snapshot.valve_discharge    != ns.valve_discharge)    ||
                            (switch_snapshot.solenoid           != ns.solenoid);

    if (ns.changed) {
        switch_snapshot = ns;
    }
}

//==============================
// INIT State
//==============================
static void s_init_enter(void)
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
    // TODO: Wait for RIU heartbeat

    // Temporary just go to ready
    fsm_set_state(STATE_READY);
}

//==============================
// READY State
//==============================
static void s_ready_enter(void)
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
        // TODO: This logic dosn't match FSM diagram, in diagram if prefire_ok() fails it should transition to sequencer danger and display a light on the RIU
        fsm_set_state(STATE_SEQUENCER);
        // Exit the function
        return;
    }
}

//==============================
// SEQUENCER State
//==============================
static void s_seq_enter(void) 
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
        // TODO: Should set abort if not just waiting in READY or UNINITIALISED
        if (sequencer_get_state() == SEQUENCER_READY || sequencer_get_state() == SEQUENCER_UNINITIALISED) {
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
static void s_post_enter(void)
{ 
    /* TODO: logging / cool-down */ 
    dbg_printf("STATE ENTER: Post Fire\n");
}

static void s_post_exit(void)
{
    dbg_printf("STATE EXIT: Post Fire\n");
}

static void s_post_tick(void)
{ 
    if(!both_armed() /* Maybe also check ESTOP in here */) {  //Use comp get interlock to check estop state
        fsm_set_state(STATE_READY);
        dbg_printf("Post fire, both arm switches disabled - moving to ready state\n");
    }
}

//==============================
// MANUAL_MODE State
//==============================

static void s_manual_enter(void)
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
// ERROR State
//==============================
static void s_error_enter(void)
{
    outputs_safe();
    dbg_printf("STATE ENTER: Error\n");
}

static void s_error_exit(void)
{
    dbg_printf("STATE EXIT: Error\n");
}

static void s_error_tick(void)
{
    // TODO: Add a proper procedure here
    if(!both_armed()) {
        fsm_set_state(STATE_READY);
    }
}



//==============================
// ABORT State
//==============================
static void s_abort_enter(void)
{
    outputs_safe();
    //TODO: Send CAN back to RIU to tell them we have aborted
    dbg_printf("STATE ENTER: Abort\n");
}

static void s_abort_exit(void)
{
    dbg_printf("STATE EXIT: Abort\n");
}

static void s_abort_tick(void)
{
    if (!both_armed()) {
        fsm_set_state(STATE_READY);
    }
}
