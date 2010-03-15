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

#ifndef GUI_H
#define GUI_H

#include "cardtree.h"

extern cardtree_t* CARDTREE;

typedef void (*application_callback_t)(const char *);

int gui_init(int *argc, char ***argv);

int gui_create(application_callback_t run_script_cb,
	       application_callback_t run_command_cb);

int gui_run();

void gui_update(int lag_allowed);

void gui_expand_view();

char* gui_select_reader(unsigned list_size, const char** list);

int gui_question_l(const char *message, unsigned item_count, const char** items);

int gui_question(const char *message, ...);

int gui_readline(const char *message, unsigned input_max, char* input);


char** gui_select_file(const char *title,
   		       const char *path,
   		       const char *filename);

void gui_about();

#endif