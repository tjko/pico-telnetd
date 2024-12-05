/* server.c
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
#include <string.h>
#include <time.h>
#include <assert.h>
#include "pico/stdlib.h"
#include "pico/stdio/driver.h"

#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "pico_telnetd.h"
#include "pico_telnetd/log.h"


/* Telnet commands */
#define TELNET_SE     240
#define TELNET_NOP    241
#define TELNET_DM     242
#define TELNET_BRK    243
#define TELNET_IP     244
#define TELNET_AO     245
#define TELNET_AYT    246
#define TELNET_EC     247
#define TELNET_EL     248
#define TELNET_GA     249
#define TELNET_SB     250
#define TELNET_WILL   251
#define TELNET_WONT   252
#define TELNET_DO     253
#define TELNET_DONT   254
#define IAC           255

/* Telnet options */
#define TO_BINARY     0
#define TO_ECHO       1
#define TO_RECONNECT  2
#define TO_SUP_GA     3
#define TO_AMSN       4
#define TO_STATUS     5
#define TO_NAWS       31
#define TO_TSPEED     32
#define TO_RFLOWCTRL  33
#define TO_LINEMODE   34
#define TO_XDISPLOC   35
#define TO_ENV        36
#define TO_AUTH       37
#define TO_ENCRYPT    38
#define TO_NEWENV     39


#define TELNET_DEFAULT_PORT 23
#define TCP_SERVER_MAX_CONN 1
#define TCP_CLIENT_POLL_TIME 1

tcp_server_t *stdio_tcpserv = NULL;

static void (*chars_available_callback)(void*) = NULL;
static void *chars_available_param = NULL;

#ifndef LOG_MSG
#define LOG_MSG(...) { if (st->log_cb) st->log_cb(__VA_ARGS__); }
#endif

static const char *telnet_default_banner = "\r\npico-telnetd\r\n\r\n";
static const char* telnet_login_prompt = "\r\nlogin: ";
static const char* telnet_passwd_prompt = "\r\npassword: ";
static const char* telnet_login_failed = "\r\nLogin failed.\r\n";
static const char* telnet_login_success = "\r\nLogin successful.\r\n";

static const uint8_t telnet_default_options[] = {
	IAC, TELNET_DO, TO_SUP_GA,
	IAC, TELNET_WILL, TO_ECHO,
	IAC, TELNET_WONT, TO_LINEMODE,
};


static tcp_server_t* tcp_server_init(size_t rxbuf_size, size_t txbuf_size)
{
	tcp_server_t *st = calloc(1, sizeof(tcp_server_t));

	if (!st) {
		LOG_MSG(LOG_WARNING, "Failed to allocate tcp server state");
		return NULL;
	}

	telnet_ringbuffer_init(&st->rb_in, NULL, rxbuf_size);
	telnet_ringbuffer_init(&st->rb_out, NULL, txbuf_size);
	st->mode = RAW_MODE;
	st->cstate = CS_NONE;
	st->log_cb = telnetd_log_msg;
	st->auth_cb = NULL;
	st->port = TELNET_DEFAULT_PORT;
	st->banner = telnet_default_banner;
	st->auto_flush = true;

	return st;
}


static err_t close_client_connection(struct tcp_pcb *pcb)
{
	err_t err = ERR_OK;

	tcp_arg(pcb, NULL);
	tcp_sent(pcb, NULL);
	tcp_recv(pcb, NULL);
	tcp_err(pcb, NULL);
	tcp_poll(pcb, NULL, 0);

	if ((err = tcp_close(pcb)) != ERR_OK) {
		tcp_abort(pcb);
		err = ERR_ABRT;
	}

	return err;
}


static err_t tcp_server_close(void *arg) {
	tcp_server_t *st = (tcp_server_t*)arg;
	err_t err = ERR_OK;

	if (!arg)
		return ERR_VAL;

	st->cstate = CS_NONE;
	if (st->client) {
		err = close_client_connection(st->client);
		st->client = NULL;
	}

	if (st->listen) {
		tcp_arg(st->listen, NULL);
		tcp_accept(st->listen, NULL);
		if ((err = tcp_close(st->listen)) != ERR_OK) {
			LOG_MSG(LOG_NOTICE, "tcp_server_close: failed to close listen pcb: %d", err);
			tcp_abort(st->listen);
			err = ERR_ABRT;
		}
		st->listen = NULL;
	}

	return err;
}


