//====================================================
// Table-driven FSM implementation
//====================================================

#include "fsm.h"
#include "servo.h"
#include "debug_io.h"

//------------------------------
// Internal state
//------------------------------
static fsm_state_t currentState = STATE_INIT;
static uint32_t move_start_ms = 0u;

//------------------------------
// Simple ring buffer for events
//------------------------------
#define FSM_EVENT_QUEUE_SIZE 8
typedef struct {
    fsm_event_t events[FSM_EVENT_QUEUE_SIZE];
    uint8_t head; // dequeue
    uint8_t tail; // enqueue
} fsm_event_queue_t;

static fsm_event_queue_t eventQueue = { .events = { FSM_EVENT_NONE }, .head = 0u, .tail = 0u };

static bool event_queue_is_full(void) {
    return (uint8_t)((eventQueue.tail + 1u) % FSM_EVENT_QUEUE_SIZE) == eventQueue.head;
}

static bool event_queue_is_empty(void) {
    return eventQueue.head == eventQueue.tail;
}

static bool event_queue_enqueue(fsm_event_t e) {
    if (e == FSM_EVENT_NONE) return true; // ignore silently
    if (event_queue_is_full()) return false;
    eventQueue.events[eventQueue.tail] = e;
    eventQueue.tail = (uint8_t)((eventQueue.tail + 1u) % FSM_EVENT_QUEUE_SIZE);
    return true;
}

static bool event_queue_dequeue(fsm_event_t *out) {
    if (event_queue_is_empty()) return false;
    *out = eventQueue.events[eventQueue.head];
    eventQueue.head = (uint8_t)((eventQueue.head + 1u) % FSM_EVENT_QUEUE_SIZE);
    return true;
}

//------------------------------
// State handler function types
//------------------------------
typedef void (*fsm_enter_fn)(void);
typedef void (*fsm_exit_fn)(void);
typedef void (*fsm_tick_fn)(void);      // Called every fsm_tick()
typedef void (*fsm_event_fn)(fsm_event_t e);

typedef struct {
    fsm_enter_fn on_enter;
    fsm_exit_fn  on_exit;
    fsm_tick_fn  on_tick;
    fsm_event_fn on_event;
    const char  *name;
} fsm_state_desc_t;

// Forward declarations of handlers per state
static void init_enter(void);   static void init_tick(void);   static void init_event(fsm_event_t e);
static void ready_enter(void);  static void ready_tick(void);  static void ready_event(fsm_event_t e);
static void moving_enter(void); static void moving_tick(void); static void moving_event(fsm_event_t e);
static void error_enter(void);  static void error_tick(void);  static void error_event(fsm_event_t e);

// Exit handlers (mostly empty now)
static void generic_noop(void) {}
static void moving_exit(void);
static void error_exit(void);

//------------------------------
// State table
//------------------------------
static const fsm_state_desc_t stateTable[STATE_COUNT] = {
    [STATE_INIT]   = { init_enter,   generic_noop, init_tick,   init_event,   "INIT" },
    [STATE_READY]  = { ready_enter,  generic_noop, ready_tick,  ready_event,  "READY" },
    [STATE_MOVING] = { moving_enter, moving_exit, moving_tick, moving_event, "MOVING" },
    [STATE_ERROR]  = { error_enter,  error_exit,  error_tick,  error_event,  "ERROR" }
};

//------------------------------
// Transition helper
//------------------------------
static void fsm_transition(fsm_state_t next) {
    if (next >= STATE_COUNT) {
        next = STATE_ERROR; // clamp invalid
    }
    if (next == currentState) return;
    const fsm_state_desc_t *cur = &stateTable[currentState];
    const fsm_state_desc_t *dst = &stateTable[next];
    if (cur->on_exit) cur->on_exit();
    dbg_printf("FSM: %s -> %s\r\n", cur->name, dst->name);
    currentState = next;
    if (dst->on_enter) dst->on_enter();
    servo_send_status(); // Update status on any state change
}

//------------------------------
// Public API
//------------------------------
void fsm_init(void) {
    currentState = STATE_INIT;
    // Run enter explicitly
    if (stateTable[currentState].on_enter) stateTable[currentState].on_enter();
}

