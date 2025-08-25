#include "manual_solenoid.h"
#include "spicy.h"
#include "main_FSM.h"

pyro_states_t pyro_state = PYRO_SAFE;

// Pyro/Solenoid helpers
void manual_pyro_arm(void){ if(!spicy_get_arm()) spicy_arm(); }
void manual_pyro_disarm(void){ if(spicy_get_arm()) spicy_disarm(); }
void manual_solenoid_on(void){ if(!spicy_get_ox()) spicy_open_ox(); }
void manual_solenoid_off(void){ if(spicy_get_ox()) spicy_close_ox(); }

void pyro_state_decoder(void)
{
    bool override_on = switch_snapshot.sequencer_override;
    bool pyro_master = switch_snapshot.master_pyro && override_on; // require override

    if(!override_on){
        // Global collapse
        if(pyro_state != PYRO_SAFE) manual_pyro_disarm();
        manual_solenoid_off();
        pyro_state = PYRO_SAFE; return;
    }

    switch(pyro_state){
        case PYRO_SAFE:
            if(pyro_master){ 
                manual_pyro_arm(); 
                pyro_state = PYRO_ARMED; 
            }
            break;

        case PYRO_ARMED:
            if(!pyro_master){ 
                manual_pyro_disarm(); 
                manual_solenoid_off(); 
                pyro_state = PYRO_SAFE; 
            }
            else if(switch_snapshot.solenoid) { 
                manual_solenoid_on(); 
                pyro_state = PYRO_SOLENOID; 
            }
            break;

        case PYRO_SOLENOID:
            if(!pyro_master) { 
                manual_pyro_disarm(); 
                manual_solenoid_off(); 
                pyro_state = PYRO_SAFE; 
            }
            else if(!switch_snapshot.solenoid) { 
                manual_solenoid_off(); 
                pyro_state = PYRO_ARMED; 
            }
            break;

        default: 
            pyro_state = PYRO_SAFE; 
            break;
    }
}