static err_t tcp_server_sent(void *arg, struct tcp_pcb *pcb, u16_t len)
{
	tcp_server_t *st = (tcp_server_t*)arg;

	LOG_MSG(LOG_DEBUG, "tcp_server_sent: %u", len);
	return ERR_OK;
}


static void process_telnet_cmd(tcp_server_t *st)
{
	int resp = -1;


	switch(st->telnet_cmd) {
	case TELNET_DO:
		switch (st->telnet_opt) {
		case TO_ECHO:
			/* do nothing since we sent WILL for these...*/
			break;

		case TO_BINARY:
		case TO_SUP_GA:
			resp = TELNET_WILL;
			break;

		default:
			resp = TELNET_WONT;
			break;
		}
		break;

	case TELNET_WILL:
		switch (st->telnet_opt) {
		case TO_SUP_GA:
			/* do nothing since we sent DO for these... */
			break;

		case TO_NAWS:
		case TO_TSPEED:
		case TO_RFLOWCTRL:
		case TO_LINEMODE:
		case TO_XDISPLOC:
		case TO_ENV:
		case TO_AUTH:
		case TO_ENCRYPT:
		case TO_NEWENV:
			resp = TELNET_DONT;
			break;

		default:
			resp = TELNET_DO;
		}
		break;

	case TELNET_DONT:
	case TELNET_WONT:
		/* ignore these */
		break;

	default:
		LOG_MSG(LOG_DEBUG, "Unknown telnet command: %d\n", st->telnet_cmd);
		break;
	}


	if (resp >= 0) {
		uint8_t buf[3] = { IAC, resp, st->telnet_opt };
		tcp_write(st->client, buf, 3, TCP_WRITE_FLAG_COPY);
	}

	st->telnet_cmd_count++;
}


static err_t process_received_data(void *arg, uint8_t *buf, size_t len)
{
	tcp_server_t *st = (tcp_server_t*)arg;

	if (!arg || !buf)
		return ERR_VAL;

	if (len < 1)
		return ERR_OK;

	telnet_ringbuffer_t *rb = &st->rb_in;


	for(int i = 0; i < len; i++) {
		uint8_t c = buf[i];

		if (st->mode == TELNET_MODE) {
		telnet_state_macine:
			if (st->telnet_state == 0) { /* normal (pass-through) mode */
				if (c == IAC) {
					st->telnet_state = 1;
					continue;
				}
			}
			else if (st->telnet_state == 1) { /* IAC seen */
				if (c == IAC) { /* escaped 0xff */
					st->telnet_state = 0;
				}
				else { /* Telnet command */
					st->telnet_cmd = c;
					st->telnet_opt = 0;
					if (c == TELNET_WILL || c == TELNET_WONT
						|| c == TELNET_DO || c == TELNET_DONT
						|| c== TELNET_SB) {
						st->telnet_state = 2;
					} else {
						process_telnet_cmd(st);
						st->telnet_state = 0;
					}
					continue;
				}
			}
			else if (st->telnet_state == 2) { /* Telnet option */
				st->telnet_opt = c;
				if (st->telnet_cmd == TELNET_SB) {
					st->telnet_state = 3;
				} else {
					process_telnet_cmd(st);
					st->telnet_state = 0;
				}
				continue;
			}
			else if (st->telnet_state == 3) { /* Subnegotiation data */
				if (c == IAC)
					st->telnet_state = 4;
				continue;
			}
			else if (st->telnet_state == 4) { /* subnegotiation end? */
				if (c == IAC)
					st->telnet_state = 3;
				else {
					st->telnet_state = 1;
					goto telnet_state_macine;
				}
			}
			else {
				st->telnet_state = 0;
			}

			if (st->telnet_prev == 13 && c == 0) { // skip NUL after CR...
				st->telnet_prev = c;
				continue;
			}
			st->telnet_prev = c;
		}

		if (st->cstate == CS_AUTH_LOGIN) {
			/* Echo back characters when in login prompt... */
			char buf[1];
			buf[0] = c;
			tcp_write(st->client, buf, 1, TCP_WRITE_FLAG_COPY);
			tcp_output(st->client);
		}
		if (telnet_ringbuffer_add_char(rb, c, false) < 0)
			return ERR_MEM;
	}

	return ERR_OK;
}


