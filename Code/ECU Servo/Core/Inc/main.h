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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
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
#define ADC_2S_Pin GPIO_PIN_2
#define ADC_2S_GPIO_Port GPIOB
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

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
