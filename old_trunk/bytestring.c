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

#include "misc.h"
#include "bytestring.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

bytestring_t* bytestring_new()
{
  bytestring_t *res=(bytestring_t*)malloc(sizeof(bytestring_t));
  res->len=0;
  res->alloc=0;
  res->data=NULL;
  return res;
}

unsigned hex_nibble(const char nibble)
{
  if (nibble<='9' && nibble>='0')
    return nibble-'0';
  if (nibble<='F' && nibble>='A')
    return nibble-'A'+10;
  if (nibble<='f' && nibble>='a')
    return nibble-'a'+10;
  return BYTESTRING_NPOS;
}

bytestring_t* bytestring_new_from_hex(const char *hex)
{
  bytestring_t *dat=bytestring_new();
  unsigned hex_len=strlen(hex);
  unsigned c;
  size_t i=0;

  bytestring_reserve(dat,hex_len/2);
  
  while (i<hex_len)
  {
    if (is_blank(hex[i]))
    {
      i++;
      continue;
    }
    c=(hex_nibble(hex[i])<<4) | hex_nibble(hex[i+1]);
    if (c>255)
      return dat;
    bytestring_pushback(dat,(unsigned char)c);
    i+=2;
  }
  return dat;
}

bytestring_t* bytestring_duplicate(const bytestring_t *bs)
{
  bytestring_t* res=bytestring_new();
  bytestring_assign_data(res,bs->len,bs->data);
  return res;
}

int bytestring_assign_data(bytestring_t* bs,
                        unsigned len, const unsigned char *data)
{
  bytestring_resize(bs,len);
  memcpy(bs->data,data,len);
  return BYTESTRING_OK;
}

int bytestring_assign_uchar(bytestring_t* bs,
                         unsigned len, unsigned char c)
{
  bytestring_resize(bs,len);
  memset(bs->data,c,len);
  return BYTESTRING_OK;
}

int bytestring_copy(bytestring_t *bs,
                    const bytestring_t *src)
{
  return bytestring_assign_data(bs,src->len,src->data);
}

int bytestring_append(bytestring_t *bs,
                      const bytestring_t *extra)
{
  return bytestring_append_data(bs,extra->len,extra->data);
}

int bytestring_append_data(bytestring_t *bs,
                           unsigned len, const unsigned char *data)
{
  bytestring_reserve(bs,bs->len+len);
  memcpy(bs->data+bs->len,data,len);
  bs->len+=len;
  return BYTESTRING_OK;
}

int bytestring_append_uchar(bytestring_t* bs,
                            unsigned len, unsigned char c)
{
  bytestring_reserve(bs,bs->len+len);
  memset(bs->data+bs->len,c,len);
  bs->len+=len;
  return BYTESTRING_OK;
}

int bytestring_pushback(bytestring_t *bs,
                        unsigned char c)
{
  bytestring_reserve(bs,bs->len+1);
  bs->data[bs->len++]=c;
  return BYTESTRING_OK;
}

unsigned char bytestring_get_uchar(const bytestring_t *bs,
				   int pos)
{
  if (pos<0)
    pos=bs->len+pos;
  if (pos<bs->len && pos>=0)
    return bs->data[pos];
  return 0;
}

int bytestring_set_uchar(const bytestring_t *bs,
                         int pos, unsigned char c)
{
  if (pos<0)
    pos=bs->len+pos;
  if (pos<bs->len && pos>=0)
  {
    bs->data[pos]=c;
    return 1;
  }
  return 0;
}

void bytestring_clear(bytestring_t *bs)
{
  bs->len=0;
}

const unsigned char *bytestring_get_data(const bytestring_t *bs)
{
  return bs->data;
}

int bytestring_erase(bytestring_t *bs,
                     unsigned pos,
                     unsigned len)
{
  if (pos>=bs->len)
    return BYTESTRING_OK;
  if (pos+len>=bs->len) 
    return bytestring_resize(bs,pos);
  memmove(bs->data+pos,bs->data+pos+len,bs->len-pos-len);
  bytestring_resize(bs,bs->len-len);
  return BYTESTRING_OK;
}

