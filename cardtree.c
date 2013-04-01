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

#include <gtk/gtk.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h>
#include <unistd.h>
#include "misc.h"
#include "cardtree.h"

cardtree_t* cardtree_new(void)
{
	cardtree_t* ct = g_new0(cardtree_t,1);

	g_assert(ct != NULL);

	ct->_store = dyntree_model_new();  
	
	dyntree_model_column_register(ct->_store,"classname");
	dyntree_model_column_register(ct->_store,"label");
	dyntree_model_column_register(ct->_store,"id");
	dyntree_model_column_register(ct->_store,"size"); 
	dyntree_model_column_register(ct->_store,"val");
	dyntree_model_column_register(ct->_store,"alt");
	g_assert((ct->_store)->n_columns == CC_INITIAL_COUNT);

	return ct;
}

gboolean cardtree_node_append(cardtree_t* ct, 
		GtkTreeIter *child, 
		GtkTreeIter *parent,
		const char *classname,
		const char *label,
		const char *id,
		const char *size)
{
	if (classname==NULL)
		classname = "item";


	dyntree_model_iter_append(ct->_store,child,parent);

	dyntree_model_iter_attributes_set(ct->_store,child,
			CC_CLASSNAME, classname,
			CC_LABEL, label,
			CC_ID, id,
			CC_SIZE, size,
			-1);

	return TRUE;
}

gboolean cardtree_node_remove(cardtree_t* ct, GtkTreeIter *iter)
{
	return dyntree_model_iter_remove(ct->_store,iter);
}


gboolean cardtree_node_child(cardtree_t *ct,
		GtkTreeIter *iter,
		GtkTreeIter *parent)
{
	return gtk_tree_model_iter_children(GTK_TREE_MODEL(ct->_store),
			iter,
			parent);
}

gboolean cardtree_node_next(cardtree_t *ct,
		GtkTreeIter *iter)
{
	return gtk_tree_model_iter_next(GTK_TREE_MODEL(ct->_store),
			iter);
}

gboolean cardtree_node_parent(cardtree_t *ct,
		GtkTreeIter *parent,
		GtkTreeIter *child)
{
	return gtk_tree_model_iter_parent(GTK_TREE_MODEL(ct->_store),
			parent,
			child);
}


gint cardtree_attribute_count(cardtree_t* ct)
{
	return dyntree_model_get_n_columns(GTK_TREE_MODEL(ct->_store));
}

const char *cardtree_attribute_name(cardtree_t* ct, int c_index)
{
	return dyntree_model_column_index_to_name(ct->_store, c_index);
}


gboolean cardtree_attribute_set(cardtree_t* ct,
				GtkTreeIter *iter,
                            	int c_index,
                            	const char *str)
{
	if (c_index<0)
	       return FALSE;

	if (dyntree_model_iter_attribute_set(ct->_store,iter,c_index,str)==FALSE)
		return FALSE;

	return TRUE;
}


gboolean cardtree_attribute_set_by_name(cardtree_t* ct,
				GtkTreeIter *iter,
                            	const char *name,
                            	const char *str)
{
	int c_index = dyntree_model_column_name_to_index(ct->_store,name);

	return cardtree_attribute_set(ct,iter,c_index,str);
}

gboolean cardtree_attribute_get(cardtree_t* ct, 
				GtkTreeIter *iter,
                            	int c_index,
                            	const char **str)
{
	return dyntree_model_iter_attribute_get(ct->_store,iter,c_index,str);
}

gboolean cardtree_attribute_get_by_name(cardtree_t* ct, 
				GtkTreeIter *iter,
                            	const char *name,
                            	const char **str)
{
	return dyntree_model_iter_attribute_get_by_name(ct->_store,iter,name,str);
}

char *cardtree_to_xml(cardtree_t* ct, GtkTreeIter *root)
{
	return dyntree_model_iter_to_xml(ct->_store,root,NULL);
}

gboolean cardtree_to_xml_file(cardtree_t* ct, GtkTreeIter *root, const char *fname)
{
	return dyntree_model_iter_to_xml_file(ct->_store,root,"cardtree",fname);
}
 
gboolean cardtree_from_xml(cardtree_t *ct, GtkTreeIter *parent, const char *source_text)
{
	return dyntree_model_iter_from_xml(ct->_store,parent,NULL,source_text,-1);
}

gboolean cardtree_from_xml_file(cardtree_t *ct, GtkTreeIter* parent, const char *fname)
{
	return dyntree_model_iter_from_xml_file(ct->_store,parent,"cardtree",fname);
}


gboolean cardtree_find_next(cardtree_t* ct, 
			    GtkTreeIter* result, 
		            GtkTreeIter* root,
			    const char* t_label, 
			    const char *t_id)
{
	int indices[2];
	const char *values[2];
	int n_values = 0;

	if (t_label)
	{
		indices[n_values]=CC_LABEL;
		values[n_values]=t_label;
		n_values++;
	}
	if (t_id)
	{
		indices[n_values]=CC_ID;
		values[n_values]=t_id;
		n_values++;
	}
	return dyntree_model_iter_find_next(ct->_store,result,root,indices,values,n_values);
}

gboolean cardtree_find_first(cardtree_t* ct, 
			  GtkTreeIter *result,
			  GtkTreeIter *root, 
			  const char *t_label, 
			  const char *t_id)
{
	int indices[2];
	const char *values[2];
	int n_values = 0;

	if (t_label)
	{
		indices[n_values]=CC_LABEL;
		values[n_values]=t_label;
		n_values++;
	}
	if (t_id)
	{
		indices[n_values]=CC_ID;
		values[n_values]=t_id;
		n_values++;
	}
	return dyntree_model_iter_find_first(ct->_store,result,root,indices,values,n_values);
}

void cardtree_bind_to_treeview(cardtree_t* ct, GtkWidget *view)
{
	gtk_tree_view_set_model(GTK_TREE_VIEW(view),GTK_TREE_MODEL(ct->_store));
}

void cardtree_free(cardtree_t* ct)
{
	g_object_unref(ct->_store); 
	g_free(ct);
}

   
