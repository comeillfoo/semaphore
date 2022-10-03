/*
 * queue.c
 *
 *  Created on: 28 сент. 2022 г.
 *      Author: come_ill_foo
 */
#include "queue.h"

size_t queue_read(struct fifo_queue* q, uint8_t* dest, size_t size) {
	size_t j = 0;
	const size_t available = q->counter;
	int i = q->data_p - q->counter;
	while (j < size && j < available && q->counter > 0) {

		if (q->data[i] == '\r')
			q->line_feeds--;

		dest[j] = q->data[i];
		q->counter--;
		i = (i + 1) % MAX_QUEUE_SIZE;
		j++;
	}

	if (q->line_feeds < 0)
		q->line_feeds = 0;

	if (q->counter < 0)
		q->counter = 0;

	return j;
}

extern struct fifo_queue commands_queue;

void queue_write(struct fifo_queue* q, uint8_t* src, size_t size) {
	size_t limit = (size + q->data_p) % MAX_QUEUE_SIZE;
	size_t i = 0;
	// TODO: fix backspace bug on queue level
	while (q->data_p != limit && i < size) {
		if (src[i] == 127 && q == &commands_queue) {
			char last_symbol;
			queue_read(q, (uint8_t*) &last_symbol, 1);
			q->data_p--;
		} else {
			q->data[q->data_p] = src[i];
			q->data_p = (q->data_p + 1) % MAX_QUEUE_SIZE;
			q->counter++;
		}

		if (src[i] == '\r') {
			q->line_feeds++;
			char lf = '\n';
			queue_write(q, (uint8_t*) &lf, 1);
		}
		i++;
	}
}


int queue_is_empty(struct fifo_queue* q) {
	return q->counter <= 0;
}

void queue_clear(struct fifo_queue* q) {
	q->counter = 0;
}
