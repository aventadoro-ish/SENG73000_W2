/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32f3xx_hal.h"

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
#define Blue_Button_Pin GPIO_PIN_13
#define Blue_Button_GPIO_Port GPIOC
#define Blue_Button_EXTI_IRQn EXTI15_10_IRQn
#define PB1_LED_Pin GPIO_PIN_4
#define PB1_LED_GPIO_Port GPIOA
#define Green_LED_Pin GPIO_PIN_5
#define Green_LED_GPIO_Port GPIOA
#define PB2_LED_Pin GPIO_PIN_6
#define PB2_LED_GPIO_Port GPIOA
#define PB3_LED_Pin GPIO_PIN_7
#define PB3_LED_GPIO_Port GPIOA
#define F1_Indicator_LED_Pin GPIO_PIN_5
#define F1_Indicator_LED_GPIO_Port GPIOC
#define F2_Indicator_LED_Pin GPIO_PIN_0
#define F2_Indicator_LED_GPIO_Port GPIOB
#define F3_Indicator_LED_Pin GPIO_PIN_1
#define F3_Indicator_LED_GPIO_Port GPIOB
#define PushButton_1_Pin GPIO_PIN_12
#define PushButton_1_GPIO_Port GPIOB
#define PushButton_1_EXTI_IRQn EXTI15_10_IRQn
#define PushButton_2_Pin GPIO_PIN_14
#define PushButton_2_GPIO_Port GPIOB
#define PushButton_2_EXTI_IRQn EXTI15_10_IRQn
#define PushButton_3_Pin GPIO_PIN_15
#define PushButton_3_GPIO_Port GPIOB
#define PushButton_3_EXTI_IRQn EXTI15_10_IRQn

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
