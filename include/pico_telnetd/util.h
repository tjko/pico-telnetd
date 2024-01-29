/* util.h
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

#ifndef _PICO_TELNETD_UTIL_H_
#define _PICO_TELNETD_UTIL_H_

char* generate_pwhash_salt(uint8_t len, char *salt);
char* generate_sha512crypt_pwhash(const char *password);
int sha512crypt_auth_cb(void *param, const char *login, const char *password);


#endif /* _PICO_TELNETD_UTIL_H_ */
