/* ringbuffer.h
   Copyright (C) 2024 Timo Kokkonen <tjko@iki.fi>

   SPDX-License-Identifier: GPL-3.0-or-later

   This file is part of BrickPico.

   BrickPico is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   BrickPico is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with BrickPico. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef PICO_TELNETD_RINGBUFFER_H
#define PICO_TELNETD_RINGBUFFER_H 1

#include <stdint.h>
#include <stdbool.h>

typedef struct simple_ringbuffer {
	uint8_t *buf;
	bool free_buf;
	size_t size;
	size_t free;
	size_t head;
	size_t tail;
} simple_ringbuffer_t;


int simple_ringbuffer_init(simple_ringbuffer_t *rb, uint8_t *buf, size_t size);
int simple_ringbuffer_free(simple_ringbuffer_t *rb);
int simple_ringbuffer_flush(simple_ringbuffer_t *rb);
size_t simple_ringbuffer_size(simple_ringbuffer_t *rb);
int simple_ringbuffer_add_char(simple_ringbuffer_t *rb, uint8_t ch, bool overwrite);
int simple_ringbuffer_add(simple_ringbuffer_t *rb, uint8_t *data, size_t len, bool overwrite);
int simple_ringbuffer_read_char(simple_ringbuffer_t *rb);
int simple_ringbuffer_read(simple_ringbuffer_t *rb, uint8_t *ptr, size_t size);
size_t simple_ringbuffer_peek(simple_ringbuffer_t *rb, uint8_t **ptr, size_t size);
int simple_ringbuffer_peek_char(simple_ringbuffer_t *rb, size_t offset);

#endif /* PICO_TELNETD_RINGBUFFER_H */
