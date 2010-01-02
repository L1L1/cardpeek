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

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include "misc.h"

const char *CONFIG_OPTIONS[]={
  "",
  "",
  "",
  ".cardpeek/",
  ".cardpeek/scripts/",
  ".cardpeek/config.lua",
  NULL
};

char *CONFIG_STRING[NUM_CONFIG_OPTIONS];


int config_init()
{
  char config_path[PATH_MAX];
  char *home=getenv("HOME");
  unsigned i;
  
  if (home==NULL)
    return 0;

  for (i=0;i<NUM_CONFIG_OPTIONS;i++)
  {
    sprintf(config_path,"%s/%s",home,CONFIG_OPTIONS[i]);
    CONFIG_STRING[i]=strdup(config_path);
  }
  return 1;
}

const char *config_get_string(unsigned index)
{
  if (index>NUM_CONFIG_OPTIONS)
    return NULL;
  return CONFIG_STRING[index];
}

int config_set_string(unsigned index, const char *path)
{
  if (index>NUM_CONFIG_OPTIONS)
    return 0;
  if (CONFIG_STRING[index])
    free(CONFIG_STRING[index]);
  if (path)
    CONFIG_STRING[index]=strdup(path);
  else
    CONFIG_STRING[index]=strdup("");
  return 1;
}

void config_release()
{
  unsigned i;
  for (i=0;i<NUM_CONFIG_OPTIONS;i++)
    free(CONFIG_STRING[i]);
}

