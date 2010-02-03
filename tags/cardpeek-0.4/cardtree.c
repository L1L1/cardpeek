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

#include "icons.c"

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
				  GDK_TYPE_PIXBUF);

  ct->_icons[0] = gdk_pixbuf_new_from_inline (-1, icon_item, FALSE, NULL);
  ct->_icons[1] = gdk_pixbuf_new_from_inline (-1, icon_card, FALSE, NULL);
  ct->_icons[2] = gdk_pixbuf_new_from_inline (-1, icon_folder, FALSE, NULL);
  ct->_icons[3] = gdk_pixbuf_new_from_inline (-1, icon_file, FALSE, NULL);
  ct->_icons[4] = gdk_pixbuf_new_from_inline (-1, icon_record, FALSE, NULL);
  ct->_icons[5] = gdk_pixbuf_new_from_inline (-1, icon_block, FALSE, NULL);
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

void cardtree_create_markup_name_id(cardtree_t* ct, GtkTreeIter* iter)
{
  a_string_t *markup_name_id;
  char *name;
  char *id;
  int icon_index;

  gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),iter,
		     C_NAME, &name,
		     C_ID, &id,
		     -1);

  if (strcasecmp(name,"Card")==0)
    icon_index=1;
  else if (strcasecmp(name,"Application")==0)
    icon_index=2;
  else if (strcasecmp(name,"File")==0)
    icon_index=3;
  else if (strcasecmp(name,"Record")==0)
    icon_index=4;
  else if (iter_get_depth(GTK_TREE_MODEL(ct->_store),iter)==1)
    icon_index=5;
  else
    icon_index=0;

  markup_name_id = a_strnew(NULL);

  if (id)
    a_sprintf(markup_name_id,"<b>%s</b> %s",name,id);
  else
    a_sprintf(markup_name_id,"<b>%s</b>",name);

  gtk_tree_store_set (ct->_store, iter,
		      C_ICON, ct->_icons[icon_index],
		      C_MARKUP_NAME_ID, a_strval(markup_name_id),
		      -1);

  if (name) g_free(name);
  if (id) g_free(id);
  a_strfree(markup_name_id);
}

void cardtree_create_markup_value(cardtree_t* ct, GtkTreeIter* iter)
{
  a_string_t *markup_value;
  char *value;
  int i,len;

  gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),iter,
		     C_VALUE, &value,
		     -1);

  if (value)  
  {
    markup_value   = a_strnew(NULL);
    a_strcpy(markup_value,"<tt>");
    i=0;
    len=strlen(value);
    while (i<len && i<256)
    {
      a_strpushback(markup_value,value[i++]);
      if (((i%64)==0) && (i+1<len)) a_strpushback(markup_value,'\n');
    }
    a_strcat(markup_value,"</tt>");
    if (len>256) a_strcat(markup_value,"<b>[...]</b>");

    gtk_tree_store_set (ct->_store, iter,
			C_MARKUP_VALUE, a_strval(markup_value),
			-1);
    g_free(value);
    a_strfree(markup_value);
  }
  else
  {
    gtk_tree_store_set (ct->_store, iter,
                        C_MARKUP_VALUE, NULL,
			-1);
  }
}

const char* cardtree_add_node(cardtree_t* ct,
			      const char* path,
			      const char* name, 
			      const char* id, 
			      int length, 
			      const char* comment)
{
  GtkTreeIter parent;
  GtkTreeIter child;
  char length_string[12];

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

  if (length>=0)
  {
    sprintf(length_string,"%i",length);
    gtk_tree_store_set (ct->_store, &child,
			C_LENGTH, length_string,
			-1);
  }

  if (id) 
    gtk_tree_store_set (ct->_store, &child, 
	  		C_ID, id, 
  			-1);
  if (comment) 
    gtk_tree_store_set (ct->_store, &child, 
	  		C_COMMENT, comment, 
  			-1);

  cardtree_create_markup_name_id(ct,&child);

  return ct->_tmpstr;
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
		      -1);

  cardtree_create_markup_value(ct,&iter);
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


gboolean cardtree_get_node(cardtree_t* ct, const char* path,
			   char** name, 
			   char** id, 
			   int *length, 
			   char** comment,
			   int *num_children)
{
  GtkTreeIter iter;
  char* length_string;

  if (path==NULL || cardtree_is_empty(ct))
    return FALSE;
  if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),&iter,path)==FALSE)
    return FALSE;
  
  if (name!=NULL)
    gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_NAME,name,-1);

  if (id!=NULL)
    gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_ID,id,-1);

  if (length!=NULL)
  {
    gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_LENGTH,&length_string,-1);
    if (length_string)
    {
      *length = atoi(length_string);
      g_free(length_string);
    }
    else
      *length = -1;
  }

  if (comment!=NULL)
    gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_COMMENT,comment,-1);

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

gboolean node_to_xml(a_string_t* res, GtkTreeModel *store, GtkTreeIter* iter, int depth)
{
  int i;
  gchar* name;
  gchar* id;
  gchar* length_string; 
  gchar* comment;
  gchar* value;
  GtkTreeIter child;

  do {
    gtk_tree_model_get(store,iter,
	     	       C_NAME,&name,
	     	       C_ID,&id,
		       C_LENGTH,&length_string,
	     	       C_COMMENT,&comment,
		       C_VALUE,&value,
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
    if (length_string)
    {
      a_strcat(res," length=\"");
      a_strcat(res,length_string);
      a_strcat(res,"\"");
    }
    if (comment)
    {
      a_strcat(res," comment=\"");
      a_strcat(res,comment);
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
	a_strcat(res,">");
	a_strcat(res,value);
	a_strcat(res,"</node>\n");
      }
      else
	a_strcat(res," />\n");
    }

    if (name) g_free(name);
    if (id) g_free(id);
    if (length_string) g_free(length_string);
    if (comment) g_free(comment);
    if (value) g_free(value);
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
	else if (a_strequal(elt,"length"))
	{
	  gtk_tree_store_set (ct->_store, &child,
			      C_LENGTH, a_strval(atr),
			      -1);
	}
	else if (a_strequal(elt,"comment"))
	{
	  gtk_tree_store_set (ct->_store, &child,
			      C_COMMENT, a_strval(atr),
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
  ct->_view=GTK_TREE_VIEW(view);
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