int bytestring_is_empty(const bytestring_t *bs)
{
  return bs->len==0;
}

int bytestring_is_printable(const bytestring_t *bs)
{
  unsigned u;

  if (bs->len==0)
    return 0;
  for (u=0;u<bs->len;u++)
  {
    if (!isprint((char)(bs->data[u]))) return 0;
  }
  return 1;
}

int bytestring_insert_data(bytestring_t *bs,
                           unsigned pos,
                           unsigned len, const unsigned char* data)
{
  if (pos>=bs->len)
    return bytestring_append_data(bs,len,data);
  bytestring_resize(bs,bs->len+len);
  memmove(bs->data+pos+len,bs->data+pos,len);
  memcpy(bs->data,data,len);
  return BYTESTRING_OK;
}

int bytestring_insert_uchar(bytestring_t *bs,
                            unsigned pos,
                            unsigned len, unsigned char c)
{
  if (pos>=bs->len)
    return bytestring_append_uchar(bs,len,c);
  bytestring_resize(bs,bs->len+len);
  memmove(bs->data+pos+len,bs->data+pos,len);
  memset(bs->data,c,len);
  return BYTESTRING_OK;
}

int bytestring_insert(bytestring_t *bs,
                      unsigned pos,
                      const bytestring_t *src)
{
  return bytestring_insert_data(bs,pos,src->len,src->data);
}

int bytestring_reserve(bytestring_t *bs, unsigned size)
{
  if (size<=bs->alloc)
    return BYTESTRING_OK;
  if (bs->alloc==0)
  {
    bs->alloc=(size<8?8:size);
    bs->len=0;
    bs->data=(unsigned char *)malloc(bs->alloc);
  }
  else
  {
    while (bs->alloc<size) bs->alloc<<=1;
    bs->data=(unsigned char *)realloc(bs->data,bs->alloc);
    if (bs->data==NULL)
    {
      bs->alloc=0;
      bs->len=0;
      return BYTESTRING_ERROR;
    }
  }
  return BYTESTRING_OK;
}

int bytestring_resize(bytestring_t *bs, unsigned len)
{
  if (len>bs->len)
  {
    if (len>bs->alloc)
      bytestring_reserve(bs,len);
  }
  bs->len=len;
  return BYTESTRING_OK;
}

unsigned bytestring_get_size(const bytestring_t *bs)
{
  return bs->len;
}

int bytestring_substr(const bytestring_t *src,
		      unsigned pos, unsigned len,
		      bytestring_t* dst)
{
  if (pos>src->len)
  {
    bytestring_clear(dst);
    return 1;
  }
  if (len==BYTESTRING_NPOS || pos+len>src->len)
    len = src->len-pos;
  return bytestring_assign_data(dst,len,src->data+pos);
}

char *bytestring_to_string(const bytestring_t *bs)
{
  char *str=(char *)malloc(bs->len+1);
  unsigned u;
  for (u=0;u<bs->len;u++)
  {
    if (!isprint((char)(bs->data[u])))
      str[u]='?';
    else
      str[u]=(char)(bs->data[u]);
  }
  str[u]=0;
  return str;
}

const char HEXA[]="0123456789ABCDEF";

char *bytestring_to_hex_string(const bytestring_t *bs)
{
  char *str=(char *)malloc(bs->len*2+1);
  unsigned i;

  for (i=0;i<bs->len;i++)
  {
    str[i<<1]=HEXA[(bs->data[i]>>4)&0xF];
    str[(i<<1)+1]=HEXA[bs->data[i]&0xF];
  }
  str[i<<1]=0;
  return str;
}

void bytestring_free(bytestring_t *bs)
{
  if (bs->data)
    free(bs->data);
  /* bs->data=NULL;
     bs->alloc=0;
     bs->len=0; */
  free(bs);
}



