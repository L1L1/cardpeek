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

cardtree_t* cardtree_new()
{
	cardtree_t* ct = (cardtree_t*)g_malloc(sizeof(cardtree_t));

	ct->_store = gtk_tree_store_new(NUM_COLS,
			G_TYPE_STRING, /* classname 		*/
			G_TYPE_STRING, /* description		*/
			G_TYPE_STRING, /* id    		*/
			G_TYPE_UINT,   /* size  		*/
			G_TYPE_STRING, /* value 		*/
			G_TYPE_STRING, /* alt 			*/
			G_TYPE_STRING, /* markup class desc id	*/
			G_TYPE_STRING, /* markup value 		*/
			G_TYPE_STRING, /* markup alt 		*/
			G_TYPE_STRING, /* icon 			*/
			G_TYPE_UINT);  /* internal flags 	*/

	return ct;
}

gboolean cardtree_is_empty(cardtree_t* ct)
{
	GtkTreeIter iter;
	return gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ct->_store),&iter)==FALSE;
}

void cardtree_create_markup_classname_description_id(cardtree_t* ct, GtkTreeIter* iter)
{
	a_string_t *markup_class_desc_id;
	char *classname;
	char *id;
	char *description;
	char *icon_name = NULL;

	gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),iter,
			C_CLASSNAME, &classname,
			C_DESCRIPTION, &description,
			C_ID, &id,
			-1);


	switch (classname[0]) {
		case 'c': if (strcmp("card",classname)==0)        icon_name="cardpeek-smartcard";   break;
		case 'a': if (strcmp("application",classname)==0) icon_name="cardpeek-application"; break;
		case 'f': if (strcmp("file",classname)==0)        icon_name="cardpeek-file";        break;
		case 'r': if (strcmp("record",classname)==0)      icon_name="cardpeek-record";      break;
		case 'b': if (strcmp("block",classname)==0)       icon_name="cardpeek-block";       break;
		default: icon_name="cardpeek-item";	
	}

	markup_class_desc_id = a_strnew(NULL);

	if (description) 
		a_sprintf(markup_class_desc_id,"<b>%s</b>",description);
	else
		a_sprintf(markup_class_desc_id,"<b>%s</b>",classname);

	if (id)
	{
		a_strcat(markup_class_desc_id," ");
		a_strcat(markup_class_desc_id,id);
	}

	gtk_tree_store_set (ct->_store, iter,
			C_ICON, icon_name,
			C_MARKUP_CLASSNAME_DESCRIPTION_ID, a_strval(markup_class_desc_id),
			-1);

	if (classname) g_free(classname);
	if (description) g_free(description);
	if (id) g_free(id);
	a_strfree(markup_class_desc_id);
}

char *cardtree_create_markup_text(const char* src, int full, int is_bytes)
{
	a_string_t *markup_value;
	int linepos,i,len,max,offset;

	len=strlen(src);

	if (is_bytes && len>2 && src[1]==':')
	{
		offset=2;
	}
	else
	{
		is_bytes=0;
		offset=0;
	}

	markup_value   = a_strnew(NULL);
	if (!is_bytes)
		a_sprintf(markup_value,"<span foreground=\"%s\">",FG_COLOR1);
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

	if (!is_bytes)
		a_strcat(markup_value,"</span>");
	else
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

void cardtree_create_markup_value(cardtree_t* ct, GtkTreeIter* iter)
{
	unsigned flags;
	char *value;
	char *markup;

	gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),iter,
			C_VALUE, &value,
			C_FLAGS, &flags,
			-1);

	if (value)  
	{
		markup = cardtree_create_markup_text(value,flags & C_FLAG_DISPLAY_FULL,1);
		gtk_tree_store_set (ct->_store, iter,
				C_MARKUP_VALUE, markup,
				-1);
		g_free(value);
		free(markup);
	}
	else
	{
		gtk_tree_store_set (ct->_store, iter,
				C_MARKUP_VALUE, NULL,
				-1);
	}
}

