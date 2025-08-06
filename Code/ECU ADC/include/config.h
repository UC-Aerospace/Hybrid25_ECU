#ifndef CONFIG_H
#define CONFIG_H

#include "stm32g0xx_hal.h"

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
#define LED2_Pin GPIO_PIN_9
#define LED2_GPIO_Port GPIOC
#define LED1_Pin GPIO_PIN_0
#define LED1_GPIO_Port GPIOD
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

#define ADC_2S_FACTOR 2.0298f 
#define VOLTAGE_2S_FLAT 6.5f // 3.25*2
#define VOLTAGE_2S_MAX 8.2f // 4.1*2

#endif // CONFIG_H