/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
extern SPI_HandleTypeDef hspi2;
extern DMA_HandleTypeDef hdma_spi2_rx;
extern DMA_HandleTypeDef hdma_spi2_tx;
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
#define PH0_OSC_IN_Pin GPIO_PIN_0
#define PH0_OSC_IN_GPIO_Port GPIOH
#define PH1_OSC_OUT_Pin GPIO_PIN_1
#define PH1_OSC_OUT_GPIO_Port GPIOH
#define BTN_Pin GPIO_PIN_0
#define BTN_GPIO_Port GPIOA
#define ETH_CS_Pin GPIO_PIN_4
#define ETH_CS_GPIO_Port GPIOC
#define ETH_INT_Pin GPIO_PIN_5
#define ETH_INT_GPIO_Port GPIOC
#define ETH_INT_EXTI_IRQn EXTI9_5_IRQn
#define ETH_RST_Pin GPIO_PIN_0
#define ETH_RST_GPIO_Port GPIOB
#define LD_GREEN_Pin GPIO_PIN_12
#define LD_GREEN_GPIO_Port GPIOD
#define LD_ORANGE_Pin GPIO_PIN_13
#define LD_ORANGE_GPIO_Port GPIOD
#define LD_RED_Pin GPIO_PIN_14
#define LD_RED_GPIO_Port GPIOD
#define LD_BLUE_Pin GPIO_PIN_15
#define LD_BLUE_GPIO_Port GPIOD

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
