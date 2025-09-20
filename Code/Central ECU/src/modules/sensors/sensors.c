#include "sensors.h"

sensor_reading_t sensor_data[3][3] = {{0}};

// Convert thermocouple ADC counts (int16) to centi-Celsius using a 7th-degree polynomial.
// Coefficients derived from Python (NumPy) fit:
// coeffs = [-8.46349804e-27,  7.10985650e-22, -2.14114276e-17,  2.48424392e-13,
//            -1.63198889e-10, -1.40441053e-05,  1.52510733e+00, -2.25001136e+00]
// The evaluation uses Horner's method with single-precision floats for efficiency on Cortex-M0+.
static inline int32_t thermo_counts_to_centiC(int16_t counts)
{
    const float x = (float)counts;
    // Horner's method, highest degree first
    float y = -8.46349804e-27f;
    y = y * x + 7.10985650e-22f;
    y = y * x + -2.14114276e-17f;
    y = y * x + 2.48424392e-13f;
    y = y * x + -1.63198889e-10f;
    y = y * x + -1.40441053e-05f;
    y = y * x + 1.52510733e+00f;
    y = y * x + -2.25001136e+00f;

    // Round to nearest integer centi-Celsius
    return (int32_t)(y + (y >= 0.0f ? 0.5f : -0.5f));
}

// Stores pressure as 
void sensors_add_pressure(uint8_t id, uint16_t first_sample, uint16_t reference) {
    // Add a pressure sensor reading
    // value is the raw ADC value (0-65535)
    // reference is the reference voltage (0-65535) corresponding to 0-100% of sensor range
    // Map value such that 0.1 * reference is the zero output level and 0.9 * reference is 0xFFFF
    uint32_t in_min = reference / 10;         // 0.1 * reference
    uint32_t in_max = (reference * 9) / 10;   // 0.9 * reference

    if (first_sample <= in_min) first_sample = 0x0000;
    else if (first_sample >= in_max) first_sample = 0xFFFF;
    else {
        uint32_t range = in_max - in_min;
        uint32_t scaled = (first_sample - in_min) * 65535UL / range;
        first_sample = (uint16_t)scaled;
    }
    // Convert to 10*bar, reading correspond to 0-50bar with PTE7100
    int32_t pressure_mbar = (first_sample * 200UL) / 13107UL;

    uint8_t type = (id >> 3) & 0x03; // Bits 3-4 for type
    uint8_t sub_id = id & 0x07;      // Bits 0
    if (type <= 2 && sub_id <= 2) {
        sensor_data[type][sub_id].value = pressure_mbar;
        sensor_data[type][sub_id].rx_time = HAL_GetTick();
    }
}

void sensors_add_temperature(uint8_t id, int16_t value) {
    // Add a temperature sensor reading
    // Value is thermocouple voltage in ADC counts; convert to centi-Celsius via polynomial fit
    uint8_t type = (id >> 3) & 0x03; // Bits 3-4 for type
    uint8_t sub_id = id & 0x07;      // Bits 0-2 for sub ID
    if (type <= 2 && sub_id <= 2) {
        int32_t centi_c = thermo_counts_to_centiC(value);
        sensor_data[type][sub_id].value = centi_c;
        sensor_data[type][sub_id].rx_time = HAL_GetTick();
    }
}

void sensors_add_pt(uint8_t id, int16_t value) {
    // Add a PT sensor reading (only pressure part)
    // Value maps from 0->100bar with values -16000->16000
    // Converts to millibar
    uint8_t type = (id >> 3) & 0x03; // Bits 3-4 for type
    uint8_t sub_id = id & 0x07;      // Bits 0-2 for sub ID
    int32_t pressure_mbar = ((int32_t)value + 16000) / 32; // Scale to bar * 10
    if (type <= 2 && sub_id <= 2) {
        sensor_data[type][sub_id].value = pressure_mbar;
        sensor_data[type][sub_id].rx_time = HAL_GetTick();
    }
}

int32_t sensors_get_data(uint8_t id) {
    // Get the latest sensor reading by ID
    uint8_t type = (id >> 3) & 0x03; // Bits 3-4 for type
    uint8_t sub_id = id & 0x07;      // Bits 0-2 for sub ID
    if (type <= 2 && sub_id <= 2) {
        sensor_reading_t reading = sensor_data[type][sub_id];
        // If data is older than 5 seconds, consider it stale and return -1
        if (HAL_GetTick() - reading.rx_time > 5000) {
            return -1;
        }
        return reading.value;
    } else {
        return -1;
    }
}