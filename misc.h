/********************************************************************** 
*
* This file is part of Cardpeek, the smartcard reader utility.
*
* Copyright 2009-2013 by 'L1L1'
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

#ifndef MISC_H
#define MISC_H
#include <glib.h>

#ifdef _WIN32
#include "win32/config.h"
#else
#include "config.h"
#endif

#ifndef HAVE_GSTATBUF
#include <unistd.h>
typedef struct stat GStatBuf;
#endif


#ifdef __APPLE__
#define DIRENT_T struct dirent
#else
#define DIRENT_T const struct dirent
#endif


#define is_hex(a)       ((a>='0' && a<='9') || \
                         (a>='A' && a<='F') || \
			 (a>='a' && a<='f'))

#define is_blank(a)     (a==' ' || a=='\t' || a=='\r' || a=='\n')

const char *filename_extension(const char *fname);
const char *filename_base(const char *fname);

typedef void (*logfunc_t)(int,const char*);

int log_printf(int level, const char *format, ...);

void log_set_function(logfunc_t logfunc);

void log_open_file(void);

void log_close_file(void);

enum {
  LOG_DEBUG,
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR
};

extern const char ANSI_RESET[];
extern const char ANSI_RED[];
extern const char ANSI_GREEN[];
extern const char ANSI_YELLOW[];
extern const char ANSI_BLUE[];
extern const char ANSI_MAGENTA[];
extern const char ANSI_CYAN[];
extern const char ANSI_WHITE[];

guint cstring_hash(gconstpointer data);
gint cstring_equal(gconstpointer a, gconstpointer b);

#include <stdio.h>
#define HERE() { fprintf(stderr,"%s[%i]\n",__FILE__,__LINE__); fflush(stderr); }

#define UNUSED(x) (void)x

#endif
