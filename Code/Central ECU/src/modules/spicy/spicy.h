#ifndef SPICY_H
#define SPICY_H

#include "stm32g0xx_hal.h"
#include "peripherals.h"
#include "config.h"
#include <stdbool.h>

void spicy_init(void);

// SOFT ARM COMMANDS
void spicy_arm(void);
void spicy_disarm(void);
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
void spicy_open_ox(void);
void spicy_close_ox(void);
bool spicy_get_ox(void); // Check if OX is open

void spicy_fire_ematch1(void);
void spicy_off_ematch1(void);

void spicy_fire_ematch2(void);
void spicy_off_ematch2(void);

#endif // SPICY_H