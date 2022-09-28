/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
enum modes {
	M_BTN_PROCESSED = 0,
	M_BTN_IGNORED
};

enum interrupts {
	INT_POLLING = 0,
	INT_INTERRUPT
};

struct settings {
	enum modes mode;
	uint32_t timeout;
	enum interrupts is_interrupts_on;
};

enum light_type {
	LT_NONBLINKING = 0,
	LT_BLINKING
};

enum states_type {
	ST_RED = 0,
	ST_GREEN = 1,
	ST_WARNING = 2,
	ST_YELLOW = 3
};

enum light_colours {
	LC_RED = 0,
	LC_YELLOW,
	LC_GREEN
};


struct stoplight_state {
	enum light_colours colour;
	enum light_type type;
	uint32_t period_factor;
};


#define STATES_NR 4

struct stoplight {
	struct stoplight_state states[STATES_NR];
	enum states_type current;
	int should_restore;
	int is_green_requested;
};

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
#define nBTN_Pin GPIO_PIN_15
#define nBTN_GPIO_Port GPIOC
#define GLC_Pin GPIO_PIN_13
#define GLC_GPIO_Port GPIOD
#define YARC_Pin GPIO_PIN_14
#define YARC_GPIO_Port GPIOD
#define YCRA_Pin GPIO_PIN_15
#define YCRA_GPIO_Port GPIOD
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
