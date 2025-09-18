#ifndef CONFIG_H
#define CONFIG_H

#include "stm32g0xx_hal.h"

// WHOAMI

// Board types: _SERVO, _CENTRAL, _ADC
#define BOARD_TYPE_ADC
extern uint8_t BOARD_ID;

#define TEST_MODE // TODO: Remove before production

#define BOARD_ID_RIU 0
#define BOARD_ID_ECU 1
#define BOARD_ID_SERVO 2
#define BOARD_ID_ADC_A 3

// Pinout configuration

#define CJT_A0_Pin GPIO_PIN_0
#define CJT_A0_GPIO_Port GPIOA
#define MIPAA_A1_Pin GPIO_PIN_1
#define MIPAA_A1_GPIO_Port GPIOA
#define MIPAB_A2_Pin GPIO_PIN_2
#define MIPAB_A2_GPIO_Port GPIOA
#define MIPAC_A3_Pin GPIO_PIN_3
#define MIPAC_A3_GPIO_Port GPIOA
#define SPI_CS_Pin GPIO_PIN_4
#define SPI_CS_GPIO_Port GPIOA
#define SYNC_Pin GPIO_PIN_4
#define SYNC_GPIO_Port GPIOC
#define ADC_RDY_Pin GPIO_PIN_5
#define ADC_RDY_GPIO_Port GPIOC
#define ADC_RST_Pin GPIO_PIN_0
#define ADC_RST_GPIO_Port GPIOB
#define REF5V_B1_Pin GPIO_PIN_1
#define REF5V_B1_GPIO_Port GPIOB
#define V2S_A10_Pin GPIO_PIN_2
#define V2S_A10_GPIO_Port GPIOB
#define A11_B10_Pin GPIO_PIN_10
#define A11_B10_GPIO_Port GPIOB
#define LED_IND_ERROR_Pin GPIO_PIN_9
#define LED_IND_ERROR_GPIO_Port GPIOC
#define LED_IND_STATUS_Pin GPIO_PIN_0
#define LED_IND_STATUS_GPIO_Port GPIOD
#define E1_Pin GPIO_PIN_1
#define E1_GPIO_Port GPIOD
#define E2_Pin GPIO_PIN_2
#define E2_GPIO_Port GPIOD
#define E3_Pin GPIO_PIN_3
#define E3_GPIO_Port GPIOD
#define E4_Pin GPIO_PIN_4
#define E4_GPIO_Port GPIOD
#define CAN1_AUX_Pin GPIO_PIN_10
#define CAN1_AUX_GPIO_Port GPIOC

// ADC configuration

extern ADC_ChannelConfTypeDef ADC_2S_Config;

#define ADC_2S_FACTOR 0.12594f 
#define VOLTAGE_2S_FLAT 6.5f // 3.25*2
#define VOLTAGE_2S_MAX 8.2f // 4.1*2

// Sensor ID config

#define SID_SENSOR_MIPA_A (0b00 << 6) | (0b000) << 3 | (0b111) // MIPA A ID at 1000Hz
#define SID_SENSOR_MIPA_B (0b00 << 6) | (0b001) << 3 | (0b111) // MIPA B ID at 1000Hz

#define SID_SENSOR_LC_Thrust (0b11 << 6) | (0b000) << 3 | (0b100) // LC Thrust ID at 100Hz
#define SID_SENSOR_LC_N2O_A (0b11 << 6) | (0b001) << 3 | (0b010) // LC N2O A ID at 20Hz
#define SID_SENSOR_LC_N2O_B (0b11 << 6) | (0b010) << 3 | (0b010) // LC N2O B ID at 20Hz

#define SID_SENSOR_PT_A (0b10 << 6) | (0b000) << 3 | (0b001) // PT A ID at 10Hz
#define SID_SENSOR_PT_B (0b10 << 6) | (0b001) << 3 | (0b001) // PT B ID at 10Hz
#define SID_SENSOR_PT_C (0b10 << 6) | (0b010) << 3 | (0b001) // PT C ID at 10Hz

#define SID_SENSOR_THERMO_A (0b01 << 6) | (0b000) << 3 | (0b001) // Thermo A ID at 10Hz
#define SID_SENSOR_THERMO_B (0b01 << 6) | (0b001) << 3 | (0b001) // Thermo B ID at 10Hz
#define SID_SENSOR_THERMO_C (0b01 << 6) | (0b010) << 3 | (0b001) // Thermo C ID at 10Hz
#define SID_SENSOR_CJT      (0b01 << 6) | (0b011) << 3 | (0b000) // CJT ID at 2Hz

#define SID_SENSOR_TEST     (0b00 << 6) | (0b111) << 3 | (0b111) // Test Sensor ID at 2Hz (7)

// Sensor Buffer Size Config
// MIPA is fixed as part of ADC scheduling to 28

#define BUFF_SIZE_LC_Thrust 29
#define BUFF_SIZE_LC_N2O_A 20
#define BUFF_SIZE_LC_N2O_B 20

#define BUFF_SIZE_THERMO_A 20
#define BUFF_SIZE_THERMO_B 20
#define BUFF_SIZE_THERMO_C 20
#define BUFF_SIZE_CJT      4

#define BUFF_SIZE_PT 10

#endif // CONFIG_H