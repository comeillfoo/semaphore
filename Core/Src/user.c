/*
 * user.c
 *
 *  Created on: 28 сент. 2022 г.
 *      Author: come_ill_foo
 */
#include <stdio.h>
#include <string.h>
#include "user.h"
#include "usart.h"
#include "queue.h"
#include "stoplight.h"
#include "settings.h"


static const char* state_names[] = {
	[ST_RED]     = "red",
	[ST_GREEN]   = "green",
	[ST_WARNING] = "blinking green",
	[ST_YELLOW]  = "yellow"
};

static const char interrupts_names[] = {
	[INT_POLLING]   = 'P',
	[INT_INTERRUPT] = 'I'
};

static void get_help_execute(struct context ctx, struct request request, char response[RESPONSE_LENGTH]) {
	snprintf(response, RESPONSE_LENGTH, "%s mode %d timeout %lu %c\n",
			state_names[ctx.light->current],
			ctx.setup->mode + 1,
			ctx.setup->timeout,
			interrupts_names[ctx.setup->is_interrupts_on]);
}

#define RESPONSE_OK "OK\n"

static void set_mode_execute(struct context ctx, struct request request, char response[RESPONSE_LENGTH]) {
	ctx.setup->mode = request.as_mode.mode;
	snprintf(response, RESPONSE_LENGTH, RESPONSE_OK);
}


static void set_timeout_execute(struct context ctx, struct request request, char response[RESPONSE_LENGTH]) {
	ctx.setup->timeout = request.as_timeout.timeout;
	snprintf(response, RESPONSE_LENGTH, RESPONSE_OK);
}


static void set_interrupts_execute(struct context ctx, struct request request, char response[RESPONSE_LENGTH]) {
	uint32_t pmask = __get_PRIMASK();
	__disable_irq();

	ctx.setup->is_interrupts_on = request.as_interrupt.polling_or_interrupt;
	switch (ctx.setup->is_interrupts_on) {
		case INT_POLLING:
			HAL_NVIC_DisableIRQ(USART6_IRQn);
			break;
		case INT_INTERRUPT:
			HAL_NVIC_SetPriority(USART6_IRQn, 0, 0);
			HAL_NVIC_EnableIRQ(USART6_IRQn);
			break;
	}
	snprintf(response, RESPONSE_LENGTH, RESPONSE_OK);

	__set_PRIMASK(pmask);
}

#undef RESPONSE_OK

static executor executors[] = {
	[C_GET_HELP]       = get_help_execute,
	[C_SET_MODE]       = set_mode_execute,
	[C_SET_TIMEOUT]    = set_timeout_execute,
	[C_SET_INTERRUPTS] = set_interrupts_execute
};

static struct fifo_queue buffer;


struct request read_command(struct settings setup) {
	char data[RESPONSE_LENGTH];
	const size_t bytes = queue_read(&buffer, (uint8_t*) data, RESPONSE_LENGTH);
	if (bytes == 0)
		return (struct request) { .type = C_NOP };

	if (data[0] == '?')
		return (struct request) { .type = C_GET_HELP };

	int ret = 0;
	int mode = 0;
	ret = sscanf(data, "set mode %d", &mode);
	if (ret == 1)
		return (struct request) {
			.type = C_SET_MODE,
			.as_mode = { C_SET_MODE, (mode == 1)? M_BTN_PROCESSED : M_BTN_IGNORED }
		};

	ret = 0;
	uint32_t secs = 0;
	ret = sscanf(data, "set timeout %lu", &secs);
	if (ret == 1)
		return (struct request) {
			.type = C_SET_TIMEOUT,
			.as_timeout = { C_SET_TIMEOUT, secs * 1000 }
		};

	ret = strcmp("set interrupts on", data);
	if (ret == 0)
		return (struct request) {
			.type = C_SET_INTERRUPTS,
			.as_interrupt = { C_SET_INTERRUPTS, INT_INTERRUPT }
		};

	ret = strcmp("set interrupts off", data);
	if (ret == 0)
		return (struct request) {
			.type = C_SET_INTERRUPTS,
			.as_interrupt = { C_SET_INTERRUPTS, INT_POLLING }
		};

	return (struct request) { .type = C_UNDEFINED };
}
