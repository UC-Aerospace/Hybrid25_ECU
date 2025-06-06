#include "app.h"
#include "debug_io.h"

void app_init(void) {
    // Initialize the application
    HAL_Delay(5000);
    uint32_t uid[3];

    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();

    dbg_printf("UID: %08lX %08lX %08lX\n", uid[0], uid[1], uid[2]); // Print the unique device ID to subsequently lock firmware to this device
}

void app_run(void) {
    // Main application loop
    // This function can be used to run the main logic of the application.
    while (1) {
        // Application logic goes here
    }
}