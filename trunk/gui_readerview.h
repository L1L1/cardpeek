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

#ifndef GUI_READERVIEW_H
#define GUI_READERVIEW_H

#include <gtk/gtk.h>
#include "bytestring.h"

void gui_readerview_print(unsigned event,
                          const bytestring_t *command,
                          unsigned short sw,
                          const bytestring_t *response,
                          void *extra_data);

GtkWidget *gui_readerview_create_window(void);

void gui_readerview_cleanup(void);

#endif
