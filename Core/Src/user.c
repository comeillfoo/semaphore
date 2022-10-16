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

enum command {
	C_GET_HELP = 0,
	C_SET_MODE,
	C_SET_TIMEOUT,
	C_SET_INTERRUPTS,
	C_NOP,
	C_UNDEFINED
};

struct mode_args {
	enum command type;
	enum modes mode;
};

struct timeout_args {
	enum command type;
	uint32_t timeout;
};

struct interrupt_args {
	enum command type;
	enum interrupts polling_or_interrupt;
};

#define RESPONSE_LENGTH 256

struct undefined_args {
	size_t length;
	char response[RESPONSE_LENGTH];
};

struct request {
	enum command type;
	union {
		struct mode_args      as_mode;
		struct timeout_args   as_timeout;
		struct interrupt_args as_interrupt;
		struct undefined_args as_undefined;
	};
};

struct context {
	struct settings*  setup;
	struct stoplight* light;
};


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

typedef void (*executor)(struct context, struct request, char[RESPONSE_LENGTH]);

static void get_help_execute(struct context ctx, struct request request, char response[RESPONSE_LENGTH]) {
	snprintf(response, RESPONSE_LENGTH, "%s mode %d timeout %lu %c\r\n",
			state_names[ctx.light->current],
			ctx.setup->mode + 1,
			ctx.setup->timeout,
			interrupts_names[ctx.setup->is_interrupts_on]);
}

#define RESPONSE_OK "OK\r\n"

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
			HAL_UART_Abort_IT(&huart6);
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

#define INIT_QUEUE { .counter = 0, .data_p = 0, .line_feeds = 0 }

struct fifo_queue commands_queue = INIT_QUEUE;


static struct request read_command(struct settings setup) {
	char data[RESPONSE_LENGTH];
	const size_t bytes = queue_read(&commands_queue, (uint8_t*) data, RESPONSE_LENGTH);
	if (bytes == 0)
		return (struct request) { .type = C_NOP };

	if (data[0] == '?')
		return (struct request) { .type = C_GET_HELP };

	int ret = 0;
	int mode = 0;
	ret = sscanf(data, "set mode %d", &mode);
	if (ret == 1) {
		if (mode != 1 && mode != 2)
			return (struct request) { .type = C_UNDEFINED };

		return (struct request) {
			.type = C_SET_MODE,
			.as_mode = { C_SET_MODE, (mode == 1)? M_BTN_PROCESSED : M_BTN_IGNORED }
		};
	}

	ret = 0;
	uint32_t secs = 0;
	ret = sscanf(data, "set timeout %lu", &secs);
	if (ret == 1) {
		if (secs == 0) return (struct request) { .type = C_UNDEFINED };

		return (struct request) {
			.type = C_SET_TIMEOUT,
			.as_timeout = { C_SET_TIMEOUT, secs * 1000 }
		};
	}

	// remove newline symbols
	char* ptr = data;
	while (*ptr != '\n' && *ptr != '\r' && *ptr != '\t' && *ptr != 0)
		ptr++;
	*ptr = 0;

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

#define POLLING_RECEIVE_TIMEOUT_PER_CHAR (10)

extern struct settings global_settings;
extern struct stoplight global_stoplight;

static struct fifo_queue from_user_queue = INIT_QUEUE;
static struct fifo_queue to_user_queue = INIT_QUEUE;

#undef INIT_QUEUE

static char last_symbol = 0;

static char echo_buffer_from[RESPONSE_LENGTH];
static char echo_buffer_to[RESPONSE_LENGTH];

static HAL_StatusTypeDef receive_status = HAL_OK;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	queue_write(&from_user_queue, (uint8_t*) &last_symbol, 1);
}

uint32_t user_uart_handler(void) {
	const uint32_t start_time = HAL_GetTick();

	// init the receiving routine
	if (!global_settings.is_interrupts_on) {
		receive_status = HAL_UART_Receive(&huart6, (uint8_t*) &last_symbol, 1, POLLING_RECEIVE_TIMEOUT_PER_CHAR);
		if (receive_status == HAL_OK)
			queue_write(&from_user_queue, (uint8_t*) &last_symbol, 1);
	} else receive_status = HAL_UART_Receive_IT(&huart6, (uint8_t*) &last_symbol, 1);


	if (queue_is_empty(&from_user_queue))
		return HAL_GetTick() - start_time;

	const size_t echo_buffer_from_length = queue_read(&from_user_queue, (uint8_t*) echo_buffer_from, RESPONSE_LENGTH);

	queue_write(&commands_queue, (uint8_t*) echo_buffer_from, echo_buffer_from_length);
	queue_write(&to_user_queue, (uint8_t*) echo_buffer_from, echo_buffer_from_length);

	// if there is a command execute command
	if (commands_queue.line_feeds > 0) {
		char response[RESPONSE_LENGTH];
		const struct request rq = read_command(global_settings);

		if (rq.type != C_NOP && rq.type != C_UNDEFINED) {
			executors[rq.type]((struct context) { &global_settings, &global_stoplight },
				rq, response);
			const size_t response_length = strlen(response);
			queue_write(&to_user_queue, (uint8_t*) response, response_length);
		}
	}

	// if there are no symbols on output returns
	if (queue_is_empty(&to_user_queue))
		return HAL_GetTick() - start_time;

	if (global_settings.is_interrupts_on)
		while (HAL_UART_GetState(&huart6) == HAL_UART_STATE_BUSY_TX);

	const size_t echo_buffer_to_length = queue_read(&to_user_queue, (uint8_t*) echo_buffer_to, RESPONSE_LENGTH);

#if 0
	char msg[RESPONSE_LENGTH];
	snprintf(msg, RESPONSE_LENGTH, "length: %u\r\n", echo_buffer_to_length);
	const size_t sz = strlen(msg);
	HAL_UART_Transmit(&huart6, (uint8_t*) msg, sz, sz * POLLING_RECEIVE_TIMEOUT_PER_CHAR);
#endif

	if (global_settings.is_interrupts_on)
		HAL_UART_Transmit_IT(&huart6, (uint8_t*) echo_buffer_to, echo_buffer_to_length);
	else
		HAL_UART_Transmit(&huart6, (uint8_t*) echo_buffer_to, echo_buffer_to_length,
			echo_buffer_to_length * POLLING_RECEIVE_TIMEOUT_PER_CHAR);


#if 0
	if (ret == HAL_TIMEOUT) {
		const char timeout_msg[] = "TO\r\n";
		const size_t timeout_msg_length = strlen(timeout_msg);
		HAL_UART_Transmit(&huart6, (uint8_t*) timeout_msg, timeout_msg_length,
				timeout_msg_length * POLLING_RECEIVE_TIMEOUT_PER_CHAR);
	}
#endif

	return HAL_GetTick() - start_time;
}

#undef POLLING_RECEIVE_TIMEOUT

#undef RESPONSE_LENGTH