void cardtree_create_markup_alt_value(cardtree_t* ct, GtkTreeIter* iter)
{
	char *markup;
	unsigned flags;
	char *value;
	char *color;
	int is_bytes;

	gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),iter,
			C_ALT_VALUE, &value,
			C_FLAGS, &flags,
			-1);

	if (!value) /* fallback */
	{
		gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),iter,
				C_VALUE, &value,
				-1);
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
		markup = cardtree_create_markup_text(value,flags & C_FLAG_DISPLAY_FULL,is_bytes);
		gtk_tree_store_set (ct->_store, iter,
				C_MARKUP_ALT_VALUE, markup,
				-1);
		g_free(value);
		free(markup);
	}
	else
	{
		gtk_tree_store_set (ct->_store, iter,
				C_MARKUP_ALT_VALUE, NULL,
				-1);
	}
}


char* cardtree_node_add(cardtree_t* ct,
		const char 	*path,
		const char 	*classname,
		const char 	*description, 
		const char 	*id, 
		unsigned 	size)
{
	GtkTreeIter parent;
	GtkTreeIter child;
	
	if (path!=NULL && cardtree_is_empty(ct))
		return NULL;

	if (classname==NULL)
		return NULL;

	if (path!=NULL)
	{
		if (cardtree_path_to_iter(ct,path,&parent)==FALSE)
			return NULL;
		gtk_tree_store_set (ct->_store, &parent,
				C_VALUE,NULL,
				C_MARKUP_VALUE, NULL,
				-1);
		gtk_tree_store_append (ct->_store, &child, &parent);
	}
	else
	{
		gtk_tree_store_append (ct->_store, &child, NULL);
	}

	gtk_tree_store_set (ct->_store, &child, 
			C_CLASSNAME, classname, 
			C_DESCRIPTION, description, 
			C_ID, id, 
			C_SIZE, size,
			C_VALUE,NULL,
			C_MARKUP_VALUE, NULL,
			-1);

	cardtree_create_markup_classname_description_id(ct,&child);

	return cardtree_path_from_iter(ct,&child);
}

gboolean cardtree_node_modify(cardtree_t* ct,
		const char 	*path,
		const char 	*classname,
		const char 	*description, 
		const char 	*id, 
		unsigned 	size)

{
	GtkTreeIter iter;

	if (!cardtree_path_is_valid(path) || cardtree_is_empty(ct))
		return FALSE;

	if (cardtree_path_to_iter(ct,path,&iter)==FALSE)
		return FALSE;

	gtk_tree_store_set (ct->_store, &iter, 
			C_CLASSNAME, classname, 
			C_DESCRIPTION, description, 
			C_ID, id, 
			C_SIZE, size,
			-1); 

	cardtree_create_markup_classname_description_id(ct,&iter);
	return TRUE;
}


gboolean cardtree_node_get(cardtree_t* ct, 
		const char	*path,
		char 		**classname,
		char 		**description,	 
		char 		**id, 
		unsigned 	*size, 
		unsigned 	*num_children)
{
	GtkTreeIter iter;

	if (!cardtree_path_is_valid(path) || cardtree_is_empty(ct))
		return FALSE;

	if (cardtree_path_to_iter(ct,path,&iter)==FALSE)
		return FALSE;

	if (classname!=NULL)
		gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_CLASSNAME,classname,-1);

	if (description!=NULL)
		gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_DESCRIPTION,description,-1);

	if (id!=NULL)
		gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_ID,id,-1);

	if (size!=NULL)
		gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_SIZE,size,-1);

	if (num_children!=NULL)
		*num_children = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(ct->_store),&iter);

	return TRUE;
}

gboolean cardtree_node_delete(cardtree_t* ct, const char *path)
{
	GtkTreeIter iter;

	if (path==NULL && cardtree_is_empty(ct))
		return FALSE;

	if (path==NULL)
	{
		gtk_tree_store_clear(ct->_store);
		return TRUE;
	}  

	if (cardtree_path_to_iter(ct,path,&iter)==FALSE)
		return FALSE;
	return gtk_tree_store_remove(ct->_store,&iter);
}


