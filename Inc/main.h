/**
  ******************************************************************************
  * File Name          : main.h
  * Description        : This file contains the common defines of the application
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2017 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H
  /* Includes ------------------------------------------------------------------*/

/* USER CODE BEGIN Includes */
#define min(a,b) 	((a)<(b) ? (a):(b))
#define max(a,b) 	((a)>(b) ? (a):(b))
//#define abs(x) 		((x)>0 ? (x):-(x))
/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/

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
#define SDDIO0_EXTI8_EXTI_IRQn EXTI9_5_IRQn
#define SXSEL_Pin GPIO_PIN_9
#define SXSEL_GPIO_Port GPIOA
#define SXRESET_Pin GPIO_PIN_10
#define SXRESET_GPIO_Port GPIOA
#define GPIO_VBUS_Pin GPIO_PIN_15
#define GPIO_VBUS_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

#define SXDIO0_EXTI8_nIRQ	EXTI9_5_IRQn
#define GPIO_PPS_EXTI0_nIRQ	EXTI0_IRQn

/* USER CODE END Private defines */

void _Error_Handler(char *, int);

#define Error_Handler() _Error_Handler(__FILE__, __LINE__)

/**
  * @}
  */ 

/**
  * @}
*/ 

#endif /* __MAIN_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
