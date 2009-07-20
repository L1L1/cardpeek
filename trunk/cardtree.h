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

#ifndef CARDTREE_H
#define CARDTREE_H

#include <gtk/gtk.h>

enum {
  C_MARKUP,
  C_LEAF,
  C_NODE,
  C_ID,
  C_LENGTH,
  C_COMMENT,
  NUM_COLS
};


typedef struct {
  GtkTreeView*  _view;
  GtkTreeStore* _store;
  gchar*        _tmpstr;
} cardtree_t;

cardtree_t* cardtree_new();

gboolean cardtree_is_empty(cardtree_t* ct);

const char* cardtree_append(cardtree_t* ct,
			    const char* path, gboolean leaf,
	      		    const char* node, const char* id, int length, const char* comment);

gboolean cardtree_get(cardtree_t* ct, const char* path,
		      gboolean* leaf,
		      char** node, char** id, int *length, char** comment);

gboolean cardtree_delete(cardtree_t* ct, const char *path);

const char* cardtree_find(cardtree_t* ct, const char *root, const char* node, const char *id);

char* cardtree_to_xml(cardtree_t* ct, const char *path);

int cardtree_to_xml_file(cardtree_t* ct, const char *fname, const char* path);

int cardtree_from_xml_file(cardtree_t *ct, const char *fname);

void cardtree_bind_to_treeview(cardtree_t* ct, GtkWidget *view);

void cardtree_free(cardtree_t* ct);

#endif
