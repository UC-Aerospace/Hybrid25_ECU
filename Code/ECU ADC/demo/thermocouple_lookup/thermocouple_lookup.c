// Example usage:
#include "../../src/modules/thermocouple_k/thermocouple_k.h"
#include <stdio.h>

int main() {
    float voltage_uv;
    int temp;
    int result;
    
    // Convert temperature to voltage
    result = tc_k_temp_to_voltage(100, &voltage_uv);
    if (result == TC_K_OK) {
        printf("100C = %f uV (%.3f mV)\n", voltage_uv, voltage_uv / 1000);
    }
    
    // Convert voltage to temperature
    result = tc_k_voltage_to_temp(4.096, &temp);
    if (result == TC_K_OK) {
        printf("4096000 uV (4.096 mV) = %dC\n", temp);
    }
    
    return 0;
}
 