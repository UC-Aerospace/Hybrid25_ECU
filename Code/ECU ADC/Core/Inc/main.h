/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define CJT_A0_Pin GPIO_PIN_0
#define CJT_A0_GPIO_Port GPIOA
#define MIPAA_A1_Pin GPIO_PIN_1
#define MIPAA_A1_GPIO_Port GPIOA
#define MIPAB_A2_Pin GPIO_PIN_2
#define MIPAB_A2_GPIO_Port GPIOA
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

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
