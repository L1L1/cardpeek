/**********************************************************************
*
* This file is part of Cardpeek, the smart card reader utility.
*
* Copyright 2009-2014 by Alain Pannetrat <L1L1@gmx.com>
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

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include "console_inprogress.h"

char ascii_pulse[4] = { '|', '/', '-', '\\' };

typedef struct {
    char *message;
    int pulse;
} inprogress_t;

void *console_inprogress_new(const char *title, const char *message)
{
   inprogress_t *prog = g_malloc(sizeof(inprogress_t));
   prog->message = strdup(message);
   prog->pulse = 0;
   printf("%s:\n",title);
   console_inprogress_pulse(prog);
   return prog;
}

unsigned console_inprogress_pulse(void *pulser)
{
    inprogress_t *prog = (inprogress_t *)pulser;
    printf("\r%s: %c",prog->message,ascii_pulse[prog->pulse%4]);
    prog->pulse++;
    return 1;
}

unsigned console_inprogress_set_fraction(void *pulser, double level)
{
    inprogress_t *prog = (inprogress_t *)pulser;
    printf("\r%s: %3i",prog->message,(int)(level*100));
    return 1;    
}

void console_inprogress_free(void *pulser)
{
    printf("\n");
    g_free(pulser);
} 
    

