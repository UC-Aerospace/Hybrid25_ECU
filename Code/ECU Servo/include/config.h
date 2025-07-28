#ifndef CONFIG_H
#define CONFIG_H

#include "stm32g0xx_hal.h"

// WHOAMI

// Board types: _SERVO, _CENTRAL, _ECU
#define BOARD_TYPE_SERVO
static uint8_t BOARD_ID;

// Pinout configuration

#define PWR_SOURCE_Pin GPIO_PIN_11
#define PWR_SOURCE_GPIO_Port GPIOC
#define POT_A_Pin GPIO_PIN_0
#define POT_A_GPIO_Port GPIOA
#define POT_B_Pin GPIO_PIN_1
#define POT_B_GPIO_Port GPIOA
#define POT_C_Pin GPIO_PIN_2
#define POT_C_GPIO_Port GPIOA
#define POT_D_Pin GPIO_PIN_3
#define POT_D_GPIO_Port GPIOA
#define AUX_A8_Pin GPIO_PIN_0
#define AUX_A8_GPIO_Port GPIOB
#define AUX_A9_Pin GPIO_PIN_1
#define AUX_A9_GPIO_Port GPIOB
#define ADC_2S_Pin GPIO_PIN_2
#define ADC_2S_GPIO_Port GPIOB
#define AUX_A11_Pin GPIO_PIN_10
#define AUX_A11_GPIO_Port GPIOB
#define PWM_D_Pin GPIO_PIN_14
#define PWM_D_GPIO_Port GPIOB
#define AUX_PWM_Pin GPIO_PIN_15
#define AUX_PWM_GPIO_Port GPIOB
#define PWM_C_Pin GPIO_PIN_8
#define PWM_C_GPIO_Port GPIOA
#define PWM_A_Pin GPIO_PIN_6
#define PWM_A_GPIO_Port GPIOC
#define PWM_B_Pin GPIO_PIN_7
#define PWM_B_GPIO_Port GPIOC
#define LED_ERROR_Pin GPIO_PIN_9
#define LED_ERROR_GPIO_Port GPIOC
#define LED_STATUS_Pin GPIO_PIN_0
#define LED_STATUS_GPIO_Port GPIOD
#define E1_Pin GPIO_PIN_1
#define E1_GPIO_Port GPIOD
#define E2_Pin GPIO_PIN_2
#define E2_GPIO_Port GPIOD
#define E3_Pin GPIO_PIN_3
#define E3_GPIO_Port GPIOD
#define E4_Pin GPIO_PIN_4
#define E4_GPIO_Port GPIOD
#define CAN2_AUX_Pin GPIO_PIN_7
#define CAN2_AUX_GPIO_Port GPIOB
#define CAN1_AUX_Pin GPIO_PIN_10
#define CAN1_AUX_GPIO_Port GPIOC

// ADC configuration

extern ADC_ChannelConfTypeDef ADC_6S_Config;
extern ADC_ChannelConfTypeDef ADC_2S_Config;

#define ADC_6S_FACTOR 6.1826f
#define VOLTAGE_6S_FLAT 19.5f // 3.25*6
#define VOLTAGE_6S_MAX 24.6f // 4.1*6

#define ADC_2S_FACTOR 2.0298f 
#define VOLTAGE_2S_FLAT 6.5f // 3.25*2
#define VOLTAGE_2S_MAX 8.2f // 4.1*2

#endif // CONFIG_H