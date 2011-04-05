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
  /* node info */
  C_NAME,
  C_ID,
  C_SIZE,
  C_TYPE,
  /* value data */
  C_VALUE,
  C_ALT_VALUE,
  /* extra helper columns */
  C_MARKUP_VALUE,
  C_MARKUP_NAME_ID,
  C_MARKUP_ALT_VALUE,
  C_ICON,
  C_FLAGS,
  NUM_COLS
};

#define CARDTREE_COUNT_ICONS 6

typedef struct {
  GtkTreeStore* _store;
  gchar*        _tmpstr;
  GdkPixbuf*    _icons[CARDTREE_COUNT_ICONS];
} cardtree_t;

cardtree_t* cardtree_new();

gboolean cardtree_is_empty(cardtree_t* ct);

const char* cardtree_add_node(cardtree_t* ct,
			      const char* path,
			      const char* name, 
			      const char* id, 
			      int length, 
			      const char* comment);

gboolean cardtree_get_attributes(cardtree_t* ct,
			         const char* path,
				 const char **attribute_names,
				 char **attribute_values);

gboolean cardtree_set_attributes(cardtree_t* ct,
			         const char* path,
				 const char **attribute_names,
				 const char **attribute_values);

gboolean cardtree_set_value(cardtree_t* ct,
			    const char* path,
			    const char* value);

gboolean cardtree_get_value(cardtree_t* ct,
			    const char* path,
                            char** value);

gboolean cardtree_set_alt_value(cardtree_t* ct,
			      	const char* path,
				const char* value);

gboolean cardtree_get_alt_value(cardtree_t* ct,
			      	const char* path,
				char** value);

gboolean cardtree_get_node(cardtree_t* ct, 
			   const char* path,
			   char** name, 
			   char** id, 
			   int *length, 
			   char** comment,
			   int *num_children);

gboolean cardtree_get_flags(cardtree_t* ct,
		 	    const char* path,
	 		    unsigned *flags);

gboolean cardtree_set_flags(cardtree_t* ct,
		    	    const char* path,
	     		    unsigned flags);

gboolean cardtree_delete_node(cardtree_t* ct, 
			      const char *path);

const char* cardtree_find_node(cardtree_t* ct, 
			       const char *root, const char* node, const char *id);

char* cardtree_to_xml(cardtree_t* ct, const char *path);

gboolean cardtree_to_xml_file(cardtree_t* ct, const char *fname, const char* path);

gboolean cardtree_from_xml(cardtree_t *ct, unsigned source_len, const char *source_val);

gboolean cardtree_from_xml_file(cardtree_t *ct, const char *fname);

void cardtree_bind_to_treeview(cardtree_t* ct, GtkWidget *view);

void cardtree_free(cardtree_t* ct);

#endif
