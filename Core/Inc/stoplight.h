/*
 * stoplight.h
 *
 *  Created on: 29 сент. 2022 г.
 *      Author: come_ill_foo
 */

#ifndef INC_STOPLIGHT_H_
#define INC_STOPLIGHT_H_

#include <stdint.h>

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

struct stoplight stoplight_next_state(struct stoplight);

struct stoplight stoplight_reset(struct stoplight);

#endif /* INC_STOPLIGHT_H_ */
