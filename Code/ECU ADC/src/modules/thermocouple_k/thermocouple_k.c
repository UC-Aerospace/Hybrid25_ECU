/*
 * Implementation
 */

#include <math.h>
#include <stddef.h>
#include "thermocouple_k.h"

// Lookup table structure
typedef struct {
    int temp;
    float voltage;
} tc_k_entry_t;

// ITS-90 Type K Thermocouple lookup table (condensed for key points)
// Full table with 1-degree resolution from -270°C to +1372°C
static const tc_k_entry_t tc_k_table[] = {
    // Negative temperatures
    {-270, -6.458f}, {-260, -6.441f}, {-250, -6.404f}, {-240, -6.344f},
    {-230, -6.262f}, {-220, -6.158f}, {-210, -6.035f}, {-200, -5.891f},
    {-190, -5.730f}, {-180, -5.550f}, {-170, -5.354f}, {-160, -5.141f},
    {-150, -4.913f}, {-140, -4.669f}, {-130, -4.411f}, {-120, -4.138f},
    {-110, -3.852f}, {-100, -3.554f}, {-90, -3.243f}, {-80, -2.920f},
    {-70, -2.587f}, {-60, -2.243f}, {-50, -1.889f}, {-40, -1.527f},
    {-30, -1.156f}, {-20, -0.778f}, {-10, -0.392f},
    
    // Zero and positive temperatures (every 10 degrees for space)
    {0, 0.000f}, {10, 0.397f}, {20, 0.798f}, {30, 1.203f}, {40, 1.612f},
    {50, 2.023f}, {60, 2.436f}, {70, 2.851f}, {80, 3.267f}, {90, 3.682f},
    {100, 4.096f}, {110, 4.509f}, {120, 4.920f}, {130, 5.328f}, {140, 5.735f},
    {150, 6.138f}, {160, 6.540f}, {170, 6.941f}, {180, 7.340f}, {190, 7.739f},
    {200, 8.138f}, {210, 8.539f}, {220, 8.940f}, {230, 9.343f}, {240, 9.747f},
    {250, 10.153f}, {260, 10.561f}, {270, 10.971f}, {280, 11.382f}, {290, 11.795f},
    {300, 12.209f}, {310, 12.624f}, {320, 13.040f}, {330, 13.457f}, {340, 13.874f},
    {350, 14.293f}, {360, 14.713f}, {370, 15.133f}, {380, 15.554f}, {390, 15.975f},
    {400, 16.397f}, {450, 18.516f}, {500, 20.644f}, {550, 22.776f}, {600, 24.905f},
    {650, 27.025f}, {700, 29.129f}, {750, 31.213f}, {800, 33.275f}, {850, 35.313f},
    {900, 37.326f}, {950, 39.314f}, {1000, 41.276f}, {1050, 43.211f}, {1100, 45.119f},
    {1150, 46.995f}, {1200, 48.838f}, {1250, 50.644f}, {1300, 52.410f}, {1350, 54.138f},
    {1372, 54.886f}
};

#define TC_K_TABLE_SIZE (sizeof(tc_k_table) / sizeof(tc_k_entry_t))

/**
 * Linear interpolation between two points
 */
static float interpolate(float x0, float y0, float x1, float y1, float x) {
    if (x1 == x0) return y0;
    return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
}

/**
 * Binary search to find the closest table entries for interpolation
 */
static int find_temp_index(int temp) {
    int left = 0;
    int right = TC_K_TABLE_SIZE - 1;
    
    while (right - left > 1) {
        int mid = (left + right) / 2;
        if (tc_k_table[mid].temp <= temp) {
            left = mid;
        } else {
            right = mid;
        }
    }
    return left;
}

static int find_voltage_index(float voltage) {
    int left = 0;
    int right = TC_K_TABLE_SIZE - 1;
    
    while (right - left > 1) {
        int mid = (left + right) / 2;
        if (tc_k_table[mid].voltage <= voltage) {
            left = mid;
        } else {
            right = mid;
        }
    }
    return left;
}

int tc_k_temp_to_voltage(int temp_c, float *voltage_mv) {
    if (voltage_mv == NULL) {
        return TC_K_ERROR_NULL_POINTER;
    }
    
    if (temp_c < TC_K_TEMP_MIN || temp_c > TC_K_TEMP_MAX) {
        return TC_K_ERROR_OUT_OF_RANGE;
    }
    
    // Find the surrounding table entries
    uint32_t idx = find_temp_index(temp_c);
    
    // Handle exact matches
    if (tc_k_table[idx].temp == temp_c) {
        *voltage_mv = tc_k_table[idx].voltage;
        return TC_K_OK;
    }
    
    // Handle boundary cases
    if (idx >= TC_K_TABLE_SIZE - 1) {
        *voltage_mv = tc_k_table[TC_K_TABLE_SIZE - 1].voltage;
        return TC_K_OK;
    }
    
    // Linear interpolation
    *voltage_mv = interpolate(
        (float)tc_k_table[idx].temp, tc_k_table[idx].voltage,
        (float)tc_k_table[idx + 1].temp, tc_k_table[idx + 1].voltage,
        (float)temp_c
    );
    
    return TC_K_OK;
}

int tc_k_voltage_to_temp(float voltage_mv, int *temp_c) {
    if (temp_c == NULL) {
        return TC_K_ERROR_NULL_POINTER;
    }
    
    if (voltage_mv < TC_K_VOLTAGE_MIN || voltage_mv > TC_K_VOLTAGE_MAX) {
        return TC_K_ERROR_OUT_OF_RANGE;
    }
    
    // Find the surrounding table entries
    uint32_t idx = find_voltage_index(voltage_mv);
    
    // Handle boundary cases
    if (idx >= TC_K_TABLE_SIZE - 1) {
        *temp_c = tc_k_table[TC_K_TABLE_SIZE - 1].temp;
        return TC_K_OK;
    }
    
    // Linear interpolation
    float temp_float = interpolate(
        tc_k_table[idx].voltage, (float)tc_k_table[idx].temp,
        tc_k_table[idx + 1].voltage, (float)tc_k_table[idx + 1].temp,
        voltage_mv
    );
    
    // Round to nearest whole number
    *temp_c = (int)(temp_float + (temp_float >= 0 ? 0.5f : -0.5f));
    
    return TC_K_OK;
}

const char* tc_k_get_error_message(int error_code) {
    switch (error_code) {
        case TC_K_OK:
            return "Success";
        case TC_K_ERROR_OUT_OF_RANGE:
            return "Temperature or voltage out of range";
        case TC_K_ERROR_NULL_POINTER:
            return "Null pointer provided";
        default:
            return "Unknown error";
    }
}