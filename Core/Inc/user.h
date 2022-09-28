/*
 * user.h
 *
 *  Created on: 28 сент. 2022 г.
 *      Author: come_ill_foo
 */

#ifndef INC_USER_H_
#define INC_USER_H_

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

struct request {
	enum command type;
	union {
		struct mode_args      as_mode;
		struct timeout_args   as_timeout;
		struct interrupt_args as_interrupt;
	};
};

struct context {
	struct settings*  setup;
	struct stoplight* light;
};

#define RESPONSE_LENGTH 256

typedef void (*executor)(struct context, struct request, char[RESPONSE_LENGTH]);


struct request read_command(struct settings);

void user_uart_handler(void);

#endif /* INC_USER_H_ */
