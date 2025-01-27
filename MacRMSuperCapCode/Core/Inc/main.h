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
#include "stm32g4xx_hal.h"

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

void HAL_HRTIM_MspPostInit(HRTIM_HandleTypeDef *hhrtim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define R_Pin GPIO_PIN_1
#define R_GPIO_Port GPIOC
#define G_Pin GPIO_PIN_2
#define G_GPIO_Port GPIOC
#define B_Pin GPIO_PIN_3
#define B_GPIO_Port GPIOC
#define I_CHASSIS_Pin GPIO_PIN_0
#define I_CHASSIS_GPIO_Port GPIOA
#define I_CAP_Pin GPIO_PIN_1
#define I_CAP_GPIO_Port GPIOA
#define V_CHASSIS_SENSE_Pin GPIO_PIN_0
#define V_CHASSIS_SENSE_GPIO_Port GPIOB
#define V_BAT_SENSE_Pin GPIO_PIN_1
#define V_BAT_SENSE_GPIO_Port GPIOB
#define EN_Pin GPIO_PIN_11
#define EN_GPIO_Port GPIOB
#define INL2_Pin GPIO_PIN_12
#define INL2_GPIO_Port GPIOB
#define INR1_Pin GPIO_PIN_14
#define INR1_GPIO_Port GPIOB
#define VCAP_SENSE_Pin GPIO_PIN_15
#define VCAP_SENSE_GPIO_Port GPIOB
#define INR2_Pin GPIO_PIN_8
#define INR2_GPIO_Port GPIOC
#define I_BAT_Pin GPIO_PIN_8
#define I_BAT_GPIO_Port GPIOA
#define INL1_Pin GPIO_PIN_10
#define INL1_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
