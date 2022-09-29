/*
 * queue.c
 *
 *  Created on: 28 сент. 2022 г.
 *      Author: come_ill_foo
 */
#include "queue.h"


void queue_write(struct fifo_queue* q, uint8_t* src, size_t size) {
	const size_t limit = (size + q->data_p) % MAX_QUEUE_SIZE;
	size_t i = q->data_p;
	size_t j = 0;
	while (i != limit && j != size) {
		if (src[j] == '\n')
			q->line_feeds++;
		q->data[i] = src[j];
		j++;
		i = (i + 1) % MAX_QUEUE_SIZE;
		q->counter++;
	}
}

size_t queue_read(struct fifo_queue* q, uint8_t* dest, size_t size) {
	size_t read = 0;
	const size_t available = q->counter;
	while (read < size && read < available && q->counter != 0) {
		if (q->data[q->counter] == '\n')
			q->line_feeds--;

		dest[read] = q->data[q->data_p - q->counter + 1];

		q->counter--;
		read++;
	}
	return read;
}

int queue_is_empty(struct fifo_queue* q) {
	return q->counter == 0;
}

void queue_clear(struct fifo_queue* q) {
	q->counter = 0;
}