static err_t authenticate_connection(tcp_server_t *st)
{
	int i;
	int l = -1;
	int len = telnet_ringbuffer_size(&st->rb_in);

	for (i = 0; i < len; i++) {
		int c = telnet_ringbuffer_peek_char(&st->rb_in, i);
		if (c == 10 || c == 13) {
			l = i;
			break;
		}
	}
	if (l < 0)
		return ERR_OK;

	if (st->cstate == CS_AUTH_LOGIN) {
		if (l >= sizeof(st->login))
			l = sizeof(st->login) - 1;
		telnet_ringbuffer_read(&st->rb_in, st->login, l+1);
		st->login[l] = 0;
		st->cstate = CS_AUTH_PASSWD;
		tcp_write(st->client, telnet_passwd_prompt, strlen(telnet_passwd_prompt), 0);
		tcp_output(st->client);
	} else if (st->cstate == CS_AUTH_PASSWD) {
		if (l >= sizeof(st->passwd))
			l = sizeof(st->passwd) - 1;
		telnet_ringbuffer_read(&st->rb_in, st->passwd, l+1);
		st->passwd[l] = 0;
		if (st->auth_cb(st->auth_cb_param, (const char*)st->login, (const char*)st->passwd) == 0) {
			st->cstate = CS_CONNECT;
			tcp_write(st->client, telnet_login_success,
				strlen(telnet_login_success), 0);
			LOG_MSG(LOG_NOTICE, "Successful login: %s (%s)",
				st->login, ip4addr_ntoa(&st->client->remote_ip));
		} else {
			st->cstate = CS_ACCEPT;
			tcp_write(st->client, telnet_login_failed,
				strlen(telnet_login_failed), 0);
			LOG_MSG(LOG_WARNING, "Login failure: %s (%s)",
				st->login, ip4addr_ntoa(&st->client->remote_ip));
		}
		tcp_output(st->client);
		memset(st->login, 0, sizeof(st->login));
		memset(st->passwd, 0, sizeof(st->passwd));
	}

	telnet_ringbuffer_flush(&st->rb_in);

	return ERR_OK;
}


static err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err)
{
	tcp_server_t *st = (tcp_server_t*)arg;
	struct pbuf *buf;
	size_t len;

	if (!p) {
		/* Connection closed by client */
		LOG_MSG(LOG_INFO, "Client closed connection: %s:%u (%d)",
			ip4addr_ntoa(&pcb->remote_ip), pcb->remote_port, err);
		close_client_connection(pcb);
		st->cstate = CS_NONE;
		st->client = NULL;
		return ERR_OK;
	}
	if (err != ERR_OK) {
		/* unknown error... */
		LOG_MSG(LOG_WARNING, "tcp_server_recv: error received: %d", err);
		if (p)
			pbuf_free(p);
		return err;
	}


	LOG_MSG(LOG_DEBUG, "tcp_server_recv: data received (pcb=%x): tot_len=%d, len=%d, err=%d",
		pcb, p->tot_len, p->len, err);


	buf = p;
	while (buf != NULL) {
		err = process_received_data(st, buf->payload, buf->len);
		if  (err != ERR_OK)
			break;
		buf = buf->next;
	}

	if ((len = telnet_ringbuffer_size(&st->rb_in)) > 0) {
		if (st->cstate == CS_AUTH_LOGIN || st->cstate == CS_AUTH_PASSWD) {
			authenticate_connection(st);
		}
		else if (st->cstate == CS_CONNECT && chars_available_callback) {
			chars_available_callback(chars_available_param);
		}
	}

	tcp_recved(pcb, p->tot_len);
	pbuf_free(p);

	return ERR_OK;
}


