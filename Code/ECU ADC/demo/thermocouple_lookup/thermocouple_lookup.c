// Example usage:
#include "../src/thermocouple_k.h"
#include <stdio.h>

int main() {
    int32_t voltage_uv;
    int16_t temp;
    int result;
    
    // Convert temperature to voltage
    result = tc_k_temp_to_voltage(100, &voltage_uv);
    if (result == TC_K_OK) {
        printf("100°C = %d µV (%.3f mV)\n", voltage_uv, voltage_uv / 1000.0);
    }
    
    // Convert voltage to temperature
    result = tc_k_voltage_to_temp(4096000, &temp);
    if (result == TC_K_OK) {
        printf("4096000 µV (4.096 mV) = %d°C\n", temp);
    }
    
    return 0;
}
 