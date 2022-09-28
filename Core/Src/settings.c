/*
 * settings.c
 *
 *  Created on: 29 сент. 2022 г.
 *      Author: come_ill_foo
 */
#include "settings.h"

#define BASE_LIGHT_PERIOD (1000)

struct settings global_settings = {
	.mode = M_BTN_PROCESSED,
	.timeout = BASE_LIGHT_PERIOD,
	.is_interrupts_on = INT_POLLING
};

#undef BASE_LIGHT_PERIOD
