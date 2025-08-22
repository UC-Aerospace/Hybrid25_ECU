//====================================================
// Table-driven STAGER FSM (mirrors style of servo fsm.c)
//====================================================

/*
 * TODO: 1) Make switch changes change against last commanded as quickly flipping can fail to register.
 *          Or just update last switch state pre-emptivly with command.
 * 
 * */

#include "legacy_stager.h"
#include "heartbeat.h"
#include "spicy.h"
#include "can.h"

//==============================
// Internal types / events
//==============================
typedef enum {
    STAGER_EVT_NONE = 0,
    STAGER_EVT_SWITCH_ARM_CHANGE,
    STAGER_EVT_SWITCH_VALVES_CHANGE,
    STAGER_EVT_ABORT_REQ,
    STAGER_EVT_ERROR_REQ,
    STAGER_EVT_HEARTBEAT_READY,   // Heartbeat system indicates all nodes up
    STAGER_EVT_COUNT
} stager_event_t;

//==============================
// Internal state variables
//==============================
static volatile stager_state_t current_state = STATE_INIT;
static stager_switch_state_t switch_snapshot = {0};
static volatile bool abort_flag = false; // set from ISR
static volatile bool error_flag = false; // set from ISR

// Sequencer sub-state (kept simple for now)
static stager_sequencer_substate_t seq_state = SEQUENCER_UNINITIALIZED;
static uint32_t seq_t0_ms = 0u;

// Lastest set position from servo feedback
struct {
    bool servoPosCommandedVent;
    bool servoPosCommandedNitrogen;
    bool servoPosCommandedNosA;
    bool servoPosCommandedNosB;
    bool initialised;
} servo_feedback = {false};

//==============================
// Event Queue (tiny ring)
//==============================
#define STAGER_EVENT_QUEUE_SIZE 8
typedef struct {
    stager_event_t ev[STAGER_EVENT_QUEUE_SIZE];
    uint8_t head; // pop
    uint8_t tail; // push
} stager_evt_queue_t;
static stager_evt_queue_t evtQ = { .ev = { STAGER_EVT_NONE }, .head = 0u, .tail = 0u };

static bool evtq_is_empty(void){ return evtQ.head == evtQ.tail; }
static bool evtq_is_full(void){ return (uint8_t)((evtQ.tail + 1u) % STAGER_EVENT_QUEUE_SIZE) == evtQ.head; }
static bool evtq_push(stager_event_t e){ if(e==STAGER_EVT_NONE||evtq_is_full()) return false; evtQ.ev[evtQ.tail]=e; evtQ.tail=(uint8_t)((evtQ.tail+1u)%STAGER_EVENT_QUEUE_SIZE); return true; }
static bool evtq_pop(stager_event_t *out){ if(evtq_is_empty()) return false; *out=evtQ.ev[evtQ.head]; evtQ.head=(uint8_t)((evtQ.head+1u)%STAGER_EVENT_QUEUE_SIZE); return true; }

//==============================
// Critical section helpers
//==============================
static inline uint32_t crit_enter(void){ uint32_t p=__get_PRIMASK(); __disable_irq(); return p; }
static inline void crit_exit(uint32_t p){ __set_PRIMASK(p); }

//==============================
// Utility helpers
//==============================
static inline bool both_armed(void){ return switch_snapshot.master_pyro && switch_snapshot.master_valve; }

static void outputs_safe(void){
    spicy_off_ematch1();
    spicy_off_ematch2();
    spicy_close_ox();
    spicy_disarm();
    // Disarm all servos
    can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_ARM, 0xF0);
}

static bool prefire_ok(void){
    // TODO: integrate comparators & sensor checks (interlock, ox low, ematch continuity, pressures)
    return true;
}

void stager_set_servo_feedback_position(bool servoSetPosition[4]){
    servo_feedback.initialised = true;
    servo_feedback.servoPosCommandedVent = servoSetPosition[0];
    servo_feedback.servoPosCommandedNitrogen = servoSetPosition[1];
    servo_feedback.servoPosCommandedNosA = servoSetPosition[2];
    servo_feedback.servoPosCommandedNosB = servoSetPosition[3];
}

