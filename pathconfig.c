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

#include "pathconfig.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <unistd.h>
#include "misc.h"

const char *PATH_CONFIG_OPTIONS[]=
{
    NULL,
    NULL,
    "",
    ".cardpeek",
    ".cardpeek/scripts",
    ".cardpeek/replay",
    ".cardpeek/logs",
    ".cardpeek/config.lua",
    ".cardpeek/cardpeekrc.lua",
    ".cardpeek.log",
    ".cardpeek/version",
    ".cardpeek/scripts/etc/smartcard_list.txt",
    ".cardpeek/scripts/etc/smartcard_list.download",
    NULL
};

char *PATH_CONFIG_STRING[NUM_PATH_CONFIG_OPTIONS];


int path_config_init(void)
{
    char path_config_path[PATH_MAX];
    char cwd_path[PATH_MAX];
    const char *home;
    unsigned i;

    home = getenv("CARDPEEK_HOME");

    if (home==NULL)
    {
#ifndef _WIN32
        home = getenv("HOME");
#else
        home = getenv("USERDATA");
        if (home==NULL)
            home = getenv("USERPROFILE");
#endif
        if (home==NULL)
            return 0;
    }

    if (!getcwd(cwd_path,PATH_MAX))
        strcpy(cwd_path,path_config_path);

    PATH_CONFIG_STRING[0]=strdup(cwd_path);
    PATH_CONFIG_STRING[1]=strdup(cwd_path);

    for (i=2; i<NUM_PATH_CONFIG_OPTIONS; i++)
    {
        sprintf(path_config_path,"%s/%s",home,PATH_CONFIG_OPTIONS[i]);
        PATH_CONFIG_STRING[i]=strdup(path_config_path);
    }
    return 1;
}

const char *path_config_get_string(unsigned c_index)
{
    if (c_index>NUM_PATH_CONFIG_OPTIONS)
        return NULL;
	
    return PATH_CONFIG_STRING[c_index];
}

int path_config_set_string(unsigned c_index, const char *path)
{
    if (c_index>NUM_PATH_CONFIG_OPTIONS)
        return 0;
    if (PATH_CONFIG_STRING[c_index])
        free(PATH_CONFIG_STRING[c_index]);
    if (path)
        PATH_CONFIG_STRING[c_index]=strdup(path);
    else
        PATH_CONFIG_STRING[c_index]=strdup("");
    return 1;
}

void path_config_release(void)
{
    unsigned i;
    for (i=0; i<NUM_PATH_CONFIG_OPTIONS; i++)
        free(PATH_CONFIG_STRING[i]);
}

