#include "main_FSM.h"
#include "debug_io.h"
#include "spicy.h"
#include "can.h"
#include "sequencer.h"
#include "manual_solenoid.h"
#include "manual_valve.h"

//==============================
// Internal state variables
//==============================
static volatile main_states_t current_state = STATE_INIT;
switch_state_t switch_snapshot = {0};
// static volatile main_states_t previous_state = STATE_INIT;

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
// typedef void (*st_on_event_fn)(stager_event_t e);

typedef struct {
    st_on_enter_fn on_enter;
    st_on_exit_fn  on_exit;
    st_on_tick_fn  on_tick;
    // st_on_event_fn on_event;
    const char    *name;
} state_parameters_t;

// Forward declarations of handlers
static void s_init_enter(void);      static void s_init_tick(void);
static void s_ready_enter(void);     static void s_ready_tick(void);
static void s_seq_enter(void);       static void s_seq_tick(void);
static void s_post_enter(void);      static void s_post_tick(void);
static void s_manual_enter(void);    static void s_manual_tick(void); static void s_manual_exit(void);
static void s_error_enter(void);     static void s_error_tick(void);
static void s_abort_enter(void);     static void s_abort_tick(void);
static void s_generic_noop(void) {}
static void s_error_exit(void);      static void s_abort_exit(void);

// State table
static const state_parameters_t st_table[] = {
    [STATE_INIT]        = { s_init_enter,   s_generic_noop, s_init_tick,   "INIT" },
    [STATE_READY]       = { s_ready_enter,  s_generic_noop, s_ready_tick,  "READY" },
    [STATE_SEQUENCER]   = { s_seq_enter,    s_generic_noop, s_seq_tick,    "SEQUENCER" },
    [STATE_POST_FIRE]   = { s_post_enter,   s_generic_noop, s_post_tick,   "POST_FIRE" },
    [STATE_MANUAL_MODE] = { s_manual_enter, s_manual_exit,  s_manual_tick, "MANUAL" },
    [STATE_ERROR]       = { s_error_enter,  s_error_exit,   s_error_tick,  "ERROR" },
    [STATE_ABORT]       = { s_abort_enter,  s_abort_exit,   s_abort_tick,  "ABORT" }
};

void outputs_safe(void)
{
    spicy_off_ematch1();
    spicy_off_ematch2();
    spicy_close_ox();
    spicy_disarm();
    // Disarm all servos
    can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_ARM, 0xF0);
}

static inline bool both_armed(void) 
{ 
    return switch_snapshot.master_pyro && switch_snapshot.master_valve; 
}

bool prefire_ok(void) 
{
    //TODO: sensor checks, ignitor ready

    // For now, simply return true
    return true;
}

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
    switch_snapshot = ns;
    // bool changed_arm=
    //     (old.master_power       != ns.master_power)       ||
    //     (old.master_valve       != ns.master_valve)       ||
    //     (old.master_pyro        != ns.master_pyro)        ||
    //     (old.sequencer_override != ns.sequencer_override);

    // bool changed_valves=
    //     (old.valve_nitrous_a    != ns.valve_nitrous_a)    ||
    //     (old.valve_nitrous_b    != ns.valve_nitrous_b)    ||
    //     (old.valve_nitrogen     != ns.valve_nitrogen)     ||
    //     (old.valve_discharge    != ns.valve_discharge)    ||
    //     (old.solenoid           != ns.solenoid);
    
    // TODO: implement a method for flagging that the switch states have changed
}

//==============================
// INIT State
//==============================
static void s_init_enter(void)
{
    outputs_safe();
    dbg_printf("Init enter, outputs safe\n");
}

static void s_init_tick(void)
{
    // TODO: Wait for RIU heartbeat

    // Temporary just go to ready
    fsm_set_state(STATE_READY);
    dbg_printf("Init tick, move to ready\n");
}

//==============================
// READY State
//==============================
static void s_ready_enter(void)
{
    // Uninitalise the sequencer
    uninitialise_sequencer();
    dbg_printf("Ready enter, uninitalised sequencer\n");
}

static void s_ready_tick(void)
{
    // Check state of switches
    if (switch_snapshot.sequencer_override) { // Switch to manual mode if override active
        fsm_set_state(STATE_MANUAL_MODE);
        dbg_printf("Ready tick, manual override, set state to manual\n");
        // Exit the function
        return;
    }

    if (both_armed() && prefire_ok()) { // Switch to sequencer if both master switches are armed and prefire checks pass
        fsm_set_state(STATE_SEQUENCER);
        dbg_printf("Ready tick, both master switches armed and prefire ok, set state to sequencer\n");
        // Exit the function
        return;
    }
}

//==============================
// SEQUENCER State
//==============================
static void s_seq_enter(void) 
{
    set_sequencer_ready();
}

static void s_seq_tick(void)
{
    sequencer_tick();
}

//==============================
// POST_FIRE State
//==============================
static void s_post_enter(void)
{ 
    /* TODO: logging / cool-down */ 
    dbg_printf("Post fire enter\n");
}

static void s_post_tick(void)
{ 
    if(!both_armed() /* Maybe also check ESTOP in here */) {
        fsm_set_state(STATE_READY);
        dbg_printf("Post fire, both arm switches disabled - moving to ready state\n");
    }
}

//==============================
// MANUAL_MODE State
//==============================
// Helper for manual mode
static void manual_all_safe(void)
{
    // Ensure hardware safe
    manual_valve_send_disarm();
    manual_solenoid_off();
    manual_pyro_disarm();
    outputs_safe();
    valve_state = VALVE_DISARMED;
    pyro_state  = PYRO_SAFE;
}

static void s_manual_enter(void)
{
    valve_state = VALVE_DISARMED;
    pyro_state  = PYRO_SAFE;
    dbg_printf("Manual mode enter, valve and pyro safe\n");
    // Do not arm anything until operator asserts specific switches.
}

static void s_manual_tick(void)
{
    // Check that the manual override is still active
    if (!switch_snapshot.sequencer_override) {// If not still overriding then return to ready
        fsm_set_state(STATE_READY);
        dbg_printf("Manual tick, override no longer active - moving to ready state\n");
        return;
    }

    valve_state_decoder();
    pyro_state_decoder();
    dbg_printf("Manual tick, override active - valve and pyro sub machines called\n");
}

static void s_manual_exit(void)
{
    manual_all_safe();
    dbg_printf("Manual exit, system in safe mode\n");
}

//==============================
// ERROR State
//==============================
static void s_error_enter(void)
{
    outputs_safe();
    dbg_printf("Error enter, outputs set to safe\n");
}

static void s_error_tick(void)
{
    if(!both_armed()) {
        fsm_set_state(STATE_READY);
        dbg_printf("Error tick, valves and pyro no longer armed, move to ready state\n");
    }
    dbg_printf("Error tick, valves and pyro still armed - stay in error\n");
}

static void s_error_exit(void)
{
    dbg_printf("Error exit\n");
}

//==============================
// ABORT State
//==============================
static void s_abort_enter(void)
{
    outputs_safe();
    dbg_printf("Abort enter, outputs safe\n");
}

static void s_abort_tick(void)
{
    if (!both_armed()) {
        fsm_set_state(STATE_READY);
        dbg_printf("Abort tick, valves and pyro no longer armed, move to ready state\n");
    }
    dbg_printf("Abort tick, valves and pyro still armed - stay in abort\n");
}

static void s_abort_exit(void)
{
    dbg_printf("Abort exit\n");
}