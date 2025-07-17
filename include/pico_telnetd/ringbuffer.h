/* ringbuffer.h
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

#ifndef PICO_TELNETD_RINGBUFFER_H
#define PICO_TELNETD_RINGBUFFER_H 1

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif


typedef struct telnet_ringbuffer {
	uint8_t *buf;
	bool free_buf;
	size_t size;
	size_t free;
	size_t head;
	size_t tail;
} telnet_ringbuffer_t;


int telnet_ringbuffer_init(telnet_ringbuffer_t *rb, uint8_t *buf, size_t size);
int telnet_ringbuffer_free(telnet_ringbuffer_t *rb);
int telnet_ringbuffer_flush(telnet_ringbuffer_t *rb);
size_t telnet_ringbuffer_size(telnet_ringbuffer_t *rb);
int telnet_ringbuffer_add_char(telnet_ringbuffer_t *rb, uint8_t ch, bool overwrite);
int telnet_ringbuffer_add(telnet_ringbuffer_t *rb, uint8_t *data, size_t len, bool overwrite);
int telnet_ringbuffer_read_char(telnet_ringbuffer_t *rb);
int telnet_ringbuffer_read(telnet_ringbuffer_t *rb, uint8_t *ptr, size_t size);
size_t telnet_ringbuffer_peek(telnet_ringbuffer_t *rb, uint8_t **ptr, size_t size);
int telnet_ringbuffer_peek_char(telnet_ringbuffer_t *rb, size_t offset);


#ifdef __cplusplus
}
#endif

#endif /* PICO_TELNETD_RINGBUFFER_H */