//==============================
// State handler function types
//==============================
typedef void (*st_on_enter_fn)(void);
typedef void (*st_on_exit_fn)(void);
typedef void (*st_on_tick_fn)(void);
typedef void (*st_on_event_fn)(stager_event_t e);

typedef struct {
    st_on_enter_fn on_enter;
    st_on_exit_fn  on_exit;
    st_on_tick_fn  on_tick;
    st_on_event_fn on_event;
    const char    *name;
} stager_state_desc_t;

// Forward declarations of handlers
static void s_init_enter(void);      static void s_init_tick(void);      static void s_init_event(stager_event_t e);
static void s_ready_enter(void);     static void s_ready_tick(void);     static void s_ready_event(stager_event_t e);
static void s_seq_enter(void);       static void s_seq_tick(void);       static void s_seq_event(stager_event_t e);
static void s_post_enter(void);      static void s_post_tick(void);      static void s_post_event(stager_event_t e);
static void s_manual_enter(void);    static void s_manual_tick(void);    static void s_manual_event(stager_event_t e); static void s_manual_exit(void);
static void s_error_enter(void);     static void s_error_tick(void);     static void s_error_event(stager_event_t e);
static void s_abort_enter(void);     static void s_abort_tick(void);     static void s_abort_event(stager_event_t e);
static void s_generic_noop(void) {}
static void s_error_exit(void);      static void s_abort_exit(void);

// State table
static const stager_state_desc_t st_table[] = {
    [STATE_INIT]        = { s_init_enter,   s_generic_noop, s_init_tick,   s_init_event,   "INIT" },
    [STATE_READY]       = { s_ready_enter,  s_generic_noop, s_ready_tick,  s_ready_event,  "READY" },
    [STATE_SEQUENCER]   = { s_seq_enter,    s_generic_noop, s_seq_tick,    s_seq_event,    "SEQUENCER" },
    [STATE_POST_FIRE]   = { s_post_enter,   s_generic_noop, s_post_tick,   s_post_event,   "POST_FIRE" },
    [STATE_MANUAL_MODE] = { s_manual_enter, s_manual_exit, s_manual_tick, s_manual_event, "MANUAL" },
    [STATE_ERROR]       = { s_error_enter,  s_error_exit,   s_error_tick,  s_error_event,  "ERROR" },
    [STATE_ABORT]       = { s_abort_enter,  s_abort_exit,   s_abort_tick,  s_abort_event,  "ABORT" }
};

//==============================
// Transition helper
//==============================
static void stager_transition(stager_state_t next){
    if(next == current_state) return;
    if(next > STATE_ABORT) next = STATE_ERROR;
    const stager_state_desc_t *cur = &st_table[current_state];
    const stager_state_desc_t *dst = &st_table[next];
    if(cur->on_exit) cur->on_exit();
    current_state = next;
    if(dst->on_enter) dst->on_enter();
}

//==============================
// Public API (table-driven) + compatibility wrappers
//==============================
bool stager_init(void){
    uint32_t p = crit_enter();
    current_state = STATE_INIT;
    seq_state = SEQUENCER_UNINITIALIZED;
    abort_flag = false; error_flag = false;
    crit_exit(p);
    outputs_safe();
    if(st_table[current_state].on_enter) st_table[current_state].on_enter();
    return true;
}

void stager_tick(void)
{
    // Promote async flags to events (single shot)
    if(abort_flag){ abort_flag=false; evtq_push(STAGER_EVT_ABORT_REQ); }
    if(error_flag){ error_flag=false; evtq_push(STAGER_EVT_ERROR_REQ); }
    // Drain limited number of events to avoid starvation
    uint8_t safety = STAGER_EVENT_QUEUE_SIZE;
    stager_event_t e;
    while(safety-- && evtq_pop(&e)){
        if(st_table[current_state].on_event) st_table[current_state].on_event(e);
    }
    // Per-state periodic tick
    if(st_table[current_state].on_tick) st_table[current_state].on_tick();
}

