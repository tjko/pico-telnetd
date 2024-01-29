/* utils.c
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
#include "pico/rand.h"
#include "pico_telnetd.h"
#include "pico_telnetd/sha512crypt.h"


static const char b64t[64] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

char* generate_pwhash_salt(uint8_t len, char *salt)
{
	int i = 0;
	int left = 0;
	uint32_t rnd;

	if (!salt)
		return NULL;

	while (i < len) {
		if (left < 6) {
			rnd = get_rand_32();
			left = 32;
		}
		salt[i++] = b64t[rnd & 0x3f];
		rnd >>= 6;
		left -= 6;
	}
	salt[i] = 0;

	return salt;
}

char* generate_sha512crypt_pwhash(const char *password)
{
	char salt[16 + 1];
	char settings[16 + 3 + 1];

	snprintf(settings, sizeof(settings), "$6$%s", generate_pwhash_salt(16, salt));

	return sha512_crypt(password, settings);
}

int sha512crypt_auth_cb(void *param, const char *login, const char *password)
{
	user_pwhash_entry_t *users = (user_pwhash_entry_t*)param;
	int i = 0;

	while (users[i].login) {
		if (!strcmp(login, users[i].login)) {
			/* Found matching user */
			char *hash = sha512_crypt(password, users[i].hash);
			if (!strcmp(hash, users[i].hash)) {
				/* password matches */
				return 0;
			} else {
				/* password does not match */
				return 1;
			}
		}
		i++;
	}

	/* no user found, authentication failure... */
	return -1;
}
