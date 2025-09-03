#include "manual_solenoid.h"

#include "spicy.h"
#include "debug_io.h"

static manual_solenoid_state_t pyro_state = PYRO_SAFE;

// Pyro/Solenoid helpers
static void manual_pyro_arm(void){ if(!spicy_get_arm()) spicy_arm(); }
static void manual_pyro_disarm(void){ if(spicy_get_arm()) spicy_disarm(); }
static void manual_solenoid_on(void){ if(!spicy_get_solenoid()) spicy_open_solenoid(); }
static void manual_solenoid_off(void){ if(spicy_get_solenoid()) spicy_close_solenoid(); }

void manual_solenoid_tick(switch_state_t *switch_snapshot)
{
    bool override_on = switch_snapshot->sequencer_override;
    bool pyro_master = switch_snapshot->master_pyro && override_on; // require override

    switch(pyro_state){
        case PYRO_SAFE:
            if(pyro_master){ 
                manual_pyro_arm(); 
                dbg_printf("MANUAL PYRO: ARM\n");
                pyro_state = PYRO_ARMED; 
            }
            break;

        case PYRO_ARMED:
            if(!pyro_master){ 
                manual_solenoid_set_safe();
            }
            else if(switch_snapshot->solenoid) { 
                manual_solenoid_on();
                dbg_printf("MANUAL PYRO: SOLENOID ON\n");
                pyro_state = PYRO_SOLENOID; 
            }
            break;

        case PYRO_SOLENOID:
            if(!pyro_master) { 
                manual_solenoid_set_safe();
            }
            else if(!switch_snapshot->solenoid) { 
                manual_solenoid_off();
                dbg_printf("MANUAL PYRO: SOLENOID OFF\n");
                pyro_state = PYRO_ARMED; 
            }
            break;

        default:
            manual_solenoid_set_safe();
            pyro_state = PYRO_SAFE;
            dbg_printf("MANUAL PYRO: ERROR, DEFAULT\n");
            break;
    }
}

void manual_solenoid_set_safe(void)
{
    manual_solenoid_off();
    manual_pyro_disarm();
    pyro_state = PYRO_SAFE;
    dbg_printf("MANUAL PYRO: SET SAFE\n");
}