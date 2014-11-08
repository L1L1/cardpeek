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

#ifndef LUAX_H
#define LUAX_H

#include "smartcard.h"
#include <glib.h>

void luax_set_card_reader(cardreader_t* r);

void luax_run_script(const char* scriptname);

void luax_run_command(const char* command);

int luax_init(void);

void luax_release(void);

char *luax_escape_string(const char *src);

enum {
    LUAX_INIT_WITHOUT_UI=0,
    LUAX_INIT_WITH_UI=1
};

/*****/

gboolean luax_config_table_save(void);

char *luax_variable_get_strdup(const char *vname);

gboolean luax_variable_set_strval(const char *vname, const char *value);

int luax_variable_get_integer(const char *vname);

gboolean luax_variable_set_integer(const char *vname, int value);

gboolean luax_variable_get_boolean(const char *vname);

gboolean luax_variable_set_boolean(const char *vname, gboolean value);

gboolean luax_variable_is_defined(const char *vname);

gboolean luax_variable_call(const char *vname, const char *format, ...);

#endif