gboolean cardtree_value_set(cardtree_t* ct,
                            const char* path,
                            const char* value)
{
	GtkTreeIter iter;

	if (!cardtree_path_is_valid(path) || cardtree_is_empty(ct))
		return FALSE;

	if (cardtree_path_to_iter(ct,path,&iter)==FALSE)
		return FALSE;
/*
	if (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(ct->_store),&iter))
		return FALSE;
*/
	gtk_tree_store_set (ct->_store, &iter, 
			C_VALUE, value, 
			C_FLAGS, 0,
			-1);

	cardtree_create_markup_value(ct,&iter);
	cardtree_create_markup_alt_value(ct,&iter);
	return TRUE;
}

gboolean cardtree_value_get(cardtree_t* ct,
                            const char* path,
                            char** value)
{
	GtkTreeIter iter;

	if (!cardtree_path_is_valid(path) || cardtree_is_empty(ct))
		return FALSE;

	if (cardtree_path_to_iter(ct,path,&iter)==FALSE)
		return FALSE;

	gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_VALUE,value,-1);

	return TRUE;
}

gboolean cardtree_alt_value_set(cardtree_t* ct,
			      	const char* path,
				const char* value)
{
	GtkTreeIter iter;

	if (!cardtree_path_is_valid(path) || cardtree_is_empty(ct))
		return FALSE;

	if (cardtree_path_to_iter(ct,path,&iter)==FALSE)
		return FALSE;
/*
	if (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(ct->_store),&iter))
		return FALSE;
*/
	gtk_tree_store_set (ct->_store, &iter, 
			C_ALT_VALUE, value, 
			-1);

	cardtree_create_markup_alt_value(ct,&iter);
	return TRUE;
}


gboolean cardtree_alt_value_get(cardtree_t* ct,
			      	const char* path,
				char** value)
{
	GtkTreeIter iter;

	if (!cardtree_path_is_valid(path) || cardtree_is_empty(ct))
		return FALSE;

	if (cardtree_path_to_iter(ct,path,&iter)==FALSE)
		return FALSE;

	gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,
			C_ALT_VALUE,value,
			-1);

	return TRUE;
}

gboolean cardtree_flags_get(cardtree_t* ct,
                            const char *path,
                            unsigned *flags)
{
	GtkTreeIter iter;

	if (!cardtree_path_is_valid(path) || cardtree_is_empty(ct))
		return FALSE;

	if (cardtree_path_to_iter(ct,path,&iter)==FALSE)
		return FALSE;

	gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_FLAGS,flags,-1);

	return TRUE;
}

gboolean cardtree_flags_set(cardtree_t* ct,
                            const char *path,
                            unsigned flags)
{
	GtkTreeIter iter;

	if (!cardtree_path_is_valid(path) || cardtree_is_empty(ct))
		return FALSE;

	if (cardtree_path_to_iter(ct,path,&iter)==FALSE)
		return FALSE;

	gtk_tree_store_set (ct->_store, &iter, C_FLAGS, flags, -1);
	cardtree_create_markup_value(ct,&iter);
	cardtree_create_markup_alt_value(ct,&iter);

	return TRUE;
}

gboolean cardtree_value_strlen(cardtree_t* ct,
                            const char *path,
                            unsigned *len)
{
	char *val;

	if (cardtree_value_get(ct,path,&val)==FALSE)
		return FALSE;

	if (val)
	{
		*len = strlen(val)-2;
	}
	else
		*len = 0;

	return TRUE;
}

