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

#ifndef GUI_TOOLBAR_H
#define GUI_TOOLBAR_H

#include <gtk/gtk.h>

typedef struct {
        const gchar *id;
        const gchar *icon;
        const gchar *text;
        GCallback callback;
        gconstpointer callback_data;
} toolbar_item_t; 

#define TOOLBAR_ITEM_SEPARATOR  "separator"
#define TOOLBAR_ITEM_EXPANDER "expander"

GtkWidget *gui_toolbar_new(toolbar_item_t *tbitems);

void gui_toolbar_run_command_cb(GtkWidget *w, gconstpointer user_data);

#endif
