/*
 * circular_buffer.h
 *
 *  Created on: May 13, 2017
 *      Author: sid
 */

#ifndef CIRCULAR_BUFFER_H_
#define CIRCULAR_BUFFER_H_

typedef struct
{
	uint8_t * const buffer;
	int head;
	int tail;
	const int maxLen;
} circBuf_t;

#define CIRC_BUF_DEF(x,y)          \
    uint8_t x##_space[y];     \
    circBuf_t x = {               \
        .buffer = x##_space,      \
        .head = 0,                \
        .tail = 0,                \
        .maxLen = y               \
    }


int circ_buf_push(circBuf_t *c, uint8_t data);
int circ_buf_pop(circBuf_t *c, uint8_t *data);

#endif /* CIRCULAR_BUFFER_H_ */