gboolean node_to_xml(a_string_t* res, GtkTreeModel *store, GtkTreeIter* iter, int depth)
{
	int i;
	gchar *classname;
	gchar *description;
	gchar *id;
	unsigned size; 
	char str_size[20];
	gchar *value;
	gchar *alt_value;
	GtkTreeIter child;
	gchar* esc_alt_value;

	do {
		gtk_tree_model_get(store,iter,
				C_CLASSNAME,&classname,
				C_DESCRIPTION,&description,
				C_ID,&id,
				C_SIZE,&size,
				C_VALUE,&value,
				C_ALT_VALUE,&alt_value,
				-1);
		for(i=0;i<depth;i++) a_strcat(res,"  ");

		a_strcat(res,"<node class=\"");
		a_strcat(res,classname);
		a_strcat(res,"\"");

		if (description)
		{
			a_strcat(res," description=\"");
			a_strcat(res,description);
			a_strcat(res,"\"");
		}
		if (id)
		{
			a_strcat(res," id=\"");
			a_strcat(res,id);
			a_strcat(res,"\"");
		}
		if (size)
		{
			sprintf(str_size," size=\"%i\"",size);
			a_strcat(res,str_size);
		}

		a_strcat(res,">\n");
		
		if (value)
		{
			for(i=0;i<=depth;i++) a_strcat(res,"  ");	
			a_strcat(res,"<val>");
			a_strcat(res,value);
			a_strcat(res,"</val>\n");
			if (alt_value)
			{
				for(i=0;i<=depth;i++) a_strcat(res,"  ");	
				a_strcat(res,"<alt>");
				esc_alt_value = g_markup_escape_text(alt_value,-1);
				a_strcat(res,esc_alt_value);
				g_free(esc_alt_value);
				a_strcat(res,"</alt>\n");
			}
		}

		if (gtk_tree_model_iter_children(store,&child,iter))
		{
			node_to_xml(res,store,&child,depth+1);
			for(i=0;i<depth;i++) a_strcat(res,"  ");
			a_strcat(res,"</node>\n");
		}

		for(i=0;i<depth;i++) a_strcat(res,"  ");	
		a_strcat(res,"</node>\n");


		if (classname) g_free(classname);
		if (description) g_free(description);
		if (id) g_free(id);
		if (value) g_free(value);
		if (alt_value) g_free(alt_value);
	} while (gtk_tree_model_iter_next(store,iter));
	return TRUE;
}

char* cardtree_to_xml(cardtree_t* ct, const char *path)
{
	a_string_t *res;
	GtkTreeIter iter;
	gboolean path_exists;

	if (path==NULL)
		path_exists = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ct->_store),&iter);
	else
		path_exists = gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),&iter,path+1);

	if (!path_exists)
		return NULL;

	res = a_strnew("<?xml version=\"1.0\"?>\n");
	a_strcat(res,"<cardtree>\n");
	node_to_xml(res,GTK_TREE_MODEL(ct->_store),&iter,1);
	a_strcat(res,"</cardtree>\n");
	return a_strfinalize(res);
}

gboolean cardtree_to_xml_file(cardtree_t* ct, const char *fname, const char* path)
{
	FILE *save;
	char *xml;
	gboolean retval;

	if ((save=fopen(fname,"w"))==NULL)
	{
		log_printf(LOG_ERROR,"Could not open '%s' for output (%s)",fname,strerror(errno));
		return FALSE;
	}
	xml = cardtree_to_xml(ct,path);

	if (fwrite(xml,strlen(xml),1,save)!=1)
	{
		log_printf(LOG_ERROR,"Output error on '%s' (%s)",fname,strerror(errno));
		retval = FALSE;
	}
	else
		retval = TRUE;
	free(xml);
	fclose(save);
	return retval;
}
 
enum {
  ND_NONE,
  ND_CARDPEEK,
  ND_NODE,
  ND_VAL,
  ND_ALT
};

char *ND_STATE[5] = {
  "root",
  "<cardpeek>",
  "<node>",
  "<val>",
  "<alt>"
};


typedef struct {
  cardtree_t   *ctx_tree;
  GtkTreeIter   ctx_node;
  int           ctx_state;
} xml_context_data_t;