void fsm_tick(void) {
    // Drain events first (bounded to prevent starvation)
    uint8_t safety = FSM_EVENT_QUEUE_SIZE;
    fsm_event_t e;
    while (safety-- && event_queue_dequeue(&e)) {
        if (stateTable[currentState].on_event) stateTable[currentState].on_event(e);
    }
    // Periodic tick for current state
    if (stateTable[currentState].on_tick) stateTable[currentState].on_tick();
}

bool fsm_dispatch(fsm_event_t e) {
    return event_queue_enqueue(e);
}

void fsm_set_state(fsm_state_t state) {
    fsm_transition(state);
}

fsm_state_t fsm_get_state(void) { return currentState; }

//====================================================
// INIT State
//====================================================
static void init_enter(void) {
    // Placeholder: could validate hardware, wait for sync, etc.
    dbg_printf("FSM: Enter INIT\r\n");

    #ifdef DEBUG // Auto progress to READY when in debug mode
    fsm_dispatch(FSM_EVENT_INIT_DONE);
    #endif
}

static void init_tick(void) { /* no-op */ }

static void init_event(fsm_event_t e) {
    if (e == FSM_EVENT_INIT_DONE) {
        fsm_transition(STATE_READY);
    }
}

//====================================================
// READY State
//====================================================
static void ready_enter(void) {
    dbg_printf("FSM: Enter READY\r\n");
}

static void ready_tick(void) {
    // If there is work, start moving (mirror old logic)
    if (servo_queue_poll()) {
        fsm_transition(STATE_MOVING);
    }
}

static void ready_event(fsm_event_t e) {
    switch (e) {
        case FSM_EVENT_EXTERNAL_DISARM:
            // For now do nothing
            break;
        case FSM_EVENT_MOVE_COMPLETE_FAIL: // Should not happen in READY, treat as error
            fsm_transition(STATE_ERROR);
            break;
        case FSM_EVENT_HEARTBEAT_TIMEOUT:
            fsm_transition(STATE_ERROR);
            break;
        default: break;
    }
}

//====================================================
// MOVING State
//====================================================
static void moving_enter(void) {
    dbg_printf("FSM: Enter MOVING\r\n");
    move_start_ms = HAL_GetTick();
}

static void moving_exit(void) {
    // Nothing extra yet
}

static void moving_tick(void) {
    // Check for completion (mirrors old logic)
    if (servo_queue_check_complete()) {
        servo_queue_complete(true);
        fsm_transition(STATE_READY);
        return;
    }
    uint32_t elapsed = HAL_GetTick() - move_start_ms;
    if (elapsed > MAX_MOVE_DURATION) {
        servo_queue_complete(false); // will set error etc.
        fsm_transition(STATE_ERROR);
    }
}

static void moving_event(fsm_event_t e) {
    switch (e) {
        case FSM_EVENT_MOVE_COMPLETE_OK:
            servo_queue_complete(true);
            fsm_transition(STATE_READY);
            break;
        case FSM_EVENT_MOVE_COMPLETE_FAIL:
            servo_queue_complete(false);
            fsm_transition(STATE_ERROR);
            break;
        case FSM_EVENT_EXTERNAL_DISARM:
            // External disarm request while moving -> treat as fail/cancel
            servo_queue_complete(false);
            fsm_transition(STATE_READY); 
            break;
        case FSM_EVENT_HEARTBEAT_TIMEOUT:
            fsm_transition(STATE_ERROR);
            break;
        default: break;
    }
}

//====================================================
// ERROR State
//====================================================
static void error_enter(void) {
    dbg_printf("FSM: Enter ERROR\r\n");
    servo_disarm_all();
    servo_queue_clear();
}

static void error_exit(void) {
    servo_queue_clear();
}

static void error_tick(void) { /* Idle in error until cleared */ }

static void error_event(fsm_event_t e) {
    switch (e) {
        case FSM_EVENT_CLEAR_ERROR:
        case FSM_EVENT_EXTERNAL_DISARM: // CAN disarm command can clear error
            fsm_transition(STATE_READY);
            break;
        case FSM_EVENT_HEARTBEAT_TIMEOUT:
            fsm_transition(STATE_ERROR);
            break;
        default: break; // ignore other events
    }
}
