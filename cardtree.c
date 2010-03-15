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

void cardtree_clear_tmpstr(cardtree_t* ct) 
/* internal */
{ 
  if (ct->_tmpstr) 
  { 
    g_free(ct->_tmpstr); 
    ct->_tmpstr=NULL; 
  } 
}

cardtree_t* cardtree_new()
{
  cardtree_t* ct = (cardtree_t*)g_malloc(sizeof(cardtree_t));

  ct->_tmpstr=NULL;
  ct->_store = gtk_tree_store_new(NUM_COLS,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_UINT);

  return ct;
}

gboolean cardtree_is_empty(cardtree_t* ct)
{
  GtkTreeIter iter;
  return gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ct->_store),&iter)==FALSE;
}

int iter_get_depth(GtkTreeModel *tree_model, GtkTreeIter* iter)
{
  GtkTreeIter parent;
  GtkTreeIter child=*iter;
  unsigned r=0;

  while (gtk_tree_model_iter_parent(tree_model,&parent,&child))
  {
    child=parent;
    r++;
  }
  return r;
}

int matchcaseprefix(const char *prefix, const char *str)
{
  if (strncasecmp(prefix,str,strlen(prefix))==0)
    return 1;
  return 0;
}

void cardtree_create_markup_name_id(cardtree_t* ct, GtkTreeIter* iter)
{
  a_string_t *markup_name_id;
  char *name;
  char *id;
  char *type;
  char *icon_name;

  gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),iter,
		     C_NAME, &name,
		     C_TYPE, &type,
		     C_ID, &id,
		     -1);

  icon_name="cardpeek-item";
  if (type!=NULL) { 
    switch (type[0]) {
      case 'c': if (strcmp("card",type)==0)        icon_name="cardpeek-smartcard";   break;
      case 'a': if (strcmp("application",type)==0) icon_name="cardpeek-application"; break;
      case 'f': if (strcmp("file",type)==0)        icon_name="cardpeek-file";        break;
      case 'r': if (strcmp("record",type)==0)      icon_name="cardpeek-record";      break;
      case 'b': if (strcmp("block",type)==0)       icon_name="cardpeek-block";       break;
    }
    g_free(type);
  }

  markup_name_id = a_strnew(NULL);

  if (id)
    a_sprintf(markup_name_id,"<b>%s</b> %s",name,id);
  else
    a_sprintf(markup_name_id,"<b>%s</b>",name);

  gtk_tree_store_set (ct->_store, iter,
		      C_ICON, icon_name,
		      C_MARKUP_NAME_ID, a_strval(markup_name_id),
		      -1);

  if (name) g_free(name);
  if (id) g_free(id);
  a_strfree(markup_name_id);
}

char *cardtree_create_markup_text(const char* src, int full, const char *color)
{
  a_string_t *markup_value;
  int i,len,max;

  markup_value   = a_strnew(NULL);
  if (color)
    a_sprintf(markup_value,"<span foreground=\"%s\">",color);
  a_strcat(markup_value,"<tt>");
  i=0;
  len=strlen(src);
  if (full)
    max=len;
  else
    max=(256<len?256:len);
  while (i<max)
  {
    switch (src[i]) {
      case '<': 
	a_strcat(markup_value,"&lt;"); break;
      case '>':
	a_strcat(markup_value,"&gt;"); break;
      case '&':
	a_strcat(markup_value,"&amp;"); break;
      default:
	a_strpushback(markup_value,src[i]);
    }
    i++;
    if (((i%64)==0) && (i+1<len)) a_strpushback(markup_value,'\n');
  }
  a_strcat(markup_value,"</tt>");
  if (len>256 && !full) a_strcat(markup_value,"<b>[...]</b>");

  if (color)
    a_strcat(markup_value,"</span>");
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
    markup = cardtree_create_markup_text(value,flags&1,NULL);
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
  char *color="#2F2F7F";

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
  }

  if (value)  
  {
    markup = cardtree_create_markup_text(value,flags&1,color);
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


const char* cardtree_add_node(cardtree_t* ct,
			      const char* path,
			      const char* name, 
			      const char* id, 
			      int size, 
			      const char* type)
{
  GtkTreeIter parent;
  GtkTreeIter child;
  char size_string[12];

  if (path!=NULL && cardtree_is_empty(ct))
    return NULL;

  if (name==NULL)
    return NULL;

  if (path!=NULL)
  {
    if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),&parent,path)==FALSE)
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
  cardtree_clear_tmpstr(ct);

  ct->_tmpstr = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(ct->_store),&child);

  /* FIXME : check if parent is not leaf */
  gtk_tree_store_set (ct->_store, &child, 
		      C_NAME, name, 
		      -1);

  if (size>=0)
  {
    sprintf(size_string,"%i",size);
    gtk_tree_store_set (ct->_store, &child,
			C_SIZE, size_string,
			-1);
  }

  if (id) 
    gtk_tree_store_set (ct->_store, &child, 
	  		C_ID, id, 
  			-1);
  if (type) 
    gtk_tree_store_set (ct->_store, &child, 
	  		C_TYPE, type, 
  			-1);

  cardtree_create_markup_name_id(ct,&child);

  return ct->_tmpstr;
}