gboolean cardtree_set_attribute_iter(cardtree_t *ct, GtkTreeIter *iter, const gchar *aname, const gchar *avalue)
{
	switch (aname[0]) {
		case 'c': if (strcmp(aname,"class")==0)
				  gtk_tree_store_set (ct->_store, iter,
						  C_CLASSNAME, avalue,
						  -1);
			  break;
		case 'd': if (strcmp(aname,"description")==0)
				  gtk_tree_store_set (ct->_store, iter,
						  C_DESCRIPTION, avalue,
						  -1);
			  break;
		case 'i': if (strcmp(aname,"id")==0)
				  gtk_tree_store_set (ct->_store, iter,
						  C_ID, avalue,
						  -1);
			  break;
		case 's': if (strcmp(aname,"size")==0)
				  gtk_tree_store_set (ct->_store, iter,
						  C_SIZE, (unsigned)atoi(avalue),
						  -1);
			  break;
		default: return FALSE;
	}
	return TRUE;
}

gboolean cardtree_get_attribute_iter(cardtree_t *ct, GtkTreeIter *iter, const gchar *aname, gchar **avalue)
{
	char size;

	switch (aname[0]) {
		case 'c': if (strcmp(aname,"class")==0)
				  gtk_tree_model_get (GTK_TREE_MODEL(ct->_store), iter,
						  C_CLASSNAME, avalue,
						  -1);
			  break;
		case 'd': if (strcmp(aname,"description")==0)
				  gtk_tree_model_get (GTK_TREE_MODEL(ct->_store), iter,
						  C_DESCRIPTION, avalue,
						  -1);
			  break;
		case 'i': if (strcmp(aname,"id")==0)
				  gtk_tree_model_get (GTK_TREE_MODEL(ct->_store), iter,
						  C_ID, avalue,
						  -1);
			  break;
		case 's': if (strcmp(aname,"size")==0)
			  {
				  gtk_tree_model_get (GTK_TREE_MODEL(ct->_store), iter,
						  C_SIZE, &size,
						  -1);
				  *avalue = (gchar*)malloc(12);
				  sprintf(*avalue,"%i",size);
			  }
			  break;
		default: return FALSE;
	}
	return TRUE;
}

void xml_start_element_cb  (GMarkupParseContext *context,
			    const gchar         *element_name,
		 	    const gchar        **attribute_names,
			    const gchar        **attribute_values,
			    gpointer             user_data,
			    GError             **error)
{
	xml_context_data_t *ctx = (xml_context_data_t *)user_data;
	GtkTreeIter child;
	int i;
	int line_number;
	int char_number;

	g_markup_parse_context_get_position(context,&line_number,&char_number);

	if (strcmp(element_name,"node")==0)
	{
		if (ctx->ctx_state==ND_CARDPEEK)
			gtk_tree_store_append ((ctx->ctx_tree)->_store, &child, NULL);
		else if (ctx->ctx_state==ND_NODE)
			gtk_tree_store_append ((ctx->ctx_tree)->_store, &child, &ctx->ctx_node);
		else
		{
			g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT,
					"Error on line %i[%i]: unexpected <node> in %s",
					line_number,char_number,ND_STATE[ctx->ctx_state]);
			return;
		}

		for (i=0;attribute_names[i]!=NULL;i++)
		{
			if (cardtree_set_attribute_iter(ctx->ctx_tree,&child,attribute_names[i],attribute_values[i])==FALSE)
			{
				g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT,
						"Error on line %i[%i]: unexpected attribute '%s' in node",
						line_number,char_number,attribute_names[i]);
				return;
			}
		}
		cardtree_create_markup_classname_description_id(ctx->ctx_tree,&child);
		ctx->ctx_node = child;
		ctx->ctx_state=ND_NODE;
	}
	else if (strcmp(element_name,"val")==0)
	{
		if (ctx->ctx_state!=ND_NODE)
		{
			g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT,
					"Error on line %i[%i]: unexpected <val> in %s",
					line_number,char_number,ND_STATE[ctx->ctx_state]);
			return;
		}
		ctx->ctx_state=ND_VAL;
	}
	else if (strcmp(element_name,"alt")==0)
	{
		if (ctx->ctx_state!=ND_NODE)
		{
			g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT,
					"Error on line %i[%i]: unexpected <alt> in %s",
					line_number,char_number,ND_STATE[ctx->ctx_state]);
			return;
		}
		ctx->ctx_state=ND_ALT;
	}
	else if (strcmp(element_name,"cardtree")==0)
	{
		if (ctx->ctx_state!=ND_NONE)
		{
			g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_INVALID_CONTENT,
					"Error on line %i[%i]: unexpected <cardtree> in %s",
					line_number,char_number,ND_STATE[ctx->ctx_state]);
			return;
		}
		ctx->ctx_state=ND_CARDPEEK;
	}
	else /* error */
	{
		g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_UNKNOWN_ELEMENT,
				"Error on line %i[%i]: unrecognized element <%s> in %s",
				line_number,char_number,element_name,ND_STATE[ctx->ctx_state]);
	}
}

