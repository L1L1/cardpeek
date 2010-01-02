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
				  G_TYPE_BOOLEAN, 
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING,
				  G_TYPE_STRING);
  return ct;
}

gboolean cardtree_is_empty(cardtree_t* ct)
{
  GtkTreeIter iter;
  return gtk_tree_model_get_iter_first(GTK_TREE_MODEL(ct->_store),&iter)==FALSE;
}


void cardtree_create_markup(cardtree_t* ct, GtkTreeIter* iter)
{
  char markup[4000]; /* FIXME: dyn alloc this */
  char *node;
  char *id;
  int leaf;
  int i,j,len;

  gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),iter,
		     C_LEAF, &leaf,
		     C_NODE, &node,
		     C_ID, &id,
		     -1);

  if (!leaf)
  {
    if (id)
      sprintf(markup,"<b>%s</b> %s",node,id);
    else
      sprintf(markup,"<b>%s</b>",node);
  }
  else
  {
    strcpy(markup,"<tt>");
    i=0;
    j=strlen(markup);
    len=strlen(node);
    while (i<len)
    {
      markup[j++]=node[i++];
      if (((i%64)==0) && (i+1<len)) markup[j++]='\n';
    }
    markup[j]=0;
    strcat(markup,"</tt>");
  }

  gtk_tree_store_set (ct->_store, iter,
		      C_MARKUP, markup,
		      -1);

  if (node) g_free(node);
  if (id) g_free(id);
}

const char* cardtree_append(cardtree_t* ct,
			    const char* path, gboolean leaf,
	      		    const char* node, const char* id, int length, const char* comment)
{
  GtkTreeIter parent;
  GtkTreeIter child;
  char length_string[12];

  if (path!=NULL && cardtree_is_empty(ct))
    return NULL;

  if (node==NULL)
    return NULL;

  if (path!=NULL)
  {
    if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),&parent,path)==FALSE)
      return NULL;
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
		      C_LEAF, leaf, 
		      C_NODE, node, 
		      -1);

  if (length>=0)
  {
    sprintf(length_string,"%i",length);
    gtk_tree_store_set (ct->_store, &child,
			C_LENGTH, length_string,
			-1);
  }

  if (id && !leaf) 
    gtk_tree_store_set (ct->_store, &child, 
	  		C_ID, id, 
  			-1);
  if (comment) 
    gtk_tree_store_set (ct->_store, &child, 
	  		C_COMMENT, comment, 
  			-1);

  cardtree_create_markup(ct,&child);

  return ct->_tmpstr;
}

gboolean cardtree_get(cardtree_t* ct, const char* path,
		      gboolean* leaf,
		      char** node, char** id, int *length, char** comment)
{
  GtkTreeIter iter;
  char* length_string;

  if (path==NULL || cardtree_is_empty(ct))
    return FALSE;
  if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(ct->_store),&iter,path)==FALSE)
      return FALSE;
  
  if (leaf!=NULL)
    gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_LEAF,leaf,-1);  
  
  if (node!=NULL)
    gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),&iter,C_NODE,node,-1);

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

  return TRUE;
}

gboolean cardtree_delete(cardtree_t* ct, const char *path)
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
  gboolean leaf;
  gchar* node;
  gchar* id;
  gchar* length_string; 
  gchar* comment;
  GtkTreeIter child;

  do {
    gtk_tree_model_get(store,iter,
	     	       C_LEAF,&leaf,
	     	       C_NODE,&node,
	     	       C_ID,&id,
		       C_LENGTH,&length_string,
	     	       C_COMMENT,&comment,
	     	       -1);
    for(i=0;i<depth;i++) a_strcat(res,"  ");

    if (leaf)
    {
      a_strcat(res,"<data>");
      a_strcat(res,node);
      a_strcat(res,"</data>\n");
    }
    else
    {
      a_strcat(res,"<node name=\"");
      a_strcat(res,node);
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
	a_strcat(res," />\n");
      }
    }
  } while (gtk_tree_model_iter_next(store,iter));
  if (node) g_free(node);
  if (id) g_free(id);
  if (length_string) g_free(length_string);
  if (comment) g_free(comment);
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
    else if (a_strequal(elt,"<node"))
    {
      gtk_tree_store_append (ct->_store, &child, parent);
      gtk_tree_store_set (ct->_store, &child,
  			  C_LEAF, 0,
  			  -1);
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
			      C_NODE, a_strval(atr),
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

      cardtree_create_markup(ct,&child);

      if (!a_strequal(elt,"/>")) 
      {
	if (!a_strequal(elt,">"))
	{
	  log_printf(LOG_ERROR,"Expected '>' or '/>'");
	  goto clean_up;
	}
	retval = internal_cardtree_from_xml_file(ct,&child,F);
	if (retval!=XML_OK) goto clean_up;
	/*if (assert_next_xml_token(F,elt,&in_element,"</node>")!=XML_OK) goto clean_up;*/
      }
    }
    else if (a_strequal(elt,"<data"))
    {
      gtk_tree_store_append (ct->_store, &child, parent);
      gtk_tree_store_set (ct->_store, &child,
  			  C_LEAF, 1,
  			  -1);

      if (get_next_xml_token(F,elt,&in_element)!=XML_OK) goto clean_up;
      if (a_strequal(elt,"/>")) continue;
      if (!a_strequal(elt,">"))
      {
	log_printf(LOG_ERROR,"Expected '>' or '/>'");
	goto clean_up;
      }
      if (get_next_xml_token(F,elt,&in_element)!=XML_OK) goto clean_up;
      if (in_element!=TOK_TEXT) 
      {
	log_printf(LOG_ERROR,"Expected text inside <data>...</data>, got %s instead.",
	     a_strval(elt));
	goto clean_up;
      }
      gtk_tree_store_set (ct->_store, &child,
	  		  C_NODE, a_strval(elt),
	  		  -1);
      cardtree_create_markup(ct,&child);
      if (assert_next_xml_token(F,elt,&in_element,"</>")!=XML_OK) goto clean_up;
    }
    else if (a_strequal(elt,"<cardtree"))
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
	log_printf(LOG_ERROR,"Expected '>' or '/>'");
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
      log_printf(LOG_ERROR,"Syntax error : %s",a_strval(elt));
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
      log_printf(LOG_ERROR,"Error in path '%s'",debug_msg);
      g_free(debug_msg);
    }
    else
      log_printf(LOG_ERROR,"Error in root");
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
			      const char* t_node, const char *t_id)
{
  gboolean leaf;
  gchar* node;
  gchar* id;
  GtkTreeIter child;
  int match;
  const char * retval;

  do {
    /* check item */
    gtk_tree_model_get(GTK_TREE_MODEL(ct->_store),iter,
	  	       C_LEAF,&leaf,
	  	       C_NODE,&node,
	  	       C_ID,&id,
	  	       -1);

    if ((t_node==NULL || (node && strcmp(t_node,node)==0)) &&
	(t_id==NULL || (id && strcmp(t_id,id)==0)))
      match = 1;
    else
      match = 0;
    if (node) g_free(node);
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
      retval = cardtree_find_ext(ct,&child,t_node,t_id);
      if (retval) return retval;
    }
  } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(ct->_store),iter)!=FALSE);
  return NULL;
}

const char* cardtree_find(cardtree_t* ct, const char *root, const char* node, const char *id)
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
  g_object_unref(ct->_store); 
  cardtree_clear_tmpstr(ct);
  g_free(ct);
}

