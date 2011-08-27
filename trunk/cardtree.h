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
#include "dyntree_model.h"

enum {
	CC_CLASSNAME = 0,	/* 0 */
	CC_LABEL,
	CC_ID,
	CC_SIZE,
	CC_VAL,
	CC_ALT,    		/* 5 */
	CC_ICON,
	CC_MARKUP_LABEL_ID,
	CC_MARKUP_VAL,
	CC_MARKUP_ALT,		
	CC_INITIAL_COUNT	/* 10 */
};

#define CARDTREE_COUNT_ICONS 6

#define C_FLAG_DISPLAY_FULL 'F'

typedef struct {
  DyntreeModel* _store;
} cardtree_t;

cardtree_t* cardtree_new();

gboolean cardtree_node_append(cardtree_t *ct, 
		GtkTreeIter *child,
		GtkTreeIter *parent,
		const char *type,
		const char *label,
		const char *id,
		const char *size);

gboolean cardtree_node_remove(cardtree_t *ct,
		GtkTreeIter *iter);


gboolean cardtree_attribute_set(cardtree_t* ct,
				GtkTreeIter *iter,
                            	int index,
                            	const char *str);

gboolean cardtree_attribute_get(cardtree_t* ct,
				GtkTreeIter *iter,
                                int index,
                                const char **ptr);

gboolean cardtree_attribute_set_by_name(cardtree_t* ct,
				GtkTreeIter *iter,
                            	const char *name,
                            	const char *str);

gboolean cardtree_attribute_get_by_name(cardtree_t* ct,
				GtkTreeIter *iter,
                                const char* name,
                                const char **ptr);

gboolean cardtree_find_next(cardtree_t* ct,
	        GtkTreeIter *result,	
		GtkTreeIter *root, 
		const char *t_label, 
		const char *t_id);

gboolean cardtree_find_first(cardtree_t* ct,
		GtkTreeIter *result,	
		GtkTreeIter *root, 
		const char *t_label, 
		const char *t_id);

char* cardtree_to_xml(cardtree_t* ct, 
		GtkTreeIter *root);
		/* result must be free'd with g_free */
		
gboolean cardtree_to_xml_file(cardtree_t* ct, 
		GtkTreeIter *root, 
		const char *fname);

gboolean cardtree_from_xml(cardtree_t *ct,
		GtkTreeIter *parent,	
		const char *source_text);

gboolean cardtree_from_xml_file(cardtree_t *ct, 
		GtkTreeIter *parent,	
		const char *fname);

void cardtree_bind_to_treeview(cardtree_t* ct, GtkWidget *view);

void cardtree_free(cardtree_t* ct);


#endif