void xml_end_element_cb  (GMarkupParseContext *context,
			  const gchar         *element_classname,
			  gpointer             user_data,
			  GError             **error)
{
	xml_context_data_t *ctx = (xml_context_data_t *)user_data;
	GtkTreeIter parent;

	switch (ctx->ctx_state) {
		case ND_NODE:
			if (gtk_tree_model_iter_parent(GTK_TREE_MODEL((ctx->ctx_tree)->_store),&parent,&(ctx->ctx_node))==TRUE)
			{
				ctx->ctx_state=ND_NODE;
				ctx->ctx_node = parent;
			}
			else
				ctx->ctx_state=ND_CARDPEEK;
			break;
		case ND_VAL:
			ctx->ctx_state=ND_NODE;
			break;
		case ND_ALT:
			ctx->ctx_state=ND_NODE;
			break;
		case ND_CARDPEEK:
			ctx->ctx_state=ND_NONE;
			break;
	}
}

void xml_text_cb  (GMarkupParseContext *context,
		   const gchar         *text,
		   gsize                text_len,  
		   gpointer             user_data,
		   GError             **error)
{
	xml_context_data_t *ctx = (xml_context_data_t *)user_data;
	char *value;
	int i;
	int line_number;
	int char_number;

	g_markup_parse_context_get_position(context,&line_number,&char_number);

	value = malloc(text_len+1);
	memcpy(value,text,text_len);
	value[text_len]=0;

	if (ctx->ctx_state==ND_VAL)
	{
		gtk_tree_store_set ((ctx->ctx_tree)->_store, &(ctx->ctx_node),
				C_VALUE, value,
				C_FLAGS, 0,
				-1);
		cardtree_create_markup_value(ctx->ctx_tree,&(ctx->ctx_node));
		cardtree_create_markup_alt_value(ctx->ctx_tree,&(ctx->ctx_node));
	}
	else if (ctx->ctx_state==ND_ALT)
	{
		gtk_tree_store_set ((ctx->ctx_tree)->_store, &(ctx->ctx_node),
				C_ALT_VALUE, value,    
				-1);                                        
		cardtree_create_markup_alt_value(ctx->ctx_tree,&(ctx->ctx_node));
	}
	else 
	{
		for (i=0;i<text_len;i++)
			if (text[i]>' ') 
			{
				g_set_error(error,G_MARKUP_ERROR,G_MARKUP_ERROR_UNKNOWN_ELEMENT,
						"Error on line %i[%i]: unexpected text '%s'",line_number,char_number,value);
				break;
			}
	}
	free(value);
}

void xml_error_cb  (GMarkupParseContext *context,
		    GError              *error,
		    gpointer             user_data)
{
	log_printf(LOG_ERROR,"XML %s",error->message);
}

static GMarkupParser cardtree_parser = 
{
	xml_start_element_cb,
	xml_end_element_cb,
	xml_text_cb,
	NULL,
	xml_error_cb
};


