/********************************************************************** 
*
* This file is part of Cardpeek, the smartcard reader utility.
*
* Copyright 2009 by 'L1L1'
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

#ifndef BYTESTRING_H
#define BYTESTRING_H

typedef struct {
  unsigned len;
  unsigned alloc;
  unsigned char *data;
} bytestring_t;

enum {
  BYTESTRING_ERROR=0,
  BYTESTRING_OK=1,
  BYTESTRING_NPOS=0x7FFFFFFF
};

bytestring_t* bytestring_new();
bytestring_t* bytestring_new_from_hex(const char *hex);
bytestring_t* bytestring_duplicate(const bytestring_t *bs);

int bytestring_assign_data(bytestring_t* bs, 
			unsigned len, const unsigned char *data);
int bytestring_assign_uchar(bytestring_t* bs, 
   			    unsigned len, unsigned char c);
int bytestring_copy(bytestring_t *bs, 
		    const bytestring_t *src);

int bytestring_append(bytestring_t *bs, 
		      const bytestring_t *extra);
int bytestring_append_data(bytestring_t *bs, 
			   unsigned len, const unsigned char *data);
int bytestring_append_uchar(bytestring_t* bs,
   			    unsigned len, unsigned char c);

int bytestring_pushback(bytestring_t *bs, 
			unsigned char c);

int bytestring_set_uchar(const bytestring_t *bs,
			 int pos, unsigned char c);


unsigned char bytestring_get_uchar(const bytestring_t *bs, 
				   int pos);

void bytestring_clear(bytestring_t *bs);

const unsigned char *bytestring_get_data(const bytestring_t *bs);

int bytestring_erase(bytestring_t *bs, 
		     unsigned pos, 
		     unsigned len);
int bytestring_is_empty(const bytestring_t *bs);
int bytestring_is_printable(const bytestring_t *bs);

int bytestring_insert_data(bytestring_t *bs, 
			   unsigned pos, 
			   unsigned len, const unsigned char* data);
int bytestring_insert_uchar(bytestring_t *bs, 
			    unsigned pos,
			    unsigned len, unsigned char c);
int bytestring_insert(bytestring_t *bs,
		      unsigned pos,
		      const bytestring_t *src);

int bytestring_reserve(bytestring_t *bs, unsigned size);
int bytestring_resize(bytestring_t *bs, unsigned len);
unsigned bytestring_get_size(const bytestring_t *bs);

int bytestring_substr(const bytestring_t *src,
		      unsigned pos, unsigned len,
		      bytestring_t* dst);

char *bytestring_to_string(const bytestring_t *bs);
char *bytestring_to_hex_string(const bytestring_t *bs);

void bytestring_free(bytestring_t *bs);


/* unsigned subbitstr(unsigned bpos, unsigned blen) const; */

#endif


