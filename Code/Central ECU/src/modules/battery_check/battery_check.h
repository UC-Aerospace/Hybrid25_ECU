#ifndef BATTERY_CHECK_H
#define BATTERY_CHECK_H

#include "stm32g0xx_hal.h"
#include "peripherals.h"
#include "config.h"

// Structures

typedef struct {
    uint16_t voltage_6s; // Voltage * 1000 of 6S battery
    uint16_t voltage_2s; // Voltage * 1000 of 2S battery
} BatteryStatus;

// Functions

// Returns the battery voltages
BatteryStatus batt_get_volt(void);

// Will print the battery voltages, should really throw into an error when low but need to sort out how that will work.
void batt_check(void);

// Converts voltage to state of charge (SOC) percentage
uint8_t batt_volt_to_soc(uint16_t voltage_mV, uint8_t cell_count);


#endif // BATTERY_CHECK_H