gboolean cardtree_from_xml(cardtree_t *ct, unsigned source_len, const char *source_text)
{
	xml_context_data_t ctx;
	GMarkupParseContext *markup_ctx;
	GError *err = NULL;

	ctx.ctx_tree  = ct;
	ctx.ctx_state = ND_NONE;

	markup_ctx = g_markup_parse_context_new(&cardtree_parser,0,&ctx,NULL);
	if (g_markup_parse_context_parse(markup_ctx,source_text,source_len,&err)==TRUE)
	{
		g_markup_parse_context_end_parse(markup_ctx,&err);
	}

	g_markup_parse_context_free(markup_ctx);

	if (err!=NULL) {
		g_error_free(err);
		return FALSE;
	}
	return TRUE;
}

gboolean cardtree_from_xml_file(cardtree_t *ct, const char *fname)
{
	char *buf_val;
	int  buf_len;
	int  rd_len;
	struct stat st;
	gboolean retval;
	int input;

	gtk_tree_store_clear(ct->_store);
	if (stat(fname,&st)!=0)
	{
		log_printf(LOG_ERROR,"Could not stat '%s' (%s)",fname,strerror(errno));
		return FALSE;
	} 
	if ((input=open(fname,O_RDONLY))<0)
	{
		log_printf(LOG_ERROR,"Could not open '%s' for input (%s)",fname,strerror(errno));
		return FALSE;
	}
	buf_len = st.st_size;
	buf_val = malloc(buf_len);
	if ((rd_len=read(input,buf_val,buf_len))==buf_len)
	{
		if (cardtree_from_xml(ct,buf_len,buf_val)==FALSE)
		{
			retval = FALSE;
			gtk_tree_store_clear(ct->_store);
		}
		else
			retval = TRUE;
	}
	else
	{
		log_printf(LOG_ERROR,"Could not read all data (%i bytes) from %s (%s)",
				buf_len,fname,strerror(errno));
		retval = FALSE;
	}
	free(buf_val);
	close(input);
	return retval;
}

char* cardtree_find_ext(cardtree_t* ct, GtkTreeIter* iter, 
			      const char* t_description, const char *t_id)
{
	gchar* description;
	gchar* id;
	GtkTreeIter child;
	int match;
	char *retval;

	/* check item */
	gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),iter,
			C_DESCRIPTION,&description,
			C_ID,&id,
			-1);

	if ((t_description==NULL || (description && strcmp(t_description,description)==0)) &&
			(t_id==NULL || (id && strcmp(t_id,id)==0)))
		match = 1;
	else
		match = 0;

	if (description) g_free(description);
	if (id) g_free(id);

	if (match) 
		return cardtree_path_from_iter(ct,iter);
	
	/* check children */
	if (gtk_tree_model_iter_children(GTK_TREE_MODEL(ct->_store),&child,iter)!=FALSE)
	{
		do
		{
			retval = cardtree_find_ext(ct,&child,t_description,t_id);
			if (retval) return retval;
		}
		while (gtk_tree_model_iter_next(GTK_TREE_MODEL(ct->_store),&child)!=FALSE);
	}
	return NULL;
}

char* cardtree_find_node(cardtree_t* ct, 
			       const char *root, const char* node, const char *id)
{
	GtkTreeIter iter;

	if (id==NULL && node==NULL)
		return NULL;

	if (!cardtree_path_to_iter(ct,root,&iter))
		return NULL;

	return cardtree_find_ext(ct,&iter,node,id);
}

int cardtree_find_all_ext(a_string_t *res,
			  cardtree_t* ct, GtkTreeIter* iter, 
			  const char* t_description, const char *t_id)
{
	gchar* description;
	gchar* id;
	GtkTreeIter child;
	int match;
	gchar* r;

	/* check item */
	gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),iter,
			C_DESCRIPTION,&description,
			C_ID,&id,
			-1);

	if ((t_description==NULL || (description && strcmp(t_description,description)==0)) &&
			(t_id==NULL || (id && strcmp(t_id,id)==0)))
		match = 1;
	else
		match = 0;
	if (description) g_free(description);
	if (id) g_free(id);

	if (match)
	{
		r = cardtree_path_from_iter(ct,iter);
		a_strcat(res,r);
		a_strcat(res,";");
		g_free(r);
	}
	/* check children */
	if (gtk_tree_model_iter_children(GTK_TREE_MODEL(ct->_store),&child,iter)!=FALSE)
	{
		do
		{
			cardtree_find_all_ext(res,ct,&child,t_description,t_id);
		}
		while (gtk_tree_model_iter_next(GTK_TREE_MODEL(ct->_store),&child)!=FALSE);
	}
	return a_strlen(res);
}