gboolean cardtree_get_attributes(cardtree_t* ct,
                                 const char* path,
                                 const char **attribute_names,
				 char **attribute_values)
{
  GtkTreeIter iter;
  int i;

  if (path==NULL || cardtree_is_empty(ct))
    return FALSE;
  if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),&iter,path)==FALSE)
    return FALSE;

  for (i=0;attribute_names[i];i++)
  {
    if (strcmp(attribute_names[i],"name")==0) 
	gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_NAME,&(attribute_values[i]),-1);
    else if (strcmp(attribute_names[i],"id")==0)     
       	gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_ID,&(attribute_values[i]),-1);
    else if (strcmp(attribute_names[i],"size")==0)     
	gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_SIZE,&(attribute_values[i]),-1);
    else if (strcmp(attribute_names[i],"type")==0)     
        gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_TYPE,&(attribute_values[i]),-1);
    else
        /* FIXME: add other attributes with attributes.h */
     	return FALSE;
  }
  attribute_values[i]=NULL;
  return TRUE;
}

gboolean internal_cardtree_set_single_attribute(GtkTreeStore *store, GtkTreeIter *iter, 
						const char *attr_name, const char *attr_value)
{
  if (strcmp(attr_name,"name")==0) 
    gtk_tree_store_set(store,iter,C_NAME,attr_value,-1);
  else if (strcmp(attr_name,"id")==0)     
    gtk_tree_store_set(store,iter,C_ID,attr_value,-1);
  else if (strcmp(attr_name,"size")==0)     
    gtk_tree_store_set(store,iter,C_SIZE,attr_value,-1);
  else if (strcmp(attr_name,"type")==0)
    gtk_tree_store_set(store,iter,C_TYPE,attr_value,-1);
  else
    /* FIXME: add other attributes with attributes.h */
    return FALSE;
  return TRUE;
}

gboolean cardtree_set_attributes(cardtree_t* ct,
                                 const char* path,
                                 const char **attribute_names,
				 const char **attribute_values)
{
  GtkTreeIter iter;
  int i;

  if (path==NULL || cardtree_is_empty(ct))
    return FALSE;
  if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),&iter,path)==FALSE)
    return FALSE;

  for (i=0;attribute_names[i];i++)
  {
    if (internal_cardtree_set_single_attribute(ct->_store,&iter,
					       attribute_names[i],attribute_values[i])==FALSE)
      return FALSE;
  }
  cardtree_create_markup_name_id(ct,&iter);
  return TRUE;
}



gboolean cardtree_set_value(cardtree_t* ct,
                            const char* path,
                            const char* value)
{
  GtkTreeIter iter;

  if (path==NULL || cardtree_is_empty(ct))
    return FALSE;

  if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),&iter,path)==FALSE)
    return FALSE;

  if (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(ct->_store),&iter))
    return FALSE;

  gtk_tree_store_set (ct->_store, &iter, 
		      C_VALUE, value, 
		      C_FLAGS, 0,
		      -1);

  cardtree_create_markup_value(ct,&iter);
  cardtree_create_markup_alt_value(ct,&iter);
  return TRUE;
}

