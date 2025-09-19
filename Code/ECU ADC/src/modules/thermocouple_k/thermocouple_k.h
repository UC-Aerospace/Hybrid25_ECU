#ifndef THERMOCOUPLE_K_H
#define THERMOCOUPLE_K_H

/**
 * Type K Thermocouple Lookup Table Module
 * Based on ITS-90 Standard
 * 
 * Temperature range: -270°C to +1372°C
 * Voltage range: -6458µV to +54886µV (use micro to avoid floats)
 * 
 */

#include <stdint.h>

// Return codes
typedef enum {
    TC_K_ERROR_NULL_POINTER = -2,
    TC_K_ERROR_OUT_OF_RANGE,
    TC_K_OK
} TC_K_RETURN_t;

// Temperature and voltage ranges
#define TC_K_TEMP_MIN   -270
#define TC_K_TEMP_MAX   1372
#define TC_K_VOLTAGE_MIN -6458  // microvolts
#define TC_K_VOLTAGE_MAX 54886  // microvolts

/**
 * Convert temperature to thermoelectric voltage
 * @param temp_c Temperature in degrees Celsius
 * @param voltage_uv Pointer to store the voltage result in microvolts
 * @return TC_K_OK on success, error code on failure
 */
int tc_k_temp_to_voltage(int temp_c, float *voltage_mv);

/**
 * Convert thermoelectric voltage to temperature
 * @param voltage_uv Voltage in microvolts
 * @param temp_c Pointer to store the temperature result in degrees Celsius
 * @return TC_K_OK on success, error code on failure
 */
int tc_k_voltage_to_temp(float voltage_mv, int *temp_c);

/**
 * Get human-readable error message
 * @param error_code Error code from tc_k functions
 * @return Pointer to error message string
 */
const char* tc_k_get_error_message(int error_code);

#endif // THERMOCOUPLE_K_H