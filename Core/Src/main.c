/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define COMMON_DELAY (250)

#define BASE_LIGHT_PERIOD (1000)
#define BLINKS_NR 3

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static struct settings global_settings = {
	.mode = M_BTN_PROCESSED,
	.timeout = BASE_LIGHT_PERIOD,
	.is_interrupts_on = INT_POLLING
};

static struct stoplight global_stoplight = {
	{
		{LC_RED,    LT_NONBLINKING, 4},
		{LC_GREEN,  LT_NONBLINKING, 1},
		{LC_GREEN,  LT_BLINKING,    1},
		{LC_YELLOW, LT_NONBLINKING, 1}
	},
	ST_RED,
	0,
	0
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static struct stoplight stoplight_next_state(struct stoplight);

static struct stoplight stoplight_reset(struct stoplight);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */
  global_stoplight = stoplight_reset(global_stoplight);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  global_stoplight = stoplight_next_state(global_stoplight);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 216;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
static struct stoplight short_period(struct stoplight sl) {
	if (!sl.should_restore && sl.is_green_requested) {
		sl.should_restore = 1; // true
		sl.is_green_requested = 0;
		sl.states[ST_RED].period_factor = sl.states[ST_RED].period_factor / 4;
	}
	return sl;
}

static struct stoplight restore_period(struct stoplight sl) {
	if (sl.should_restore) {
		sl.should_restore = 0;
		sl.states[ST_RED].period_factor = sl.states[ST_RED].period_factor * 4;
	}
	return sl;
}


static GPIO_PinState btn_set_debounce(GPIO_TypeDef* type_p, uint16_t btn, uint32_t delay) {
	const GPIO_PinState measure_1 = HAL_GPIO_ReadPin(type_p, btn);
	HAL_Delay(delay);
	const GPIO_PinState measure_2 = HAL_GPIO_ReadPin(type_p, btn);
	return (measure_1 && measure_2)? GPIO_PIN_SET : GPIO_PIN_RESET;
}


static struct stoplight wait_for_press(struct stoplight sl, uint32_t blink_period) {
	size_t i = 0;
	const char message[] = "hello stm32\r\n";
	while ((i * (COMMON_DELAY) * blink_period) < sl.states[sl.current].period_factor * global_settings.timeout) {
		// because nBTN signal is inverted
		sl.is_green_requested = !btn_set_debounce(GPIOC, nBTN_Pin, (COMMON_DELAY));
		// HAL_UART_Transmit(&huart6, (uint8_t*) message, sizeof(message), (COMMON_DELAY));
		switch (sl.current) {
			case ST_RED:     sl = short_period(sl); break;
			case ST_GREEN:   sl = restore_period(sl); break;
			case ST_WARNING: sl = restore_period(sl); break;
			case ST_YELLOW:  sl = short_period(sl); break;
			default: break;
		}

		++i;
	}

	return sl;
}

static void turn_on_light(enum light_colours colour) {
	switch (colour) {
		case LC_RED:
			HAL_GPIO_WritePin(GPIOD, YCRA_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOD, YARC_Pin, GPIO_PIN_RESET);
			break;
		case LC_YELLOW:
			HAL_GPIO_WritePin(GPIOD, YCRA_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOD, YARC_Pin, GPIO_PIN_SET);
			break;
		case LC_GREEN:
			HAL_GPIO_WritePin(GPIOD, GLC_Pin, GPIO_PIN_SET);
			break;
	}
}

static void turn_off_light(enum light_colours colour) {
	switch (colour) {
		case LC_RED:
		case LC_YELLOW:
			HAL_GPIO_WritePin(GPIOD, YCRA_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(GPIOD, YARC_Pin, GPIO_PIN_RESET);
			break;
		case LC_GREEN:
			HAL_GPIO_WritePin(GPIOD, GLC_Pin, GPIO_PIN_RESET);
			break;
	}
}

static struct stoplight light_state(struct stoplight sl) {
	const size_t limit        = (sl.states[sl.current].type == LT_NONBLINKING)? 1 : (BLINKS_NR);
	const size_t blink_period = (sl.states[sl.current].type == LT_NONBLINKING)? limit : (2 * limit);

	for (size_t i = 0; i < limit; ++i) {
		turn_on_light(sl.states[sl.current].colour);
		sl = wait_for_press(sl, blink_period);

		turn_off_light(sl.states[sl.current].colour);
		if (sl.states[sl.current].type != LT_NONBLINKING)
			sl = wait_for_press(sl, blink_period);
	}
	return sl;
}


static struct stoplight stoplight_next_state(struct stoplight sl) {
	sl = light_state(sl);

	sl.current = (sl.current + 1) % (STATES_NR);
	return sl;
}

static struct stoplight stoplight_reset(struct stoplight sl) {
	sl.current = ST_RED;
	sl.should_restore = 0;
	sl.is_green_requested = 0;
	return sl;
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
