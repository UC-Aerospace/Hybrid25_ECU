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
#define EMATCH2_FIRE_Pin GPIO_PIN_11
#define EMATCH2_FIRE_GPIO_Port GPIOC
#define EMATCH1_FIRE_Pin GPIO_PIN_12
#define EMATCH1_FIRE_GPIO_Port GPIOC
#define ARM_HS_Pin GPIO_PIN_13
#define ARM_HS_GPIO_Port GPIOC
#define SPI1_CSS_Pin GPIO_PIN_4
#define SPI1_CSS_GPIO_Port GPIOA
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
#define LED_STATUS_Pin GPIO_PIN_0
#define LED_STATUS_GPIO_Port GPIOD
#define LED_IND_GREEN_Pin GPIO_PIN_1
#define LED_IND_GREEN_GPIO_Port GPIOD
#define LED_ERROR_Pin GPIO_PIN_2
#define LED_ERROR_GPIO_Port GPIOD
#define HORN_IND_Pin GPIO_PIN_3
#define HORN_IND_GPIO_Port GPIOD
#define E4_Pin GPIO_PIN_4
#define E4_GPIO_Port GPIOD
#define OX_FIRE_Pin GPIO_PIN_10
#define OX_FIRE_GPIO_Port GPIOC

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
