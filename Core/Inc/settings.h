/*
 * settings.h
 *
 *  Created on: 29 сент. 2022 г.
 *      Author: come_ill_foo
 */

#ifndef INC_SETTINGS_H_
#define INC_SETTINGS_H_

#include <stdint.h>

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

#endif /* INC_SETTINGS_H_ */
