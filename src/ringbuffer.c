/* ringbuffer.c
   Copyright (C) 2024 Timo Kokkonen <tjko@iki.fi>

   SPDX-License-Identifier: GPL-3.0-or-later

   This file is part of pico-telnetd Library.

   pico-telnetd Library is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   pico-telnetd Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with pico-telnetd Library. If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "pico_telnetd/ringbuffer.h"

#define PREFIX_LEN 1
#define SUFFIX_LEN 1



int telnet_ringbuffer_init(telnet_ringbuffer_t *rb, uint8_t *buf, size_t size)
{
	if (!rb)
		return -1;

	if (!buf) {
		if (!(rb->buf = calloc(1, size)))
			return -2;
		rb->free_buf = true;
	} else {
		rb->buf = buf;
		rb->free_buf = false;
	}

	rb->size = size;
	rb->free = size;
	rb->head = 0;
	rb->tail = 0;

	return 0;
}


int telnet_ringbuffer_free(telnet_ringbuffer_t *rb)
{
	if (!rb)
		return -1;

	if (rb->free_buf && rb->buf)
		free(rb->buf);

	rb->buf = NULL;
	rb->size = 0;
	rb->free = 0;
	rb->head = 0;
	rb->tail = 0;

	return 0;
}


int telnet_ringbuffer_flush(telnet_ringbuffer_t *rb)
{
	if (!rb)
		return -1;

	rb->head = 0;
	rb->tail = 0;
	rb->free = rb->size;

	return 0;
}

inline size_t telnet_ringbuffer_size(telnet_ringbuffer_t *rb)
{
	return (rb ? rb->size - rb->free : 0);
}


static inline size_t telnet_ringbuffer_offset(telnet_ringbuffer_t *rb, size_t offset, size_t delta, int direction)
{
	size_t o = offset % rb->size;
	size_t d = delta % rb->size;

	if (!rb)
		return 0;
	if (d == 0)
		return o;

	if (direction >= 0) {
		o = (o + d) % rb->size;
	} else {
		if (o < d) {
			o = rb->size - (d - o);
		} else {
			o -= d;
		}
	}

	return o;
}

inline int telnet_ringbuffer_add_char(telnet_ringbuffer_t *rb, uint8_t ch, bool overwrite)
{
	if (!rb)
		return -1;

	if (overwrite && rb->free < 1) {
		rb->head = telnet_ringbuffer_offset(rb, rb->head, 1, 1);
		rb->free += 1;
	}
	if (rb->free < 1)
		return -2;

	rb->buf[rb->tail] = ch;
	rb->tail = telnet_ringbuffer_offset(rb, rb->tail, 1, 1);
	rb->free--;

	return 0;
}

int telnet_ringbuffer_add(telnet_ringbuffer_t *rb, uint8_t *data, size_t len, bool overwrite)
{
	if (!rb || !data)
		return -1;

	if (len == 0)
		return 0;

	if (len > rb->size)
		return -2;

	if (overwrite && rb->free < len) {
		size_t needed = len - rb->free;
		rb->head = telnet_ringbuffer_offset(rb, rb->head, needed, 1);
		rb->free += needed;
	}
	if (rb->free < len)
		return -3;

	size_t new_tail = telnet_ringbuffer_offset(rb, rb->tail, len, 1);

	if (new_tail > rb->tail) {
		memcpy(rb->buf + rb->tail, data, len);
	} else {
		size_t part1 = rb->size - rb->tail;
		size_t part2 = len - part1;
		memcpy(rb->buf + rb->tail, data, part1);
		memcpy(rb->buf, data + part1, part2);
	}

	rb->tail = new_tail;
	rb->free -= len;

	return 0;
}

inline int telnet_ringbuffer_read_char(telnet_ringbuffer_t *rb)
{
	if (!rb)
		return -1;
	if (rb->head == rb->tail)
		return -2;

	int val = rb->buf[rb->head];
	rb->head = telnet_ringbuffer_offset(rb, rb->head, 1, 1);
	rb->free++;

	return val;
}

inline int telnet_ringbuffer_peek_char(telnet_ringbuffer_t *rb, size_t offset)
{
	if (!rb)
		return -1;
	if (rb->head == rb->tail)
		return -2;
	if (offset >= (rb->size - rb->free))
		return -3;

	return rb->buf[telnet_ringbuffer_offset(rb, rb->head, offset, 1)];
}

int telnet_ringbuffer_read(telnet_ringbuffer_t *rb, uint8_t *ptr, size_t size)
{
	if (!rb || size < 1)
		return -1;

	size_t used = rb->size - rb->free;
	if (used < size)
		return -2;

	size_t new_head = telnet_ringbuffer_offset(rb, rb->head, size, 1);

	if (ptr) {
		if (new_head > rb->head) {
			memcpy(ptr, rb->buf + rb->head, size);
		} else {
			size_t part1 = rb->size - rb->head;
			size_t part2 = size - part1;
			memcpy(ptr, rb->buf + rb->head, part1);
			memcpy(ptr + part1, rb->buf, part2);
		}
	}

	rb->head = new_head;
	rb->free += size;

	return 0;
}

size_t telnet_ringbuffer_peek(telnet_ringbuffer_t *rb, uint8_t **ptr, size_t size)
{
	if (!rb || !ptr || size < 1)
		return 0;

	*ptr = NULL;
	size_t used = rb->size - rb->free;
	size_t toread = (size < used ? size : used);
	size_t len = rb->size - rb->head;

	if (used < 1)
		return 0;

	*ptr = rb->buf + rb->head;

	return (len < toread ? len : toread);
}


