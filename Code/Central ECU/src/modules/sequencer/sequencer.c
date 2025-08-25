#include "sequencer.h"
#include "main_FSM.h"
#include "stm32g0xx_hal.h"
#include "debug_io.h"
#include "spicy.h"

#define COUNTDOWN_MS 3000u
#define SEQUENCER_FIRE_MS 10000u

bool fire = false;

typedef enum {
    SEQUENCER_READY = 0b0000,
    SEQUENCER_COUNTDOWN = 0b0001,
    SEQUENCER_FIRE = 0b0010,
    SEQUENCER_FAILED_START = 0b1111,
    SEQUENCER_UNINITIALISED = 0b1110
} sequencer_states_t;

static sequencer_states_t sequencer_state = SEQUENCER_UNINITIALISED;
static uint32_t sequencer_start_tick = 0u;

void unititialise_sequencer(void)
{
    fire = false;
    sequencer_state = SEQUENCER_UNINITIALISED;
}

void set_sequencer_ready(void)
{
    sequencer_state = SEQUENCER_READY;
}

void sequencer_tick(void)
{
    switch(sequencer_state){
        case SEQUENCER_READY:
            if(fire) {
                sequencer_state = SEQUENCER_COUNTDOWN;
                sequencer_start_tick = HAL_GetTick();
            }
            break;
        
        case SEQUENCER_COUNTDOWN:
            if(!prefire_ok()) {
                sequencer_state = SEQUENCER_FAILED_START;
                dbg_printf("Sequencer not safe, failed start");
            } else if (HAL_GetTick() - sequencer_start_tick >= COUNTDOWN_MS) {
                spicy_open_ox();
                spicy_arm();
                sequencer_state = SEQUENCER_FIRE;
                dbg_printf("Sequencer countdown, opened ox, armed spicy");
            }
            // Question for Brad -> Can I please have more sequencer states, I need additional states to implement the T-6 sections etc
            break;
        
        case SEQUENCER_FIRE:
            if (HAL_GetTick() - sequencer_start_tick >= SEQUENCER_FIRE_MS) {
                // TODO: IGNITE!!!!
                dbg_printf("Sequencer fire, candle lit");
                // TODO: POST FIRE THINGS
                fsm_set_state(STATE_POST_FIRE);
            }
            break;
        
        case SEQUENCER_FAILED_START:
            outputs_safe();
            fsm_set_state(STATE_ERROR);
            dbg_printf("Sequencer failed start, secured system and moved to error state");
            break;

        case SEQUENCER_UNINITIALISED:
            break;

        default:
            sequencer_state = SEQUENCER_READY;
            dbg_printf("Sequencer defaulted");
            break;
    }
}