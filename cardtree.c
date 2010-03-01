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

gboolean cardtree_get_attribute(cardtree_t* ct,
                                const char* path,
                                char **attr_list)
{
  GtkTreeIter iter;
  char *attr_name;
  char *attr_value;
  int i;

  if (path==NULL || cardtree_is_empty(ct))
    return FALSE;
  if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),&iter,path)==FALSE)
    return FALSE;

  for (i=0;attr_list[i];i+=2)
  {
    attr_name=attr_list[i];
    attr_value=attr_list[i+1];
    if (strcmp(attr_name,"name")==0) 
	gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_NAME,&attr_value,-1);
    else if (strcmp(attr_name,"id")==0)     
       	gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_ID,&attr_value,-1);
    else if (strcmp(attr_name,"size")==0)     
	gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_SIZE,&attr_value,-1);
    else if (strcmp(attr_name,"type")==0)     
        gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_TYPE,&attr_value,-1);
    else
        /* FIXME: add other attributes with attributes.h */
     	return FALSE;
  }
  cardtree_create_markup_name_id(ct,&iter);
  return TRUE;
}

gboolean cardtree_set_attribute(cardtree_t* ct,
                                const char* path,
                                const char **attr_list)
{
  GtkTreeIter iter;
  const char *attr_name;
  const char *attr_value;
  int i;

  if (path==NULL || cardtree_is_empty(ct))
    return FALSE;
  if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),&iter,path)==FALSE)
    return FALSE;

  for (i=0;attr_list[i];i+=2)
  {
    attr_name=attr_list[i];
    attr_value=attr_list[i+1];
    if (strcmp(attr_name,"name")==0) 
	gtk_tree_store_set(ct->_store,&iter,C_NAME,attr_value,-1);
    else if (strcmp(attr_name,"id")==0)     
       	gtk_tree_store_set(ct->_store,&iter,C_ID,attr_value,-1);
    else if (strcmp(attr_name,"size")==0)     
	gtk_tree_store_set(ct->_store,&iter,C_SIZE,attr_value,-1);
    else if (strcmp(attr_name,"type")==0)
        gtk_tree_store_set(ct->_store,&iter,C_TYPE,attr_value,-1);
    else
        /* FIXME: add other attributes with attributes.h */
     	return FALSE;
  }
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
	a_strcat(res,"><value>");
	a_strcat(res,value);
	a_strcat(res,"</value>");
	if (alt_value)
	{
	  a_strcat(res,"<alt>");
	  a_strcat(res,alt_value);
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

int cardtree_to_xml_file(cardtree_t* ct, const char *fname, const char* path)
{
  FILE *save;
  char *xml;
  int retval;

  if ((save=fopen(fname,"w"))==NULL)
  {
    log_printf(LOG_ERROR,"Could not open '%s' for output (%s)",fname,strerror(errno));
    return 0;
  }
  xml = cardtree_to_xml(ct,path);

  if (fwrite(xml,strlen(xml),1,save)!=1)
  {
    log_printf(LOG_ERROR,"Output error on '%s' (%s)",fname,strerror(errno));
    retval = 0;
  }
  else
    retval = 1;
  free(xml);
  fclose(save);
  return retval;
}

int fpeek(FILE* F)
{
  int c = fgetc(F);
  if (c!=EOF) ungetc(c,F);
  return c;
}

int lineno;
int linepos;
int xfgetc(FILE *F)
{
  int c = fgetc(F);
  linepos++;
  if (c=='\n')
  {
    lineno++;
    linepos=0;
  }
  return c;
}

int isvalid(int c)
{
  if (c>='a' && c<='z') return 1;
  if (c>='A' && c<='Z') return 1;
  return 0;
}

enum {
  XML_ERROR = -1,
  XML_END = 0,
  XML_OK = 1
};

enum {
  TOK_TEXT,
  TOK_INSTRUCTION,
  TOK_SLASH_GTE,
  TOK_GTE,
  TOK_END,
  TOK_BEGIN,
  TOK_ATTRIBUTE,
  TOK_EQUAL,
  TOK_STRING
};

int get_next_xml_token(FILE* F, a_string_t *tok, int *in_element)
{
  int c,marker;
  
  while (isspace(c = fpeek(F))) xfgetc(F);
  
  a_strcpy(tok,"");

  c = xfgetc(F);

  if (*in_element>=TOK_BEGIN)
  {
    if (c=='\'' || c=='"')
    {
      marker = c;
      *in_element =  TOK_STRING;
      for (;;)
      {
	c = xfgetc(F);
	if (c==marker) return XML_OK;
	if (c==EOF) {
	  log_printf(LOG_ERROR,"unterminated quoted value");
	  return XML_ERROR;
	}
	a_strpushback(tok,c);
      }
    }

    if (c=='=')
    {
      *in_element = TOK_EQUAL;
      a_strcpy(tok,"=");
      return XML_OK;
    }

    if (c=='>')
    {
      a_strcpy(tok,">");
      *in_element=TOK_GTE;
      return XML_OK;
    }

    if (c=='/')
    {
      c = xfgetc(F);
      if (c!='>') {
	log_printf(LOG_ERROR,"Expected '>'");
	return XML_ERROR;
      }
      a_strcpy(tok,"/>");
      *in_element=TOK_SLASH_GTE;
      return XML_OK;
    }

    *in_element=TOK_ATTRIBUTE;
    if (!isvalid(c)) return XML_ERROR;
    a_strpushback(tok,c);

    for (;;)
    {
      c = fpeek(F);
      if (c==EOF) {
	log_printf(LOG_ERROR,"Unterminated attribute '%s...'",a_strval(tok));
	return XML_ERROR;
      }
      if (!isvalid(c)) return XML_OK;
      a_strpushback(tok,xfgetc(F));
    }
    return XML_OK;
  }
  else
  {
    if (c=='<')
    {
      a_strpushback(tok,c);
      c = fgetc(F);

      if (c=='?' || c=='!' || c=='/')
      {
	if (c=='/')
	  *in_element=TOK_END;
	else
	  *in_element=TOK_INSTRUCTION;

	a_strpushback(tok,c);
	for (;;)
	{
	  c = xfgetc(F);
	  if (c==EOF) {
	    log_printf(LOG_ERROR,"Unterminated '%s...'",a_strval(tok));
	    return XML_ERROR;
	  }
	  if (c=='>') {
	    a_strpushback(tok,'>');
	    return XML_OK;
	  }
	}
      }

      *in_element=TOK_BEGIN;
      if (!isvalid(c)) return XML_ERROR;
      a_strpushback(tok,c);

      for (;;)
      {
	c = fpeek(F);
	if (c==EOF) {
	  log_printf(LOG_ERROR,"Unterminated element '%s...'",a_strval(tok));
	  return XML_ERROR;
	}
	if (!isvalid(c)) return XML_OK;
	a_strpushback(tok,xfgetc(F));
      }
    }
    else /* if c=='<' */
    {
      *in_element=TOK_TEXT;

      a_strpushback(tok,c);
      for (;;)
      {
	c = fpeek(F);
	if (c==EOF) {
	  log_printf(LOG_ERROR,"Unterminated data '%s...'",a_strval(tok));
	  return XML_ERROR;
	}
	if (c=='<') return XML_OK;
	a_strpushback(tok,xfgetc(F));
      }
    } /* else not c=='<' */
  } /* else not in_element */
  log_printf(LOG_ERROR,"Syntax error (should not happen)");
  return XML_ERROR;
}

int assert_next_xml_token(FILE *F, a_string_t *tok, int *in_element, const char *match)
{
  int retval = get_next_xml_token(F,tok,in_element);
  if (retval!=XML_OK)
    return retval;
  if (a_strequal(tok,match))
    return XML_OK;
  log_printf(LOG_ERROR,"Expected %s",match);
  return XML_ERROR;
}

int internal_cardtree_from_xml_file(cardtree_t *ct, GtkTreeIter *parent, FILE* F)
{
  /* quick and dirty, absolutely no checking ! */
  GtkTreeIter child;
  a_string_t *atr;
  a_string_t *elt;
  int retval = XML_ERROR;
  char *debug_msg;
  int in_element;
    
  elt = a_strnew(NULL);
  atr = a_strnew(NULL);

  while (!ferror(F))
  {
    in_element = TOK_TEXT;
    if (get_next_xml_token(F,elt,&in_element)!=XML_OK) goto clean_up;

    if (in_element==TOK_END)
    {
      retval = XML_OK;
      goto clean_up;
    }
    else if (in_element==TOK_TEXT)
    {
      if (parent==NULL)
      {
	log_printf(LOG_ERROR,"Unexpected string value in XML: %s",a_strval(elt));
      	retval = XML_ERROR;
    	goto clean_up;
      }

      gtk_tree_store_set (ct->_store, parent,
                          C_VALUE, a_strval(elt),
                          -1);

      cardtree_create_markup_value(ct,parent);
    }
    else if (in_element==TOK_BEGIN && a_strequal(elt,"<node"))
    {
      gtk_tree_store_append (ct->_store, &child, parent);
      for (;;)
      {
	if (get_next_xml_token(F,elt,&in_element)!=XML_OK) goto clean_up;

	if (in_element != TOK_ATTRIBUTE) break;

	if (assert_next_xml_token(F,atr,&in_element,"=")!=XML_OK) goto clean_up;

	if (get_next_xml_token(F,atr,&in_element)!=XML_OK) goto clean_up;
	if (in_element != TOK_STRING) goto clean_up;

	if (a_strequal(elt,"name"))
	{
	  gtk_tree_store_set (ct->_store, &child,
			      C_NAME, a_strval(atr),
			      -1);
	}
	else if (a_strequal(elt,"id")) 
	{
	  gtk_tree_store_set (ct->_store, &child,
			      C_ID, a_strval(atr),
			      -1);
	}
	else if (a_strequal(elt,"size"))
	{
	  gtk_tree_store_set (ct->_store, &child,
			      C_SIZE, a_strval(atr),
			      -1);
	}
	else if (a_strequal(elt,"type"))
	{
	  gtk_tree_store_set (ct->_store, &child,
			      C_TYPE, a_strval(atr),
			      -1);
	}
	else
	{
	  log_printf(LOG_ERROR,"Unexpected XML attribute '%s'",a_strval(elt));
	  retval = XML_ERROR;
	  goto clean_up;
	}
      }

      cardtree_create_markup_name_id(ct,&child);

      if (!a_strequal(elt,"/>")) 
      {
	if (!a_strequal(elt,">"))
	{
	  log_printf(LOG_ERROR,"Expected '>' or '/>' in XML");
	  goto clean_up;
	}
	retval = internal_cardtree_from_xml_file(ct,&child,F);
	if (retval!=XML_OK) goto clean_up;
	/*if (assert_next_xml_token(F,elt,&in_element,"</node>")!=XML_OK) goto clean_up;*/
      }
    }
    else if (in_element==TOK_BEGIN && a_strequal(elt,"<cardtree"))
    {
      if (parent!=NULL)
      {
	log_printf(LOG_ERROR,"XML ERROR - cardtree must be root element");
	goto clean_up;
      }
      if (get_next_xml_token(F,elt,&in_element)!=XML_OK) goto clean_up;
      if (a_strequal(elt,"/>"))
      {
	retval = XML_OK;
	goto clean_up;
      }
      if (!a_strequal(elt,">"))
      {
	log_printf(LOG_ERROR,"Expected '>' or '/>' in XML");
	goto clean_up;
      }
      retval = internal_cardtree_from_xml_file(ct,NULL,F);
      if (retval!=XML_OK) goto clean_up;
      /*retval = assert_next_xml_token(F,elt,&in_element,"</cardtree>");*/
      goto clean_up;
    }
    else if (in_element==TOK_INSTRUCTION)
    {
      /* NOP */
    }
    else
    {
      log_printf(LOG_ERROR,"XML syntax error : %s",a_strval(elt));
      retval = XML_ERROR;
      goto clean_up;
    }
  }
  
  log_printf(LOG_ERROR,"Unexpected end-of-file or error in XML");
  retval = XML_ERROR;

clean_up:
  if (retval==XML_ERROR)
  {
    if (parent)
    {
      debug_msg = gtk_tree_model_get_string_from_iter(GTK_TREE_MODEL(ct->_store),parent);
      log_printf(LOG_ERROR,"XML error in path '%s'",debug_msg);
      g_free(debug_msg);
    }
    else
      log_printf(LOG_ERROR,"XML error in root");
  }
  a_strfree(atr);
  a_strfree(elt);
  return retval;
}

int cardtree_from_xml_file(cardtree_t *ct, const char *fname)
{
  FILE *input;
  int retval;

  lineno=1;
  linepos=0;
  gtk_tree_store_clear(ct->_store);
  if ((input=fopen(fname,"r"))==NULL)
  {
    log_printf(LOG_ERROR,"Could not open '%s' for input (%s)",fname,strerror(errno));
    return 0;
  }
  retval = internal_cardtree_from_xml_file(ct,NULL,input);
  if (retval==XML_ERROR)
  {
    log_printf(LOG_ERROR,"Error on line %i[%i], while processing XML file '%s'",lineno,linepos,fname);
    gtk_tree_store_clear(ct->_store);
  }
  fclose(input);
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

