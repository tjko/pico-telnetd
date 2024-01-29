/* log.c
   Copyright (C) 2023 Timo Kokkonen <tjko@iki.fi>

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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <wctype.h>
#include <assert.h>
#include <malloc.h>
#include <time.h>
#include "pico/stdlib.h"
#include "pico/util/datetime.h"

#include "pico_telnetd/log.h"

#define LOG_MAX_MSG_LEN 256


static int global_log_level = LOG_ERR;

struct log_priority {
	uint8_t  priority;
	const char *name;
};

static const struct log_priority log_priorities[] = {
	{ LOG_EMERG,   "EMERG" },
	{ LOG_ALERT,   "ALERT" },
	{ LOG_CRIT,    "CRIT" },
	{ LOG_ERR,     "ERR" },
	{ LOG_WARNING, "WARNING" },
	{ LOG_NOTICE,  "NOTICE" },
	{ LOG_INFO,    "INFO" },
	{ LOG_DEBUG,   "DEBUG" },
	{ 0, NULL }
};


static const char* log_priority2str(int pri)
{
	int i = 0;

	while(log_priorities[i].name) {
		if (log_priorities[i].priority == pri)
			return log_priorities[i].name;
		i++;
	}

	return NULL;
}

void telnetd_log_level(int priority)
{
	global_log_level = priority;
}




void telnetd_log_msg(int priority, const char *format, ...)
{
	va_list ap;
	char *buf;
	char tstamp[32];
	int len;
	uint core = get_core_num();

	if (priority > global_log_level)
		return;

	if (!(buf = malloc(LOG_MAX_MSG_LEN)))
		return;

	va_start(ap, format);
	vsnprintf(buf, LOG_MAX_MSG_LEN, format, ap);
	va_end(ap);

	if ((len = strnlen(buf, LOG_MAX_MSG_LEN - 1)) > 0) {
		/* If string ends with \n, remove it. */
		if (buf[len - 1] == '\n')
			buf[len - 1] = 0;
	}


	uint64_t t = to_us_since_boot(get_absolute_time());
	snprintf(tstamp, sizeof(tstamp), "[%6llu.%06llu][%u]",
		(t / 1000000), (t % 1000000), core);
	printf("%s %s %s\n", tstamp, log_priority2str(priority), buf);

	free(buf);
}