stager_state_t stager_get_state(void){ return current_state; }
void stager_set_state(stager_state_t s){ stager_transition(s); }

void stager_request_abort_from_isr(void){ abort_flag = true; }
void stager_request_error_from_isr(void){ error_flag = true; }
void stager_request_abort(void){ uint32_t p=crit_enter(); abort_flag=true; crit_exit(p);} 
void stager_request_error(void){ uint32_t p=crit_enter(); error_flag=true; crit_exit(p);} 

const stager_switch_state_t* stager_get_switch_state(void){ return &switch_snapshot; }

void stager_set_switches(uint16_t switches){
    stager_switch_state_t old = switch_snapshot;
    stager_switch_state_t ns = {0};
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
    bool changed_arm=
        (old.master_power       != ns.master_power)       ||
        (old.master_valve       != ns.master_valve)       ||
        (old.master_pyro        != ns.master_pyro)        ||
        (old.sequencer_override != ns.sequencer_override);

    bool changed_valves=
        (old.valve_nitrous_a    != ns.valve_nitrous_a)    ||
        (old.valve_nitrous_b    != ns.valve_nitrous_b)    ||
        (old.valve_nitrogen     != ns.valve_nitrogen)     ||
        (old.valve_discharge    != ns.valve_discharge)    ||
        (old.solenoid           != ns.solenoid);

    if(changed_arm) evtq_push(STAGER_EVT_SWITCH_ARM_CHANGE);
    if(changed_valves) evtq_push(STAGER_EVT_SWITCH_VALVES_CHANGE);
}

//==============================
// Timing constants (ms)
//==============================
#define STAGER_COUNTDOWN_MS   (3000u)
#define STAGER_FIRE_PULSE_MS  (1000u)

//==============================
// INIT State Handlers
//==============================
static void s_init_enter(void)
{ 
    outputs_safe(); 
}
static void s_init_tick(void)
{ 
    // For now just go straight to ready
    stager_transition(STATE_READY);
    if(heartbeat_all_started()) evtq_push(STAGER_EVT_HEARTBEAT_READY); 
}
static void s_init_event(stager_event_t e)
{ 
    if(e==STAGER_EVT_HEARTBEAT_READY) stager_transition(STATE_READY); 
}

//==============================
// READY State
//==============================
static void s_ready_enter(void){ seq_state = SEQUENCER_UNINITIALIZED; }
static void s_ready_tick(void){ /* Idle; switch changes drive events */ }
static void s_ready_event(stager_event_t e)
{
    switch (e)
    {
        case STAGER_EVT_SWITCH_ARM_CHANGE:
            // If the sequencer override is active, switch to manual mode
            if(switch_snapshot.sequencer_override) { stager_transition(STATE_MANUAL_MODE); return; }
            // If both master switches are armed and prefire checks pass, switch to sequencer state
            if(both_armed() && prefire_ok()) { stager_transition(STATE_SEQUENCER); }
            break;

        case STAGER_EVT_SWITCH_VALVES_CHANGE:
            // Handle valve changes if needed (currently no specific action)
            break;

        case STAGER_EVT_ABORT_REQ:
            stager_transition(STATE_ABORT);
            break;

        case STAGER_EVT_ERROR_REQ:
            stager_transition(STATE_ERROR);
            break;
    }
}

