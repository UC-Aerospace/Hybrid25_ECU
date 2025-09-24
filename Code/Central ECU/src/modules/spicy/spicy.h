#ifndef SPICY_H
#define SPICY_H

#include "stm32g0xx_hal.h"
#include "peripherals.h"
#include "config.h"
#include <stdbool.h>

void spicy_init(void);
void spicy_send_status_update(void);

// SOFT ARM COMMANDS
bool spicy_arm(void);
bool spicy_disarm(void);
bool spicy_get_arm(void);

// COMPARATOR GET COMMANDS
bool comp_get_interlock(void);
bool comp_get_oxlow(void);
bool comp_get_ematch1(void);
bool comp_get_ematch2(void);

// HIGH SIDE SWITCH GET COMMANDS
bool hss_get_IMON(void);
bool hss_get_PGOOD(void);

// FIRE COMMANDS
bool spicy_open_solenoid(void);
bool spicy_close_solenoid(void);
bool spicy_get_solenoid(void); // Check if OX is open

bool spicy_fire_ematch1(void);
bool spicy_off_ematch1(void);
bool spicy_get_ematch1(void);

bool spicy_fire_ematch2(void);
bool spicy_off_ematch2(void);

#endif // SPICY_H