char* cardtree_find_all_nodes(cardtree_t* ct, 
				    const char *root, const char* node, const char *id)
{
	GtkTreeIter iter;
	a_string_t *res = a_strnew(NULL);
	char *s;
	char *r;

	if (id==NULL && node==NULL)
		return NULL;

	if (!cardtree_path_to_iter(ct,root,&iter))
		return NULL;

	cardtree_find_all_ext(res,ct,&iter,node,id);

	s = r = a_strfinalize(res);
	while (*s)
	{
		if (*s==';') *s='\0';
		s++;
	}
	return r;
}


void cardtree_bind_to_treeview(cardtree_t* ct, GtkWidget *view)
{
	gtk_tree_view_set_model(GTK_TREE_VIEW(view),GTK_TREE_MODEL(ct->_store));
}

void cardtree_free(cardtree_t* ct)
{
	int i;
	g_object_unref(ct->_store); 
	for (i=0;i<CARDTREE_COUNT_ICONS;i++)
		g_object_unref(ct->_icons[i]);
	g_free(ct);
}

/* * * */

char *cardtree_path_from_gtktreepath(GtkTreePath *path)
{
	gint depth;
	gint *indices;
	char *res;
	int i,size;
	
	depth = gtk_tree_path_get_depth(path);
	indices = gtk_tree_path_get_indices(path);

	if (depth==0) return NULL;

	size = 0;
	for (i=0;i<depth;i++)
	{
		size += snprintf(NULL,0,"%i",indices[i])+1;	
	}
	res = malloc(size+1);
	
	size = sprintf(res,"@%i",indices[0]);
	for (i=1;i<depth;i++)
	{
		size += sprintf(res+size,":%i",indices[i]);
	}
	return res;
}

char *cardtree_path_from_iter(cardtree_t* ct, GtkTreeIter *iter)
{
	GtkTreePath *gtk_path = gtk_tree_model_get_path(GTK_TREE_MODEL(ct->_store),iter);
	char *rpath;

	if (!gtk_path)
		return NULL;
	rpath = cardtree_path_from_gtktreepath(gtk_path);
	gtk_tree_path_free(gtk_path);
	return rpath;
}

gboolean cardtree_path_to_iter(cardtree_t *ct, const char *path, GtkTreeIter *iter)
{
	if (path==NULL)
		return gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ct->_store),iter);

	if (cardtree_path_is_valid(path))
		return gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),iter,path+1);	

	return FALSE;
}

void cardtree_path_free(char *path)
{
	if (path) free(path);
}

gboolean cardtree_path_is_valid(const char *path)
{
	if (path==NULL)
		return FALSE;
	if (*path++!='@')
		return FALSE;
/*	for (;;)
	{
		if (*path<'0' || *path>'9') return FALSE;
		path++;
		while (*path>='0' && *path<='9') path++;
		if (*path==0) return TRUE;
		if (*path!=':') return FALSE;
		path++;
	}*/
	return TRUE;
}

gboolean cardtree_attribute_set(cardtree_t *ct, const char *path, const gchar *aname, const gchar *avalue)
{
	GtkTreeIter iter;
	if (cardtree_path_to_iter(ct,path,&iter))
		return cardtree_set_attribute_iter(ct,&iter,aname,avalue);
	return FALSE;
}

gboolean cardtree_attribute_get(cardtree_t *ct, const char *path, const gchar *aname, gchar **avalue)
{
	GtkTreeIter iter;
	if (cardtree_path_to_iter(ct,path,&iter))
		return cardtree_get_attribute_iter(ct,&iter,aname,avalue);
	return FALSE;
}
   
