/**
 * Finite State Machine interface (table-driven implementation).
 *
 * Each state supplies optional enter/exit/tick/event handlers. Only this
 * module mutates the current state; external code posts events or (rarely)
 * requests a transition via fsm_set_state().
 */

#ifndef FSM_H
#define FSM_H

#include "stm32g0xx_hal.h"
#include "config.h"
#include "peripherals.h"
#include <stdbool.h>

#define MAX_MOVE_DURATION 2000u // Maximum duration for a move in milliseconds

//============================
// States
//============================
typedef enum {
    STATE_INIT = 0,
    STATE_READY,
    STATE_MOVING,
    STATE_ERROR,
    STATE_COUNT // Not an actual state, just the number of states as zero indexed
} fsm_state_t;

//============================
// Events (extend as required)
//============================
typedef enum {
    FSM_EVENT_NONE = 0,
    FSM_EVENT_INIT_DONE,          // INIT complete, proceed to READY
    FSM_EVENT_QUEUE_ITEM_ADDED,   // A movement was queued (could trigger MOVING)
    FSM_EVENT_MOVE_COMPLETE_OK,   // Current movement finished successfully
    FSM_EVENT_MOVE_COMPLETE_FAIL, // Movement failed (timeout / error)
    FSM_EVENT_CLEAR_ERROR,        // External request to leave ERROR state
    FSM_EVENT_EXTERNAL_DISARM,    // External disarm request (CAN etc.)
    FSM_EVENT_HEARTBEAT_TIMEOUT,  // Heartbeat timeout event
    FSM_EVENT_COUNT
} fsm_event_t;

//============================
// Public API
//============================
void fsm_init(void);            // Initialize and enter INIT state
void fsm_tick(void);            // Process queued events then tick current state
bool fsm_dispatch(fsm_event_t e); // Queue an event (ISR-safe if single producer)
void fsm_set_state(fsm_state_t state); // Forced transition (prefer events)
fsm_state_t fsm_get_state(void);

// Convenience helpers
static inline bool fsm_is_error(void) { return fsm_get_state() == STATE_ERROR; }

#endif // FSM_H