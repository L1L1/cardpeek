/********************************************************************** 
*
* This file is part of Cardpeek, the smartcard reader utility.
*
* Copyright 2009-2012 by 'L1L1'
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

#include <time.h>
#include "misc.h"
#include "pathconfig.h"
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <glib/gstdio.h>

const char ANSI_RESET[] = "\x1b[0m";
const char ANSI_RED[]     = "\x1b[31m";
const char ANSI_GREEN[]   = "\x1b[32m";
const char ANSI_YELLOW[]  = "\x1b[33m";
const char ANSI_BLUE[]    = "\x1b[34m";
const char ANSI_MAGENTA[] = "\x1b[35m";
const char ANSI_CYAN[]    = "\x1b[36m";
const char ANSI_WHITE[]   = "\x1b[37m";

const char *filename_extension(const char *fname)
{
	const char* dot = strrchr(fname, '.');
	if (!dot || dot==fname) return "";
	return dot; 
}

const char *filename_base(const char *fname)
{
	const char* slash = strrchr(fname, '/');
	if (!slash) slash = strrchr(fname, '\\');
	if (!slash) return fname;
	return slash+1; 
}

void logstring_default(int,const char *);

unsigned logpos=0;
logfunc_t LOGFUNCTION = logstring_default;
FILE* LOGFILE=NULL;

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
  char *buf=NULL;
  unsigned len_buf;

  va_start(al,format);
  len_buf = vsnprintf(buf,0,format,al);
  va_end(al);
  buf = (char *)malloc(len_buf+24);
  if (level==LOG_DEBUG)
    sprintf(buf,"%04i DEBUG   ",logpos++);
  else if (level==LOG_INFO)
    sprintf(buf,"%04i INFO    ",logpos++);
  else if (level==LOG_WARNING)
    sprintf(buf,"%04i WARNING ",logpos++);
  else if (level==LOG_ERROR)
    sprintf(buf,"%04i ERROR   ",logpos++);

  va_start(al,format);
  vsprintf(buf+strlen(buf),format,al);
  va_end(al);
  strcat(buf,"\n");
  if (LOGFUNCTION)
    LOGFUNCTION(level,buf);
  if (LOGFILE)
    fprintf(LOGFILE,"%s",buf);
  free(buf);
  
  return len_buf;
}

void log_set_function(logfunc_t logfunc)
{
  LOGFUNCTION=logfunc;
}

void log_open_file(void)
{
  time_t now = time(NULL);
  
  LOGFILE = g_fopen(path_config_get_string(PATH_CONFIG_FILE_LOG),"w+");

  if (LOGFILE)
    fprintf(LOGFILE,"cardpeek log start: %s",ctime(&now));
  else
    fprintf(stderr,"Could not open %s for output. Proceeding without a log file.\n",path_config_get_string(PATH_CONFIG_FILE_LOG));
}

void log_close_file(void)
{
  time_t now = time(NULL);

  if (LOGFILE) 
  {
    fprintf(LOGFILE,"cardpeek log ends: %s",ctime(&now));
    fclose(LOGFILE);
  }
  LOGFILE = NULL;
}

/*********************************************************/
guint cstring_hash(gconstpointer str)
{
	const unsigned char *s = str;
	guint res=0;
	while (*s)
	{
		res = (res*27)+(*s);
		s++;
	}
	return res;
}

gint cstring_equal(gconstpointer a, gconstpointer b)
{
	return (strcmp(a,b)==0);
}

