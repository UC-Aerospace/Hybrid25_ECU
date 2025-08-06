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

// Define a struct for tasks
typedef struct {
    uint32_t last_run_time; // Last time the task was run
    uint32_t interval;       // Interval in milliseconds
    void (*task_function)(void); // Pointer to the task function
} Task;

#endif // APP_H