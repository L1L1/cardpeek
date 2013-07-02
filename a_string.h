/********************************************************************** 
*
* This file is part of Cardpeek, the smart card reader utility.
*
* Copyright 2009-2013 by Alain Pannetrat <L1L1@gmx.com>
*
* Cardpeek is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* Cardpeek is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Cardpeek.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#ifndef A_STRING_H
#define A_STRING_H

typedef struct {
  unsigned _alloc;
  unsigned _size;
  char*    _data;
} a_string_t;

a_string_t* a_strnnew(unsigned n, const char* str);
a_string_t* a_strnew(const char* str);
void a_strfree(a_string_t* cs);
char* a_strfinalize(a_string_t* cs);

const char* a_strncpy(a_string_t* cs, unsigned n, const char* str);
const char* a_strcpy(a_string_t* cs, const char* str);
const char* a_strncat(a_string_t* cs, unsigned n, const char* str);
const char* a_strcat(a_string_t* cs, const char* str);

const char* a_strpushback(a_string_t* cs, char c);

const char* a_strval(const a_string_t* cs);
unsigned a_strlen(const a_string_t* cs);

int a_sprintf(a_string_t* cs, const char *format, ...);
int a_strnequal(const a_string_t* cs, unsigned n, const char *value);
int a_strequal(const a_string_t* cs, const char *value);

#endif
