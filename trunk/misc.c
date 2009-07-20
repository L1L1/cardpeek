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
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>



const char ANSI_RESET[] = "\x1b[0m";
const char ANSI_RED[]     = "\x1b[31m";
const char ANSI_GREEN[]   = "\x1b[32m";
const char ANSI_YELLOW[]  = "\x1b[33m";
const char ANSI_BLUE[]    = "\x1b[34m";
const char ANSI_MAGENTA[] = "\x1b[35m";
const char ANSI_CYAN[]    = "\x1b[36m";
const char ANSI_WHITE[]   = "\x1b[37m";


void logstring_default(int,const char *);

unsigned logpos=0;
logfunc_t LOGFUNCTION = logstring_default;

void logstring_default(int level, const char *str)
{
  if (level==LOG_DEBUG)
    fprintf(stderr,"%s",str);
  if (level==LOG_INFO)
    fprintf(stderr,"%s%s",ANSI_GREEN,str);
  if (level==LOG_WARNING)
    fprintf(stderr,"%s%s",ANSI_MAGENTA,str);
  if (level==LOG_ERROR)
    fprintf(stderr,"%s%s",ANSI_RED,str);
  fprintf(stderr,"%s",ANSI_RESET);
}

int log_printf(int level, const char *format, ...)
{
  va_list al;
  char *buf;
  unsigned len_buf;

  va_start(al,format);
  len_buf = vsnprintf(buf,0,format,al);
  buf = (char *)malloc(len_buf+24);
  if (level==LOG_DEBUG)
    sprintf(buf,"%04i DEBUG   ",logpos++);
  else if (level==LOG_INFO)
    sprintf(buf,"%04i INFO    ",logpos++);
  else if (level==LOG_WARNING)
    sprintf(buf,"%04i WARNING ",logpos++);
  else if (level==LOG_ERROR)
    sprintf(buf,"%04i ERROR   ",logpos++);

  vsprintf(buf+strlen(buf),format,al);
  strcat(buf,"\n");
  if (LOGFUNCTION)
    LOGFUNCTION(level,buf);
  free(buf);
  va_end(al);
  return len_buf;
}

void log_activate(logfunc_t logfunc)
{
  LOGFUNCTION=logfunc;
}

/*********************************************************/

a_string_t* a_strnnew(unsigned n, const char* str)
{
  a_string_t* cs = (a_string_t *)malloc(sizeof(a_string_t));

  if (str)
  {
    cs->_alloc=n+1;
    cs->_size=cs->_alloc;
    cs->_data=(char *)malloc(cs->_alloc);
    a_strncpy(cs,n,str);
  }
  else
  {
    cs->_alloc=8;
    cs->_size=0;
    cs->_data=(char *)malloc(8);
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
  char *res = a_strval(cs);
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
  return a_strncpy(cs,strlen(str),str);
}

const char* a_strncat(a_string_t* cs, unsigned n, const char* str)
{
  unsigned needed_size;

  if (cs->_alloc==0) /* should never happen */
    return NULL;
 
  if (cs->_size==0) /* cs->_alloc != 0 */
  {
    cs->_size=1;
    cs->_data[0]=0;
  }

  needed_size = n + cs->_size;

  /*fprintf(stderr,"BEG[%i|%i|%s]\n",cs->_size,cs->_alloc,cs->_data);*/

  if (needed_size>cs->_alloc)
  {
    while (needed_size>cs->_alloc) cs->_alloc<<=1;
    cs->_data=(char *)realloc(cs->_data,cs->_alloc);
  }

  memcpy(cs->_data+cs->_size-1,str,n);
  cs->_size=needed_size;
  cs->_data[cs->_size-1]=0;

  /*fprintf(stderr,"END[%i|%i|%s]\n",cs->_size,cs->_alloc,cs->_data);*/

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

char* a_strval(a_string_t* cs)
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
  if (reclen+1>cs->_alloc)
  {
    cs->_alloc=reclen+1;
    free(cs->_data);
    cs->_data=(char *)malloc(cs->_alloc);
  }
  vsnprintf(cs->_data,reclen+1,format,al);
  cs->_size=reclen+1;
  va_end(al);
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

/******
void a_strdump(const a_string_t* cs)
{
  unsigned u;
  for (u=0;u<cs->_size;u++)
  {
    if (cs->_data[u]<' ')
      fprintf(stderr,"\\x%02x",cs->_data[u]);
    else
      fprintf(stderr,"%c",cs->_data[u]);
  }
  fprintf(stderr,"\n");
}

int main()
{
  a_string_t *s;

  s=a_strnew(NULL);
  a_strcpy(s,"Alain");
  a_strdump(s);
  a_strncat(s,6,"Boris");
  a_strdump(s);
  a_strcat(s,"Celine");
  a_strdump(s);
  a_strfree(s);
}
****/
