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

#ifndef BYTESTRING_H
#define BYTESTRING_H

typedef struct {
  unsigned len;
  unsigned alloc;
  unsigned char width;
  unsigned char mask;
  unsigned char *data;
} bytestring_t;

enum {
  BYTESTRING_ERROR=0,
  BYTESTRING_OK=1,
  BYTESTRING_NPOS=0x7FFFFFFF
};

/* constructors */

int bytestring_init(bytestring_t *bs, unsigned element_width);

bytestring_t* bytestring_new(unsigned element_width);

bytestring_t* bytestring_new_from_string(const char *str);

bytestring_t* bytestring_duplicate(const bytestring_t *bs);

/* assign */

int bytestring_assign_data(bytestring_t* bs, 
			   unsigned len, const unsigned char *data);
int bytestring_assign_element(bytestring_t* bs, 
			      unsigned len, unsigned char c);
int bytestring_assign_digit_string(bytestring_t* bs,
				   const char* str);

/* copy */

int bytestring_copy(bytestring_t *bs, 
		    const bytestring_t *src);

/* convert */

int bytestring_convert(bytestring_t *bs,
		       const bytestring_t *src);

/* append data */

int bytestring_append(bytestring_t *bs, 
		      const bytestring_t *extra);
int bytestring_append_data(bytestring_t *bs, 
			   unsigned len, const unsigned char *data);
int bytestring_append_element(bytestring_t* bs,
			      unsigned len, unsigned char c);

int bytestring_pushback(bytestring_t *bs, 
			unsigned char c);

/* accessors */

int bytestring_set_element(const bytestring_t *bs,
			   int pos, unsigned char element);


int bytestring_get_element(unsigned char* element,
			   const bytestring_t *bs, 
			   int pos);

const unsigned char *bytestring_get_data(const bytestring_t *bs);

/* invert */

int bytestring_invert(bytestring_t *bs);

/* clear */

void bytestring_clear(bytestring_t *bs);


int bytestring_erase(bytestring_t *bs, 
		     unsigned pos, 
		     unsigned len);

/* tests */

int bytestring_is_equal(const bytestring_t *a, const bytestring_t *b);

int bytestring_is_empty(const bytestring_t *bs);

int bytestring_is_printable(const bytestring_t *bs);

/* insertion */

int bytestring_insert_data(bytestring_t *bs, 
			   unsigned pos, 
			   unsigned len, const unsigned char* data);
int bytestring_insert_element(bytestring_t *bs, 
			      unsigned pos,
			      unsigned len, unsigned char c);
int bytestring_insert(bytestring_t *bs,
		      unsigned pos,
		      const bytestring_t *src);

int bytestring_pad_left(bytestring_t *bs, 
			unsigned block_size, unsigned char c);

int bytestring_pad_right(bytestring_t *bs,
			 unsigned block_size, unsigned char c);

/* get size / resize */

int bytestring_resize(bytestring_t *bs, unsigned len);

unsigned bytestring_get_size(const bytestring_t *bs);

/* substring */

int bytestring_substr(bytestring_t *dst,
		      unsigned pos, unsigned len,
		      const bytestring_t* src);

/* conversion to other types */

char *bytestring_to_format(const char *format, const bytestring_t *bs);

double bytestring_to_number(const bytestring_t *bs);

/* destructors */

void bytestring_release(bytestring_t *bs);

void bytestring_free(bytestring_t *bs);

/* bytestring_release frees data in bs, bytestring_free also frees bs itself. */

#endif