static int tcp_server_flush_buffer(tcp_server_t *st)
{
	uint8_t *rbuf;
	int waiting;
	int wcount = 0;

	if (!st)
		return -1;

	if (st->cstate != CS_CONNECT)
		return 0;

	while ((waiting = telnet_ringbuffer_size(&st->rb_out)) > 0) {
		size_t len = telnet_ringbuffer_peek(&st->rb_out, &rbuf, waiting);
		if (len > 0) {
			u8_t flags = TCP_WRITE_FLAG_COPY;
			if (len < waiting)
				flags |= TCP_WRITE_FLAG_MORE;
			err_t err = tcp_write(st->client, rbuf, len, flags);
			if (err != ERR_OK)
				break;
			telnet_ringbuffer_read(&st->rb_out, NULL, len);
			wcount++;
		} else {
			break;
		}
	}

	if (wcount > 0)
		tcp_output(st->client);

	return wcount;
}


static err_t tcp_server_poll(void *arg, struct tcp_pcb *pcb)
{
	tcp_server_t *st = (tcp_server_t*)arg;
	int wcount = 0;


	if (st->cstate == CS_ACCEPT) {
		if ((st->mode == TELNET_MODE && st->telnet_cmd_count > 0) || st->mode == RAW_MODE) {
			st->cstate = (st->auth_cb ? CS_AUTH_LOGIN : CS_CONNECT);
			if (st->banner) {
				tcp_write(pcb, st->banner, strlen(st->banner), TCP_WRITE_FLAG_COPY);
				wcount++;
			}
		}

		if (st->cstate == CS_AUTH_LOGIN) {
			tcp_write(pcb, telnet_login_prompt, strlen(telnet_login_prompt), 0);
			wcount++;
		}

		if (wcount > 0)
			tcp_output(pcb);
	}

	if (st->auto_flush && st->cstate == CS_CONNECT) {
		tcp_server_flush_buffer(st);
	}

	return ERR_OK;
}


static void tcp_server_err(void *arg, err_t err)
{
	tcp_server_t *st = (tcp_server_t*)arg;

	if (err == ERR_ABRT)
		return;

	LOG_MSG(LOG_ERR,"tcp_server_err: client connection error: %d", err);
}


static err_t tcp_server_accept(void *arg, struct tcp_pcb *pcb, err_t err)
{
	tcp_server_t *st = (tcp_server_t*)arg;


	if (!pcb || err != ERR_OK) {
		LOG_MSG(LOG_ERR, "tcp_server_accept: failure: %d", err);
		return ERR_VAL;
	}

	LOG_MSG(LOG_INFO, "Client connected: %s:%u",
		ip4addr_ntoa(&pcb->remote_ip), pcb->remote_port);

	if (st->cstate != CS_NONE) {
		LOG_MSG(LOG_ERR, "tcp_server_accept: reject connection");
		return ERR_MEM;
	}

	st->client = pcb;
	tcp_arg(pcb, st);
	tcp_sent(pcb, tcp_server_sent);
	tcp_recv(pcb, tcp_server_recv);
	tcp_poll(pcb, tcp_server_poll, TCP_CLIENT_POLL_TIME);
	tcp_err(pcb, tcp_server_err);

	st->cstate = CS_ACCEPT;
	st->telnet_state = 0;
	st->telnet_cmd_count = 0;
	telnet_ringbuffer_flush(&st->rb_in);
	telnet_ringbuffer_flush(&st->rb_out);

	if (st->mode == TELNET_MODE) { /* Send Telnet "handshake"... */
		tcp_write(pcb, telnet_default_options, sizeof(telnet_default_options), 0);
		tcp_output(pcb);
	}

	return ERR_OK;
}