gboolean cardtree_set_alt_value(cardtree_t* ct,
			      	const char* path,
				const char* value)
{
  GtkTreeIter iter;

  if (path==NULL || cardtree_is_empty(ct))
    return FALSE;

  if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),&iter,path)==FALSE)
    return FALSE;

  if (gtk_tree_model_iter_has_child(GTK_TREE_MODEL(ct->_store),&iter))
    return FALSE;

  gtk_tree_store_set (ct->_store, &iter, 
		      C_ALT_VALUE, value, 
		      -1);

  cardtree_create_markup_alt_value(ct,&iter);
  return TRUE;
}



gboolean cardtree_get_value(cardtree_t* ct,
                            const char* path,
                            char** value)
{
  GtkTreeIter iter;
  
  if (path==NULL || cardtree_is_empty(ct))
    return FALSE;

  if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),&iter,path)==FALSE)
    return FALSE;

  gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_VALUE,value,-1);

  return TRUE;
}

gboolean cardtree_get_alt_value(cardtree_t* ct,
			      	const char* path,
				char** value)
{
  GtkTreeIter iter;
  
  if (path==NULL || cardtree_is_empty(ct))
    return FALSE;

  if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),&iter,path)==FALSE)
    return FALSE;

  gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_ALT_VALUE,value,-1);

  return TRUE;
}


gboolean cardtree_get_node(cardtree_t* ct, const char* path,
			   char** name, 
			   char** id, 
			   int *size, 
			   char** type,
			   int *num_children)
{
  GtkTreeIter iter;
  char* size_string;

  if (path==NULL || cardtree_is_empty(ct))
    return FALSE;
  if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),&iter,path)==FALSE)
    return FALSE;
  
  if (name!=NULL)
    gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_NAME,name,-1);

  if (id!=NULL)
    gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_ID,id,-1);

  if (size!=NULL)
  {
    gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_SIZE,&size_string,-1);
    if (size_string)
    {
      *size = atoi(size_string);
      g_free(size_string);
    }
    else
      *size = -1;
  }

  if (type!=NULL)
    gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_TYPE,type,-1);

  if (num_children!=NULL)
    *num_children = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(ct->_store),&iter);

  return TRUE;
}

gboolean cardtree_delete_node(cardtree_t* ct, const char *path)
{
  GtkTreeIter iter;
	
  if (path==NULL && cardtree_is_empty(ct))
    return FALSE;

  if (path==NULL)
  {
    gtk_tree_store_clear(ct->_store);
    return TRUE;
  }  
 
  if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),&iter,path)==FALSE)
    return FALSE;
  return gtk_tree_store_remove(ct->_store,&iter);
}

gboolean cardtree_get_flags(cardtree_t* ct,
                            const char* path,
                            unsigned *flags)
{
  GtkTreeIter iter;

  if (path==NULL || cardtree_is_empty(ct))
    return FALSE;
  if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),&iter,path)==FALSE)
    return FALSE;

  gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_FLAGS,flags,-1);

  return TRUE;
}

gboolean cardtree_set_flags(cardtree_t* ct,
                            const char* path,
                            unsigned flags)
{
  GtkTreeIter iter;

  if (path==NULL || cardtree_is_empty(ct))
    return FALSE;
  if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),&iter,path)==FALSE)
    return FALSE;

  gtk_tree_store_set (ct->_store, &iter, C_FLAGS, flags, -1);
  cardtree_create_markup_value(ct,&iter);
  cardtree_create_markup_alt_value(ct,&iter);

  return TRUE;
}


