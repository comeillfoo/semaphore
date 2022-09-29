/*
 * queue.h
 *
 *  Created on: 28 сент. 2022 г.
 *      Author: come_ill_foo
 */

#ifndef INC_QUEUE_H_
#define INC_QUEUE_H_

#include <stdint.h>
#include <stdlib.h>

#define MAX_QUEUE_SIZE 1024

struct fifo_queue {
	size_t data_p;
	int counter;
	uint8_t data[MAX_QUEUE_SIZE];
	int line_feeds;
};

void queue_write(struct fifo_queue*, uint8_t*, size_t);

size_t queue_read(struct fifo_queue*, uint8_t*, size_t);

int queue_is_empty(struct fifo_queue*);

void queue_clear(struct fifo_queue*);

#endif /* INC_QUEUE_H_ */
