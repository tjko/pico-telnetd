/* pico_telnetd.h
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

#ifndef TCPSERVER_H
#define TCPSERVER_H 1

#include "pico/stdio.h"
#include "pico_telnetd/ringbuffer.h"



typedef struct user_pwhash_entry {
	const char *login;
	const char *hash;
} user_pwhash_entry_t;


typedef enum tcp_server_mode {
	RAW_MODE = 0,
	TELNET_MODE
} tcp_server_mode_t;

typedef enum tcp_connection_state {
	CS_NONE = 0,
	CS_ACCEPT,
	CS_AUTH_LOGIN,
	CS_AUTH_PASSWD,
	CS_CONNECT,
	CS_CLOSE,
} tcp_connection_state_t;

typedef struct tcp_server_t {
	struct tcp_pcb *listen;
	struct tcp_pcb *client;
	tcp_connection_state_t cstate;
	telnet_ringbuffer_t rb_in;
	telnet_ringbuffer_t rb_out;
	int telnet_state;
	uint8_t telnet_cmd;
	uint8_t telnet_opt;
	uint8_t telnet_prev;
	int telnet_cmd_count;
	uint8_t login[32 + 1];
	uint8_t passwd[64 + 1];

	/* Configuration options... set before calling telnet_server_start() */
	uint16_t port;             /* Listen port (default is telnet port 23) */
	tcp_server_mode_t mode;    /* Server mode: TELNET_MODE or RAW_MODE */
	const char *banner;        /* Login banner string to display when connection starts. */
	void (*log_cb)(int priority, const char *format, ...);
	int (*auth_cb)(void* param, const char *login, const char *password);
	void *auth_cb_param;
} tcp_server_t;



tcp_server_t* telnet_server_init(size_t rxbuf_size, size_t tzbuf_set);
bool telnet_server_start(tcp_server_t *server, bool stdio);
void telnet_server_destroy(tcp_server_t *server);



#endif /* TCPSERVER_H */