gboolean node_to_xml(a_string_t* res, GtkTreeModel *store, GtkTreeIter* iter, int depth)
{
  int i;
  gchar* name;
  gchar* id;
  gchar* size_string; 
  gchar* type;
  gchar* value;
  gchar* alt_value;
  GtkTreeIter child;
  gchar* esc_alt_value;

  do {
    gtk_tree_model_get(store,iter,
	     	       C_NAME,&name,
	     	       C_ID,&id,
		       C_SIZE,&size_string,
	     	       C_TYPE,&type,
		       C_VALUE,&value,
		       C_ALT_VALUE,&alt_value,
	     	       -1);
    for(i=0;i<depth;i++) a_strcat(res,"  ");

    a_strcat(res,"<node name=\"");
    a_strcat(res,name);
    a_strcat(res,"\"");
    if (id)
    {
      a_strcat(res," id=\"");
      a_strcat(res,id);
      a_strcat(res,"\"");
    }
    if (size_string)
    {
      a_strcat(res," size=\"");
      a_strcat(res,size_string);
      a_strcat(res,"\"");
    }
    if (type)
    {
      a_strcat(res," type=\"");
      a_strcat(res,type);
      a_strcat(res,"\"");
    }
    if (gtk_tree_model_iter_children(store,&child,iter))
    {
      a_strcat(res,">\n");
      node_to_xml(res,store,&child,depth+1);
      for(i=0;i<depth;i++) a_strcat(res,"  ");
      a_strcat(res,"</node>\n");
    }
    else
    {
      if (value)
      {
	a_strcat(res,"><val>");
	a_strcat(res,value);
	a_strcat(res,"</val>");
	if (alt_value)
	{
	  a_strcat(res,"<alt>");
	  esc_alt_value = g_markup_escape_text(alt_value,-1);
	  a_strcat(res,esc_alt_value);
	  g_free(esc_alt_value);
	  a_strcat(res,"</alt>");
	}
	a_strcat(res,"</node>\n");
      }
      else
	a_strcat(res," />\n");
    }

    if (name) g_free(name);
    if (id) g_free(id);
    if (size_string) g_free(size_string);
    if (type) g_free(type);
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
    path_exists = gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),&iter,path);

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
      internal_cardtree_set_single_attribute((ctx->ctx_tree)->_store,&child,
					     attribute_names[i],
					     attribute_values[i]);
    }
    cardtree_create_markup_name_id(ctx->ctx_tree,&child);
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
			  const gchar         *element_name,
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

const char* cardtree_find_ext(cardtree_t* ct, GtkTreeIter* iter, 
			      const char* t_name, const char *t_id)
{
  gchar* name;
  gchar* id;
  GtkTreeIter child;
  int match;
  const char * retval;

 
  /* check item */
  gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),iter,
		     C_NAME,&name,
		     C_ID,&id,
		     -1);

  if ((t_name==NULL || (name && strcmp(t_name,name)==0)) &&
      (t_id==NULL || (id && strcmp(t_id,id)==0)))
    match = 1;
  else
    match = 0;
  if (name) g_free(name);
  if (id) g_free(id);

  if (match)
  {
    cardtree_clear_tmpstr(ct);
    ct->_tmpstr = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(ct->_store),iter);
    return ct->_tmpstr;
  }
  /* check children */
  if (gtk_tree_model_iter_children(GTK_TREE_MODEL(ct->_store),&child,iter)!=FALSE)
  {
    do
    {
      retval = cardtree_find_ext(ct,&child,t_name,t_id);
      if (retval) return retval;
    }
    while (gtk_tree_model_iter_next(GTK_TREE_MODEL(ct->_store),&child)!=FALSE);
  }
  return NULL;
}

const char* cardtree_find_node(cardtree_t* ct, 
			       const char *root, const char* node, const char *id)
{
  GtkTreeIter iter;
  int path_exists;

  if (id==NULL && node==NULL)
    return NULL;

  if (root==NULL)
    path_exists = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ct->_store),&iter);
  else
    path_exists = gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),&iter,root);

  if (!path_exists)
    return NULL;

  return cardtree_find_ext(ct,&iter,node,id);
}

void cardtree_bind_to_treeview(cardtree_t* ct, GtkWidget *view)
{
  gtk_tree_view_set_model(GTK_TREE_VIEW(view),GTK_TREE_MODEL(ct->_store));
}

void cardtree_free(cardtree_t* ct)
{
  int i;
  g_object_unref(ct->_store); 
  cardtree_clear_tmpstr(ct);
  for (i=0;i<CARDTREE_COUNT_ICONS;i++)
    g_object_unref(ct->_icons[i]);
  g_free(ct);
}

