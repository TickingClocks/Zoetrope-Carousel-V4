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
#include "stm32c0xx_hal.h"

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
#define IR_input_Pin GPIO_PIN_9
#define IR_input_GPIO_Port GPIOB
#define neopixel_Pin GPIO_PIN_2
#define neopixel_GPIO_Port GPIOA
#define led1_Pin GPIO_PIN_4
#define led1_GPIO_Port GPIOA
#define led2_Pin GPIO_PIN_5
#define led2_GPIO_Port GPIOA
#define led3_Pin GPIO_PIN_6
#define led3_GPIO_Port GPIOA
#define led4_Pin GPIO_PIN_7
#define led4_GPIO_Port GPIOA
#define led5_Pin GPIO_PIN_0
#define led5_GPIO_Port GPIOB
#define led6_Pin GPIO_PIN_1
#define led6_GPIO_Port GPIOB
#define led7_Pin GPIO_PIN_2
#define led7_GPIO_Port GPIOB
#define redLED_Pin GPIO_PIN_9
#define redLED_GPIO_Port GPIOA
#define ledBLUE_Pin GPIO_PIN_10
#define ledBLUE_GPIO_Port GPIOA
#define ledGREEN_Pin GPIO_PIN_11
#define ledGREEN_GPIO_Port GPIOA
#define strobe_Pin GPIO_PIN_12
#define strobe_GPIO_Port GPIOA
#define led8_Pin GPIO_PIN_15
#define led8_GPIO_Port GPIOA
#define led9_Pin GPIO_PIN_3
#define led9_GPIO_Port GPIOB
#define led10_Pin GPIO_PIN_4
#define led10_GPIO_Port GPIOB
#define led11_Pin GPIO_PIN_5
#define led11_GPIO_Port GPIOB
#define led12_Pin GPIO_PIN_6
#define led12_GPIO_Port GPIOB
#define led13_Pin GPIO_PIN_7
#define led13_GPIO_Port GPIOB
#define led14_Pin GPIO_PIN_8
#define led14_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