//==============================
// SEQUENCER State (simplified skeleton)
//==============================
static void s_seq_enter(void)
{ 
    seq_state = SEQUENCER_READY; 
    seq_t0_ms = HAL_GetTick(); 
}
static void s_seq_tick(void)
{
    switch(seq_state){
        case SEQUENCER_READY:
            // TODO: wait for explicit fire command input (button) -> then COUNTDOWN
            break;
        case SEQUENCER_COUNTDOWN:
            if(!prefire_ok()){ seq_state = SEQUENCER_FAILED_START; }
            else if(HAL_GetTick() - seq_t0_ms >= STAGER_COUNTDOWN_MS){
                spicy_arm();
                spicy_open_ox();
                seq_state = SEQUENCER_FIRE; seq_t0_ms = HAL_GetTick();
            }
            break;
        case SEQUENCER_FIRE:
            if(HAL_GetTick() - seq_t0_ms >= STAGER_FIRE_PULSE_MS){ outputs_safe(); stager_transition(STATE_POST_FIRE); }
            break;
        case SEQUENCER_FAILED_START:
            outputs_safe(); stager_transition(STATE_ERROR); break;
        case SEQUENCER_UNINITIALIZED: default: seq_state = SEQUENCER_READY; break;
    }
}
static void s_seq_event(stager_event_t e)
{
    if(e==STAGER_EVT_ABORT_REQ) stager_transition(STATE_ABORT);
    else if(e==STAGER_EVT_ERROR_REQ) stager_transition(STATE_ERROR);
}

//==============================
// POST_FIRE State
//==============================
static void s_post_enter(void){ /* TODO: logging / cool-down */ }
static void s_post_tick(void){ if(!both_armed()) stager_transition(STATE_READY); }
static void s_post_event(stager_event_t e){ if(e==STAGER_EVT_ABORT_REQ) stager_transition(STATE_ABORT); }

//==============================
// MANUAL_MODE State (hierarchical with two orthogonal sub-FSMs)
//=============================================================
// Left side (Valves): DISARMED -> ARMED -> ACTIVE (send pos cmds)
// Right side (Pyro/Solenoid): SAFE -> ARMED -> SOLENOID_ACTIVE
// Each region can run independently; any global disarm condition collapses both.

typedef enum { VALVE_SUB_DISARMED=0, VALVE_SUB_ARMED, VALVE_SUB_ACTIVE } valve_sub_state_t;
typedef enum { PYRO_SUB_SAFE=0, PYRO_SUB_ARMED, PYRO_SUB_SOLENOID } pyro_sub_state_t;

static valve_sub_state_t valve_sub = VALVE_SUB_DISARMED;
static pyro_sub_state_t  pyro_sub  = PYRO_SUB_SAFE;

