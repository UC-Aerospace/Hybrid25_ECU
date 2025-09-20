#ifndef SENSORS_H
#define SENSORS_H

#include "stm32g0xx_hal.h"
#include "config.h"

#define SENSOR_P_CHAMBER 0u   // MIPA A ID
#define SENSOR_P_MANIFOLD 1u  // MIPA B ID

#define SENSOR_LC_Thrust 24u  // LC Thrust ID
#define SENSOR_LC_N2O_A 25u   // LC N2O A ID
#define SENSOR_LC_N2O_B 26u   // LC N2O B ID

#define SENSOR_PT_MAINLINE 16u  // PT A ID
#define SENSOR_PT_BRANCH_A 17u  // PT B ID
#define SENSOR_PT_BRANCH_B 18u  // PT C ID

#define SENSOR_THERMO_A 8u   // Thermo A ID
#define SENSOR_THERMO_B 9u   // Thermo B ID
#define SENSOR_THERMO_C 10u  // Thermo C ID 
#define SENSOR_CJT 11u       // CJT ID

typedef struct {
    int32_t value;
    int32_t rx_time;
} sensor_reading_t;

void sensors_add_pressure(uint8_t id, uint16_t value, uint16_t reference);
void sensors_add_temperature(uint8_t id, int16_t value);
void sensors_add_pt(uint8_t id, int16_t value);                             // Only decodes the pressure part of PT sensors

int32_t sensors_get_data(uint8_t id);

#endif // SENSORS_H