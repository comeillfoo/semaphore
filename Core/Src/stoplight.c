/*
 * stoplight.c
 *
 *  Created on: 29 сент. 2022 г.
 *      Author: come_ill_foo
 */

#include "stoplight.h"
#include "user.h"
#include "settings.h"
#include "gpio.h"

extern struct settings global_settings;

struct stoplight global_stoplight = {
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

#define BTN_DEBOUNCE_TIMEOUT (100)

static struct stoplight poll_events(struct stoplight sl, uint32_t blink_period) {
	uint32_t passed_time = 0;
	uint32_t timeout = sl.states[sl.current].period_factor * global_settings.timeout;
	while (passed_time * blink_period < timeout) {

		// TODO: handle uart here
		passed_time += user_uart_handler();

		// process button in polling mode
		if (global_settings.mode == M_BTN_PROCESSED) {
			// because nBTN signal is inverted
			sl.is_green_requested = !btn_set_debounce(GPIOC, nBTN_Pin, BTN_DEBOUNCE_TIMEOUT);
			passed_time += BTN_DEBOUNCE_TIMEOUT;
			switch (sl.current) {
				case ST_RED:     sl = short_period(sl); break;
				case ST_GREEN:   sl = restore_period(sl); break;
				case ST_WARNING: sl = restore_period(sl); break;
				case ST_YELLOW:  sl = short_period(sl); break;
				default: break;
			}
		}

		timeout = sl.states[sl.current].period_factor * global_settings.timeout;
	}

	return sl;
}

#undef PASSED_TIME

#undef BTN_DEBOUNCE_TIMEOUT

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

#define BLINKS_NR 3

static struct stoplight light_state(struct stoplight sl) {
	const size_t limit        = (sl.states[sl.current].type == LT_NONBLINKING)? 1 : (BLINKS_NR);
	const size_t blink_period = (sl.states[sl.current].type == LT_NONBLINKING)? limit : (2 * limit);

	for (size_t i = 0; i < limit; ++i) {
		turn_on_light(sl.states[sl.current].colour);
		sl = poll_events(sl, blink_period);

		turn_off_light(sl.states[sl.current].colour);
		if (sl.states[sl.current].type != LT_NONBLINKING)
			sl = poll_events(sl, blink_period);
	}
	return sl;
}

#undef BLINKS_NR

struct stoplight stoplight_next_state(struct stoplight sl) {
	sl = light_state(sl);

	sl.current = (sl.current + 1) % (STATES_NR);
	return sl;
}

struct stoplight stoplight_reset(struct stoplight sl) {
	sl.current = ST_RED;
	sl.should_restore = 0;
	sl.is_green_requested = 0;
	return sl;
}
