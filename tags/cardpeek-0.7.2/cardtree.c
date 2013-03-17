/********************************************************************** 
*
* This file is part of Cardpeek, the smartcard reader utility.
*
* Copyright 2009-2011 by 'L1L1'
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

#define FG_COLOR1 "#2F2F7F"
#define FG_COLOR2 "#2F2FFF"

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
	dyntree_model_column_register(ct->_store,"-icon");
	dyntree_model_column_register(ct->_store,"-markup-label-id");
	dyntree_model_column_register(ct->_store,"-markup-val");
	dyntree_model_column_register(ct->_store,"-markup-alt");
	dyntree_model_column_register(ct->_store,"-markup-alt");
	g_assert((ct->_store)->n_columns == CC_INITIAL_COUNT);

	return ct;
}

static void internal_cardtree_create_icon_markup_label_id(DyntreeModel *dm, GtkTreeIter* iter)
{
	a_string_t *markup_label_id;
	const char *classname;
	const char *id;
	const char *label;
	const char *icon_name = NULL;

	dyntree_model_iter_attributes_get(dm,
			iter,
			CC_CLASSNAME, &classname,
			CC_LABEL, &label,
			CC_ID, &id,
			-1);

	switch (classname[0]) {
		case 'c': if (strcmp("card",classname)==0)        icon_name="cardpeek-smartcard";   break;
		case 'a': if (strcmp("application",classname)==0) icon_name="cardpeek-application"; break;
		case 'f': if (strcmp("file",classname)==0)        icon_name="cardpeek-file";        break;
		case 'r': if (strcmp("record",classname)==0)      icon_name="cardpeek-record";      break;
		case 'b': if (strcmp("block",classname)==0)       icon_name="cardpeek-block";       break;
		default: icon_name="cardpeek-item";	
	}

	markup_label_id = a_strnew(NULL);

	if (label) 
		a_sprintf(markup_label_id,"<b>%s</b>",label);
	else
		a_sprintf(markup_label_id,"<b>%s</b>",classname);

	if (id)
	{
		a_strcat(markup_label_id," ");
		a_strcat(markup_label_id,id);
	}

	dyntree_model_iter_attributes_set(dm, iter,
			CC_ICON, icon_name,
			CC_MARKUP_LABEL_ID, a_strval(markup_label_id),
			-1);

	a_strfree(markup_label_id);
}

static char *internal_cardtree_create_markup_text(const char* src, int full, int is_bytes)
{
	a_string_t *markup_value;
	int linepos,i,len,max,offset;

	len=strlen(src);

	if (is_bytes && len>=2 && src[1]==':')
	{
		offset=2;
		if (len==2)
			return g_strdup("");
	}
	else
	{
		is_bytes=0;
		offset=0;
	}

	markup_value   = a_strnew(NULL);
	if (!is_bytes)
		a_sprintf(markup_value,"<span foreground=\"%s\">&gt; </span>",FG_COLOR2);
	a_strcat(markup_value,"<tt>");

	i=0;
	linepos=0;
	if (full)
		max=len;
	else
		max=((256+offset)<len?(256+offset):len);
	while (i+offset<max)
	{
		switch (src[i+offset]) {
			case '<': 
				a_strcat(markup_value,"&lt;"); break;
			case '>':
				a_strcat(markup_value,"&gt;"); break;
			case '&':
				a_strcat(markup_value,"&amp;"); break;
			case '\n':
				linepos=0;
				a_strpushback(markup_value,'\n'); break;
			default:
				a_strpushback(markup_value,src[i+offset]);
		}
		i++;
		if ((++linepos==64) && (i+1<len)) { 
			a_strpushback(markup_value,'\n');
			linepos=0;
		}
	}
	a_strcat(markup_value,"</tt>");
	if (len>256 && !full) a_strcat(markup_value,"<b>[...]</b>");

	if (is_bytes)
	{
		switch(src[0]) {
			case '8': a_strcat(markup_value,"<span foreground=\"" FG_COLOR2 "\">h</span>"); 
				  break;
			case '4': a_strcat(markup_value,"<span foreground=\"" FG_COLOR2 "\">q</span>"); 
				  break;
			case '1': a_strcat(markup_value,"<span foreground=\"" FG_COLOR2 "\">b</span>"); 
				  break;
		}
	}

	return a_strfinalize(markup_value);
}

static void internal_cardtree_create_markup_val(DyntreeModel *dm, GtkTreeIter* iter)
{
	const char *value;
	char *markup;

	dyntree_model_iter_attributes_get(dm,iter,
			CC_VAL, &value,
			-1);

	if (value)  
	{
		markup = internal_cardtree_create_markup_text(value,1 ,1);
		dyntree_model_iter_attribute_set (dm, iter, CC_MARKUP_VAL, markup);
		free(markup);
	}
	else
	{
		dyntree_model_iter_attribute_set (dm, iter, CC_MARKUP_VAL, NULL);
	}
}

static void internal_cardtree_create_markup_alt(DyntreeModel *dm, GtkTreeIter* iter)
{
	char *markup;
	const char *value;
	const char *color;
	int is_bytes;

	dyntree_model_iter_attributes_get(dm,iter,
			CC_ALT, &value,
			-1);

	if (!value) /* fallback */
	{
		dyntree_model_iter_attribute_get(dm,iter,CC_VAL, &value);
		color=NULL;
		is_bytes=1;
	}
	else
	{
		color="#2F2F7F";
		is_bytes=0;
	}

	if (value)  
	{
		markup = internal_cardtree_create_markup_text(value,is_bytes^1,is_bytes);
		dyntree_model_iter_attribute_set (dm, iter, CC_MARKUP_ALT, markup);
		free(markup);
	}
	else
	{
		dyntree_model_iter_attribute_set (dm, iter, CC_MARKUP_ALT, NULL);
	}
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

	internal_cardtree_create_icon_markup_label_id(ct->_store,child);

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

	switch (c_index) {
		case CC_ID:
		case CC_LABEL:
		case CC_CLASSNAME:
			internal_cardtree_create_icon_markup_label_id(ct->_store,iter);
			break;
		case CC_VAL:
			internal_cardtree_create_markup_val(ct->_store,iter);
			internal_cardtree_create_markup_alt(ct->_store,iter);
			break;
		case CC_ALT:
			internal_cardtree_create_markup_alt(ct->_store,iter);
			break;
	}
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
 
static gboolean internal_node_update_func(DyntreeModel *dm, GtkTreeIter *iter, gpointer user_data)
{
	internal_cardtree_create_icon_markup_label_id(dm,iter);
	internal_cardtree_create_markup_val(dm,iter);
	internal_cardtree_create_markup_alt(dm,iter);
	return TRUE;
}

gboolean cardtree_from_xml(cardtree_t *ct, GtkTreeIter *parent, const char *source_text)
{
	gboolean r = dyntree_model_iter_from_xml(ct->_store,parent,NULL,source_text,-1);
	dyntree_model_foreach(ct->_store,parent,internal_node_update_func,NULL);
	return r;
}

gboolean cardtree_from_xml_file(cardtree_t *ct, GtkTreeIter* parent, const char *fname)
{
	gboolean r = dyntree_model_iter_from_xml_file(ct->_store,parent,"cardtree",fname);
	dyntree_model_foreach(ct->_store,parent,internal_node_update_func,NULL);
	return r;
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
	/* int i; */
	g_object_unref(ct->_store); 
	/* for (i=0;i<CARDTREE_COUNT_ICONS;i++)
		g_object_unref(ct->_icons[i]); */
	g_free(ct);
}

   