static bool tcp_server_open(tcp_server_t *st) {
	struct tcp_pcb *pcb;
	err_t err;

	if (!st)
		return false;

	if (!(pcb = tcp_new_ip_type(IPADDR_TYPE_ANY))) {
		LOG_MSG(LOG_ERR, "tcp_server_open: failed to create pcb");
		return false;
	}

	if ((err = tcp_bind(pcb, NULL, st->port)) != ERR_OK) {
		LOG_MSG(LOG_ERR, "tcp_server_open: cannot bind to port %d: %d", st->port, err);
		tcp_abort(pcb);
		return false;
	}

	if (!(st->listen = tcp_listen_with_backlog(pcb, TCP_SERVER_MAX_CONN))) {
		LOG_MSG(LOG_ERR, "tcp_server_open: failed to listen on port %d", st->port);
		tcp_abort(pcb);
		return false;
	}

	tcp_arg(st->listen, st);
	tcp_accept(st->listen, tcp_server_accept);

	return true;
}


static void stdio_tcp_out_chars(const char *buf, int length)
{
	int count = 0;

	if (!stdio_tcpserv)
		return;
	if (stdio_tcpserv->cstate != CS_CONNECT)
		return;

	cyw43_arch_lwip_begin();
	for (int i = 0; i <length; i++) {
		if (telnet_ringbuffer_add_char(&stdio_tcpserv->rb_out, buf[i], false) < 0)
			break;
		count++;
	}
	if (count > 0)
		tcp_server_flush_buffer(stdio_tcpserv);
	cyw43_arch_lwip_end();
}


static int stdio_tcp_in_chars(char *buf, int length)
{
	int i = 0;
	int c;

	if (!stdio_tcpserv)
		return PICO_ERROR_NO_DATA;
	if (stdio_tcpserv->cstate != CS_CONNECT)
		return PICO_ERROR_NO_DATA;

	cyw43_arch_lwip_begin();
	while (i < length && ((c = telnet_ringbuffer_read_char(&stdio_tcpserv->rb_in)) >= 0)) {
		//printf("'%c' (%02x / %u)\n", isprint(c) ? c : '.', c, c);
		buf[i++] = c;
	}
	cyw43_arch_lwip_end();

	return i ? i : PICO_ERROR_NO_DATA;
}


static void stdio_tcp_set_chars_available_callback(void (*fn)(void*), void *param)
{
    chars_available_callback = fn;
    chars_available_param = param;
}


stdio_driver_t stdio_tcp_driver = {
	.out_chars = stdio_tcp_out_chars,
	.in_chars = stdio_tcp_in_chars,
	.set_chars_available_callback = stdio_tcp_set_chars_available_callback,
	.crlf_enabled = PICO_STDIO_DEFAULT_CRLF
};


static void stdio_tcp_init(tcp_server_t *server)
{
	stdio_tcpserv = server;
	chars_available_callback = NULL;
	chars_available_param = NULL;
	stdio_set_driver_enabled(&stdio_tcp_driver, true);
}


static void stdio_tcp_close(tcp_server_t *server)
{
	if (!stdio_tcpserv || stdio_tcpserv != server)
		return;
	stdio_set_driver_enabled(&stdio_tcp_driver, false);
	stdio_tcpserv = NULL;
	chars_available_callback = NULL;
	chars_available_param = NULL;
}


tcp_server_t* telnet_server_init(size_t rxbuf_size, size_t txbuf_size)
{
	return tcp_server_init(rxbuf_size > 0 ? rxbuf_size : 2048,
			txbuf_size > 0 ? txbuf_size : 2048);
}


bool telnet_server_start(tcp_server_t *server, bool stdio)
{
	cyw43_arch_lwip_begin();
	bool res = tcp_server_open(server);
	if (!res) {
		tcp_server_close(server);
	} else {
		if (stdio)
			stdio_tcp_init(server);
	}
	cyw43_arch_lwip_end();

	return res;
}


void telnet_server_destroy(tcp_server_t *server)
{
	cyw43_arch_lwip_begin();
	stdio_tcp_close(server);
	tcp_server_close(server);
	cyw43_arch_lwip_end();
	telnet_ringbuffer_free(&server->rb_in);
	telnet_ringbuffer_free(&server->rb_out);
}


int telnet_server_flush_buffer(tcp_server_t *st)
{
	cyw43_arch_lwip_begin();
	int res = tcp_server_flush_buffer(st);
	cyw43_arch_lwip_end();

	return res;
}
