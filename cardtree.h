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
  C_CLASSNAME,
  C_DESCRIPTION,
  C_ID,
  C_SIZE,
  /* value data */
  C_VALUE,
  C_ALT_VALUE,
  /* extra helper columns */
  C_MARKUP_CLASSNAME_DESCRIPTION_ID,
  C_MARKUP_VALUE,
  C_MARKUP_ALT_VALUE,
  C_ICON,
  C_FLAGS,
  NUM_COLS
};

#define CARDTREE_COUNT_ICONS 6

#define C_FLAG_DISPLAY_FULL 1

typedef struct {
  GtkTreeStore* _store;
  GdkPixbuf*    _icons[CARDTREE_COUNT_ICONS];
} cardtree_t;

cardtree_t* cardtree_new();

gboolean cardtree_is_empty(cardtree_t *ct);

char *cardtree_node_add(cardtree_t *ct,
		const char 	*path,
		const char 	*classname,
		const char 	*description, 
		const char 	*id, 
		unsigned 	size);
		/* result (char *) must be freed with cardtree_path_free() */

gboolean cardtree_node_modify(cardtree_t *ct,
		const char 	*path,
		const char 	*classname,
		const char 	*description, 
		const char 	*id, 
		unsigned 	size);

gboolean cardtree_node_get(cardtree_t *ct, 
		const char 	*path,
		char 		**classname,
		char 		**description, 
		char 		**id, 
		unsigned 	*size, 
		unsigned 	*num_children);

gboolean cardtree_node_delete(cardtree_t* ct, 
                              const char *path);


gboolean cardtree_value_set(cardtree_t* ct,
                            const char* path,
                            const char* value);

gboolean cardtree_value_get(cardtree_t* ct,
                            const char* path,
                            char** value);

gboolean cardtree_alt_value_set(cardtree_t* ct,
                                const char* path,
                                const char* value);

gboolean cardtree_alt_value_get(cardtree_t* ct,
                                const char* path,
                                char** value);

gboolean cardtree_flags_get(cardtree_t* ct,
                            const char* path,
                            unsigned *flags);

gboolean cardtree_flags_set(cardtree_t* ct,
                            const char* path,
			    unsigned flags);

gboolean cardtree_value_strlen(cardtree_t* ct,
                               const char *path,
                               unsigned *len);

char* cardtree_find_node(cardtree_t* ct, 
		const char *root, const char* node, const char *id);
		/* result (char*) must be freed with cardtree_path_free() */

char* cardtree_find_all_nodes(cardtree_t* ct, 
		const char *root, const char* node, const char *id);
		/* result (char*) must be freed with cardtree_path_free() */

char* cardtree_to_xml(cardtree_t* ct, const char *path);
		/* result (char*) must be freed with cardtree_path_free() */

gboolean cardtree_to_xml_file(cardtree_t* ct, const char *fname, const char* path);

gboolean cardtree_from_xml(cardtree_t *ct, unsigned source_len, const char *source_val);

gboolean cardtree_from_xml_file(cardtree_t *ct, const char *fname);

void cardtree_bind_to_treeview(cardtree_t* ct, GtkWidget *view);

void cardtree_free(cardtree_t* ct);

gboolean cardtree_attribute_set(cardtree_t *ct,
    			        const char      *path,
				const char	*aname,
				const char	*avalue);

gboolean cardtree_attribute_get(cardtree_t *ct,
    			        const char      *path,
				const char	*aname,
				char		**avalue);

/**** extras ****/

char *cardtree_path_from_gtktreepath(GtkTreePath *path);

char *cardtree_path_from_iter(cardtree_t *ct, GtkTreeIter *iter);

gboolean cardtree_path_to_iter(cardtree_t *ct, const char *path, GtkTreeIter *iter);

void cardtree_path_free(char *path);

gboolean cardtree_path_is_valid(const char *path);

#endif

