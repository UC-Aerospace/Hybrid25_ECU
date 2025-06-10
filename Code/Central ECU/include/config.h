#ifndef CONFIG_H
#define CONFIG_H

#include "stm32g0xx_hal.h"

// Pinout configuration

#define EMATCH2_FIRE_Pin GPIO_PIN_11
#define EMATCH2_FIRE_GPIO_Port GPIOC
#define EMATCH1_FIRE_Pin GPIO_PIN_12
#define EMATCH1_FIRE_GPIO_Port GPIOC
#define ARM_HS_Pin GPIO_PIN_13
#define ARM_HS_GPIO_Port GPIOC
#define CAN2_AUX_Pin GPIO_PIN_12
#define CAN2_AUX_GPIO_Port GPIOB
#define CAN1_AUX_Pin GPIO_PIN_13
#define CAN1_AUX_GPIO_Port GPIOB
#define PGOOD_HS_Pin GPIO_PIN_14
#define PGOOD_HS_GPIO_Port GPIOB
#define IMON_HS_Pin GPIO_PIN_15
#define IMON_HS_GPIO_Port GPIOB
#define EMATCH2_CONT_Pin GPIO_PIN_6
#define EMATCH2_CONT_GPIO_Port GPIOC
#define EMATCH1_CONT_Pin GPIO_PIN_7
#define EMATCH1_CONT_GPIO_Port GPIOC
#define OX_LOWSIDE_SENSE_Pin GPIO_PIN_8
#define OX_LOWSIDE_SENSE_GPIO_Port GPIOC
#define SENSE_12V_Pin GPIO_PIN_9
#define SENSE_12V_GPIO_Port GPIOC
#define LED_Pin GPIO_PIN_0
#define LED_GPIO_Port GPIOD
#define E1_Pin GPIO_PIN_1
#define E1_GPIO_Port GPIOD
#define E2_Pin GPIO_PIN_2
#define E2_GPIO_Port GPIOD
#define E3_Pin GPIO_PIN_3
#define E3_GPIO_Port GPIOD
#define E4_Pin GPIO_PIN_4
#define E4_GPIO_Port GPIOD
#define OX_FIRE_Pin GPIO_PIN_10
#define OX_FIRE_GPIO_Port GPIOC

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