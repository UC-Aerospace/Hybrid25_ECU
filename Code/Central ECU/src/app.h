/*
    Main header file for the application.
*/

#ifndef APP_H
#define APP_H

#include "stm32g0xx_hal.h"
#include "peripherals.h"
#include "config.h"

void app_init(void);
void app_run(void);

#endif // APP_H