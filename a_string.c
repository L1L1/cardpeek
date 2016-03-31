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

#include "a_string.h"
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

/*********************************************************/

a_string_t* a_strnnew(unsigned n, const char* str)
{
  a_string_t* cs = malloc(sizeof(a_string_t)); //(a_string_t *)

  if (n)
  {
    cs->_alloc=n+1;
    cs->_size=cs->_alloc;
    cs->_data=(char *)malloc(cs->_alloc);
    if (str)
        a_strncpy(cs,n,str);
    else
        memset(cs->_data,0,n+1);
  }
  else
  {
    cs->_alloc=8;
    cs->_size=0;
    cs->_data=(char *)malloc(8);
    cs->_data[0]=0;
  }
  return cs;
}

a_string_t* a_strnew(const char* str)
{
  if (str)
    return a_strnnew(strlen(str),str);
  return a_strnnew(0,NULL);
}

void a_strfree(a_string_t* cs)
{
  if (cs->_alloc)
    free(cs->_data);
  memset(cs,0,sizeof(a_string_t));
  free(cs);
}

char* a_strfinalize(a_string_t* cs)
{
  char *res = cs->_data;
  free(cs);
  return res;
}


const char* a_strncpy(a_string_t* cs, unsigned n, const char* str)
{
  cs->_size=0;
  return a_strncat(cs,n,str);
}

const char* a_strcpy(a_string_t* cs, const char* str)
{
  if (str==NULL) return a_strncpy(cs,0,""); 

  return a_strncpy(cs,strlen(str),str);
}

const char* a_strncat(a_string_t* cs, unsigned n, const char* str)
{
  unsigned needed_size;
  unsigned p;

  if (cs->_alloc==0) /* should never happen */
    return NULL;

  if (str==NULL)
    return cs->_data;
 
  if (cs->_size==0) /* cs->_alloc != 0 */
  {
    cs->_size=1;
    cs->_data[0]=0;
  }

  p=0;
  while (p<n && str[p]) p++;
  if (p<n) n=p;

  needed_size = n + cs->_size;

  if (needed_size>cs->_alloc)
  {
    while (needed_size>cs->_alloc) cs->_alloc<<=1;
    cs->_data=(char *)realloc(cs->_data,cs->_alloc);
  }

  memcpy(cs->_data+cs->_size-1,str,n);
  cs->_size=needed_size;
  cs->_data[cs->_size-1]=0;

  return cs->_data;
}

const char* a_strcat(a_string_t* cs, const char* str)
{
  return a_strncat(cs,strlen(str),str);
}

const char* a_strpushback(a_string_t* cs, char c)
{
  return a_strncat(cs,1,&c);
}

const char* a_memcpy(a_string_t* cs, unsigned n, const void* str)
{
    cs->_size=0;
    return a_memcat(cs,n,str);
}

const char* a_memcat(a_string_t* cs, unsigned n, const void* str)
{
    unsigned needed_size;

    if (cs->_alloc==0) /* should never happen */
        return NULL;

    if (str==NULL)
        return cs->_data;

    if (cs->_size==0) /* cs->_alloc != 0 */
    {
        cs->_size=1;
        cs->_data[0]=0;
    }

    needed_size = n + cs->_size;

    if (needed_size>cs->_alloc)
    {
        while (needed_size>cs->_alloc) cs->_alloc<<=1;
        cs->_data=(char *)realloc(cs->_data,cs->_alloc);
    }

    memcpy(cs->_data+cs->_size-1,str,n);
    cs->_data[n + cs->_size]=0;
    cs->_size=needed_size;

    return cs->_data;
}

const char* a_mempushback(a_string_t* cs, unsigned char c)
{
    return a_memcat(cs,1,&c);
}

const char* a_strval(const a_string_t* cs)
{
  return cs->_data;
}

unsigned a_strlen(const a_string_t* cs)
{
  if (cs->_size==0)
    return 0;
  return cs->_size-1;
}

int a_sprintf(a_string_t* cs, const char *format, ...)
{
  unsigned reclen;
  va_list al;

  va_start(al,format);
  reclen=vsnprintf(cs->_data,0,format,al);
  va_end(al);
  if (reclen+1>cs->_alloc)
  {
    cs->_alloc=reclen+1;
    free(cs->_data);
    cs->_data=(char *)malloc(cs->_alloc);
  }
  va_start(al,format);
  vsnprintf(cs->_data,reclen+1,format,al);
  va_end(al);
  cs->_size=reclen+1;
  return reclen;
}

int a_strnequal(const a_string_t* cs, unsigned n, const char *value)
{
  if (cs->_alloc==0)
    return 0;
  if (n!=a_strlen(cs))
    return 0;
  return memcmp(cs->_data,value,n)==0;
}

int a_strequal(const a_string_t* cs, const char *value)
{
  return a_strnequal(cs,strlen(value),value);
}