//--- Valve helpers (stubs for now) ---------------------------------
static void manual_valve_send_arm(void){
    can_send_command(CAN_NODE_TYPE_SERVO, CAN_NODE_ADDR_BROADCAST, CAN_CMD_SET_SERVO_ARM, 0xFF); // FIXME: confirm format
}
static void manual_valve_send_disarm(void){
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

//--- Pyro/Solenoid helpers -----------------------------------------
static void manual_pyro_arm(void){ if(!spicy_get_arm()) spicy_arm(); }
static void manual_pyro_disarm(void){ if(spicy_get_arm()) spicy_disarm(); }
static void manual_solenoid_on(void){ if(!spicy_get_ox()) spicy_open_ox(); }
static void manual_solenoid_off(void){ if(spicy_get_ox()) spicy_close_ox(); }

//--- Shared collapse to safe ---------------------------------------
static void manual_all_safe(void){
    // Ensure hardware safe
    manual_valve_send_disarm();
    manual_solenoid_off();
    manual_pyro_disarm();
    outputs_safe();
    valve_sub = VALVE_SUB_DISARMED;
    pyro_sub  = PYRO_SUB_SAFE;
}

// Entry/Exit for MANUAL top-level
static void s_manual_enter(void){
    valve_sub = VALVE_SUB_DISARMED;
    pyro_sub  = PYRO_SUB_SAFE;
    // Do not arm anything until operator asserts specific switches.
}
static void s_manual_exit(void){
    manual_all_safe();
}

// Evaluate sub-FSM transitions based on current switches.
static void manual_valve_region_eval(void){
    bool override_on = switch_snapshot.sequencer_override;
    bool valve_master = switch_snapshot.master_valve && override_on; // require override
    if(!override_on){
        // Global collapse
        if(valve_sub != VALVE_SUB_DISARMED) { 
            manual_valve_send_disarm(); 
        }
        valve_sub = VALVE_SUB_DISARMED; return;
    }
    switch(valve_sub){
        case VALVE_SUB_DISARMED:
            if(valve_master) { 
                manual_valve_send_arm(); 
                valve_sub = VALVE_SUB_ARMED;
            }
            break;
        case VALVE_SUB_ARMED:
            if(!valve_master) { 
                manual_valve_send_disarm(); 
                valve_sub = VALVE_SUB_DISARMED;
            }
            else { 
                manual_valve_send_positions(); 
            }
            break;
        default: valve_sub = VALVE_SUB_DISARMED; break;
    }
}

static void manual_pyro_region_eval(void){
    bool override_on = switch_snapshot.sequencer_override;
    bool pyro_master = switch_snapshot.master_pyro && override_on; // require override
    if(!override_on){
        // Global collapse
        if(pyro_sub != PYRO_SUB_SAFE) manual_pyro_disarm();
        manual_solenoid_off();
        pyro_sub = PYRO_SUB_SAFE; return;
    }
    switch(pyro_sub){
        case PYRO_SUB_SAFE:
            if(pyro_master){ 
                manual_pyro_arm(); 
                pyro_sub = PYRO_SUB_ARMED; 
            }
            break;
        case PYRO_SUB_ARMED:
            if(!pyro_master){ 
                manual_pyro_disarm(); 
                manual_solenoid_off(); 
                pyro_sub = PYRO_SUB_SAFE; 
            }
            else if(switch_snapshot.solenoid) { 
                manual_solenoid_on(); 
                pyro_sub = PYRO_SUB_SOLENOID; 
            }
            break;
        case PYRO_SUB_SOLENOID:
            if(!pyro_master) { 
                manual_pyro_disarm(); 
                manual_solenoid_off(); 
                pyro_sub = PYRO_SUB_SAFE; 
            }
            else if(!switch_snapshot.solenoid) { 
                manual_solenoid_off(); 
                pyro_sub = PYRO_SUB_ARMED; 
            }
            break;
        default: pyro_sub = PYRO_SUB_SAFE; break;
    }
}

static void s_manual_tick(void){
    // Periodic safety re-evaluation (could add timeout logic here)
    manual_valve_region_eval();
    manual_pyro_region_eval();
}

static void s_manual_event(stager_event_t e){
    switch(e){
        case STAGER_EVT_SWITCH_ARM_CHANGE:
        case STAGER_EVT_SWITCH_VALVES_CHANGE:
            // Re-evaluate both orthogonal regions
            if(!switch_snapshot.sequencer_override){
                stager_transition(STATE_READY);
                return;
            }
            manual_valve_region_eval();
            manual_pyro_region_eval();
            break;
        case STAGER_EVT_ABORT_REQ:
            stager_transition(STATE_ABORT); break;
        case STAGER_EVT_ERROR_REQ:
            stager_transition(STATE_ERROR); break;
        default: break;
    }
}

//==============================
// ERROR State
//==============================
static void s_error_enter(void){ outputs_safe(); }
static void s_error_exit(void){ /* placeholder for clearing error info */ }
static void s_error_tick(void){ /* remain until cleared by disarm */ }
static void s_error_event(stager_event_t e){
    if(e==STAGER_EVT_SWITCH_ARM_CHANGE){ if(!both_armed()) stager_transition(STATE_READY); }
    else if(e==STAGER_EVT_ABORT_REQ){ stager_transition(STATE_ABORT); }
}

//==============================
// ABORT State
//==============================
static void s_abort_enter(void){ outputs_safe(); }
static void s_abort_exit(void){ /* keep outputs safe */ }
static void s_abort_tick(void){ /* stay until manual clear */ }
static void s_abort_event(stager_event_t e){
    if(e==STAGER_EVT_SWITCH_ARM_CHANGE){ if(!both_armed()) stager_transition(STATE_READY); }
}