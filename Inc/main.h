/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
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
#include "stm32l4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
//#define min(a,b) 	((a)<(b) ? (a):(b))
//#define max(a,b) 	((a)>(b) ? (a):(b))
//#define abs(x) 		((x)>0 ? (x):-(x))

//#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
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
#define TP1_Pin GPIO_PIN_5
#define TP1_GPIO_Port GPIOA
#define SXRX_Pin GPIO_PIN_6
#define SXRX_GPIO_Port GPIOA
#define SXTX_Pin GPIO_PIN_7
#define SXTX_GPIO_Port GPIOA
#define GPIO_PPS_EXTI0_Pin GPIO_PIN_0
#define GPIO_PPS_EXTI0_GPIO_Port GPIOB
#define GPIO_PPS_EXTI0_EXTI_IRQn EXTI0_IRQn
#define SXDIO0_EXTI8_Pin GPIO_PIN_8
#define SXDIO0_EXTI8_GPIO_Port GPIOA
#define SXDIO0_EXTI8_EXTI_IRQn EXTI9_5_IRQn
#define SXSEL_Pin GPIO_PIN_9
#define SXSEL_GPIO_Port GPIOA
#define SXRESET_Pin GPIO_PIN_10
#define SXRESET_GPIO_Port GPIOA
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
