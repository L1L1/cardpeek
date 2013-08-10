/********************************************************************** 
*
* This file is part of Cardpeek, the smart card reader utility.
*
* Copyright 2009-2013 by Alain Pannetrat <L1L1@gmx.com>
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
#include <lauxlib.h>
#include "lua_ui.h"
#include "gui.h"
#include "gui_cardview.h"
#include "dyntree_model.h"
#include "misc.h"
#include <string.h>
#include <stdlib.h>
#include "bytestring.h"
#include "lua_bytes.h"
#include <glib.h>
#include <glib/gprintf.h>
#include "lua_nodes.h"

GtkTreeIter PSEUDO_ROOT;

/***********************************************************
 * SOME HELPER FUNCTIONS
 */

static GtkTreeIter* luaL_check_node_ref(lua_State *L, int p)
{
  void *ud = luaL_checkudata(L,p,"node_ref.type");
  luaL_argcheck(L, ud != NULL, p, "`node_ref' expected");
  return (GtkTreeIter *)ud;
}

static void lua_push_node_ref(lua_State *L, GtkTreeIter *iter)
/* Pushes iter on the stack, without deep copy */
{
  GtkTreeIter *dst;

  if (iter==NULL)
    lua_pushnil(L);
  else
  {
    dst = (GtkTreeIter *)lua_newuserdata(L, sizeof(GtkTreeIter));
    luaL_getmetatable(L, "node_ref.type");
    lua_setmetatable(L, -2);
    *dst = *iter;
  }
}

static gboolean internal_is_pseudo_root(GtkTreeIter *iter)
{
	return (iter->stamp==0x1B16BEEF && iter->user_data==NULL);
}

static GtkTreeIter* luaL_check_node_ref_no_pseudo_root(lua_State* L, int p)
{
	GtkTreeIter *iter = luaL_check_node_ref(L,p);
	if (internal_is_pseudo_root(iter))
	{
		luaL_error(L,"This function cannot be applied to the root node.");
		return NULL;
	}
	return iter;
}

static char *internal_bytes_or_string_dup(lua_State* L, int lindex)
{
	bytestring_t *bytes;
	const char* str;
	char *retval;

	if (luaL_is_bytestring(L,lindex))
	{
		bytes = luaL_check_bytestring(L,lindex);
		retval = bytestring_to_format("%S",bytes);
	}
	else if (lua_isstring(L,lindex))
	{
		str = luaL_checkstring(L,lindex);
		retval = g_strdup_printf("t:%s",str);
	}
	else
	{
		luaL_error(L,"Expected either a bytestring or a string, but got %s instead",lua_typename(L,lua_type(L,lindex)));
		return NULL; /* never reached */
	}

	return retval;
}

static void internal_bytes_or_string_push(lua_State* L, const char *src)
{
	if (src)
	{
		switch (src[0]) {
			case 't':
				lua_pushstring(L,src+2);
				break;
			case '8':
			case '4':
			case '1':
				lua_push_bytestring(L,bytestring_new_from_string(src));
				break;
			case 0:
				lua_pushnil(L);
				break;
			default:
				log_printf(LOG_ERROR,"Unrecognized attribute format code (0x%02X) in '%s'",src[0],src);
				luaL_error(L,"Internal error. Please report this bug.");
		}
	}
	else
		lua_pushnil(L);
}

typedef struct {
	unsigned size;
	int *indices;
	const char **names;
	char **values;
} attr_table_t;

static void internal_empty_table(attr_table_t *table)
{
  unsigned u;
	
  for (u=0;u<table->size;u++)
  {
	/* table->names[u] is const, no de-alloc */
	if (table->values[u]) 
		g_free(table->values[u]);
  }
  if (table->indices)
  {
	  g_free(table->indices);
	  table->indices = NULL;
  }
  if (table->values) 
  {
	g_free(table->values);
	table->values = NULL;
  }
  if (table->names) 
  {
	g_free(table->names);
	table->names = NULL;
  }

  table->size=0;
}

static gboolean internal_fill_table(lua_State* L, int lindex, attr_table_t *table)
{
  unsigned u;
  const char *attr_name;
  
  luaL_checktype(L, lindex, LUA_TTABLE);
 
  lua_pushnil(L);
  table->size=0;
  while (lua_next(L,-2)!=0)
  {
	if (!lua_isstring(L,-2)) 
	{
		luaL_error(L,"Expected string for attribute key in table, got %s instead",lua_typename(L,lua_type(L,-2)));
		return FALSE; 
	}

	if (!luaL_is_bytestring(L,-1) && !lua_isstring(L,-1))
	{
		attr_name = lua_tostring(L,-2);
		luaL_error(L,"Expected bytestring or string for value of attribute '%s' in table, got %s instead",attr_name,lua_typename(L,lua_type(L,-1)));
		return FALSE; 	
	}
        lua_pop(L,1);
	table->size++;
  }

  if (table->size==0) return FALSE;

  table->indices = g_malloc0((sizeof(int)*table->size));
  table->names =   g_malloc0((sizeof(char*)*table->size));
  table->values =  g_malloc0((sizeof(char*)*table->size));

  lua_pushnil(L);
  u=0;
  while (lua_next(L,lindex)!=0)
  {
	table->indices[u] = -1;
	table->names[u]   = lua_tostring(L,-2);
        table->values[u]  = internal_bytes_or_string_dup(L,-1);	
        lua_pop(L,1);
 	u++;
  }

  return TRUE;
}

static gboolean internal_index_table(attr_table_t *table)
{
   unsigned u;
   
   for (u=0;u<table->size;u++)
   {
	if ((table->indices[u] = dyntree_model_column_name_to_index(gui_cardview_get_store(),table->names[u]))<0)
		return FALSE;
   }
   return TRUE;
}


/*************************************
 * USER INTERFACE FUNCTIONS
 */

static int subr_nodes_root(lua_State* L)
{
	lua_push_node_ref(L,&PSEUDO_ROOT);

	return 1;
}

static int subr_nodes_append(lua_State* L)
{
	GtkTreeIter *parent;            /*1*/
 	char *classname;
	GtkTreeIter node;
	attr_table_t table;
	unsigned u;

	if (lua_gettop(L)>2)
	{
		lua_pushstring(L, "node.append() takes at most two arguments: a parent and either a classname or a table.");
		lua_error(L);
	}

	parent=luaL_check_node_ref(L,1);

	if (internal_is_pseudo_root(parent)) parent = NULL;

	dyntree_model_iter_append(gui_cardview_get_store(),&node,parent);

	switch (lua_type(L,2)) {
		case LUA_TNONE:
		case LUA_TNIL:
			dyntree_model_iter_attribute_set(gui_cardview_get_store(),&node,CC_CLASSNAME,"t:item");
			break;	
		case LUA_TTABLE:
			if (internal_fill_table(L,2,&table))
			{
				for (u=0;u<table.size;u++)
				{
					dyntree_model_iter_attribute_set_by_name(gui_cardview_get_store(),&node,table.names[u],table.values[u]);	
				}
				internal_empty_table(&table);
			}
			break;
		case LUA_TSTRING:
			classname = g_strdup_printf("t:%s",luaL_checkstring(L,2));	
			dyntree_model_iter_attribute_set(gui_cardview_get_store(),&node,CC_CLASSNAME,classname);
			g_free(classname);
			break;
		default:
			luaL_error(L,"Parameter #2 of node.append() must be a string describing a classname or a table of attributes.");
	}

	lua_push_node_ref(L,&node);
	
	gui_update(1);

	return 1;
}

static int subr_nodes_set_attribute(lua_State* L)
{
  GtkTreeIter *iter;
  const char *attribute_name;
  char *attribute_value;


  iter = luaL_check_node_ref_no_pseudo_root(L,1);
  
  attribute_name  = luaL_checkstring(L,2);

  if (lua_isnoneornil(L,3))
	  attribute_value = NULL;
  else
	  attribute_value = internal_bytes_or_string_dup(L,3);

  if (dyntree_model_iter_attribute_set_by_name(gui_cardview_get_store(),iter,attribute_name,attribute_value))
    lua_push_node_ref(L,iter);
  else
    lua_pushnil(L);

  if (attribute_value) g_free(attribute_value); 
  
  return 1;
}

static int subr_nodes_get_attribute(lua_State* L)
{
  GtkTreeIter *iter;
  const char *attribute_name;
  const char *attribute_value;

  iter  	  = luaL_check_node_ref_no_pseudo_root(L,1);
  attribute_name  = luaL_checkstring(L,2);

  if (dyntree_model_iter_attribute_get_by_name(gui_cardview_get_store(),iter,attribute_name,&attribute_value))
	  internal_bytes_or_string_push(L,attribute_value);
  else
	  lua_pushnil(L);

  return 1;
}

typedef struct {
	GtkTreeIter *iter;
	int index;
	int count;
} attribute_iter_data_t;

static int iter_attribute(lua_State* L)
{
	const char *attr_name;
	const char *attr_value;
	attribute_iter_data_t *attr = (attribute_iter_data_t *)lua_touserdata(L,lua_upvalueindex(1));
 
	while (attr->index<attr->count)
	{
		if (dyntree_model_iter_attribute_get(gui_cardview_get_store(),attr->iter,attr->index,&attr_value))
                {
			if (attr_value)
			{
				attr_name = dyntree_model_column_index_to_name(gui_cardview_get_store(),attr->index++);
				lua_pushstring(L,attr_name);
				internal_bytes_or_string_push(L,attr_value);
				return 2;
			}
		}
		attr->index++;
	}
	return 0;
}

static int subr_nodes_attributes(lua_State* L)
{
	GtkTreeIter *iter = luaL_check_node_ref_no_pseudo_root(L,1);
	attribute_iter_data_t *attr = (attribute_iter_data_t *)lua_newuserdata(L,sizeof(attribute_iter_data_t)); 

	attr->index = 0;
	attr->count = dyntree_model_get_n_columns(GTK_TREE_MODEL(gui_cardview_get_store()));
	attr->iter  = iter;
	
	lua_pushcclosure(L, iter_attribute, 1);
	return 1;
}

typedef struct {
	int 	    has_more;
	GtkTreeIter iter;
} children_iter_data_t;

static int iter_children(lua_State* L)
{
	children_iter_data_t *children = (children_iter_data_t *)lua_touserdata(L,lua_upvalueindex(1));
	
	if (children->has_more)
	{
		lua_push_node_ref(L,&(children->iter));
		children->has_more = gtk_tree_model_iter_next(GTK_TREE_MODEL(gui_cardview_get_store()),&(children->iter));
		return 1;
	}
	return 0;
}


static int subr_nodes_children(lua_State *L)
{
	GtkTreeIter *parent;
	children_iter_data_t *children;
	
	parent = luaL_check_node_ref(L,1);

	if (internal_is_pseudo_root(parent)) parent = NULL;
	
	children = (children_iter_data_t *)lua_newuserdata(L,sizeof(children_iter_data_t));
	children->has_more = gtk_tree_model_iter_children(GTK_TREE_MODEL(gui_cardview_get_store()),&(children->iter),parent);
	
	lua_pushcclosure(L, iter_children, 1);
	return 1;
}

static int subr_nodes_parent(lua_State *L)
{
	GtkTreeIter *iter = luaL_check_node_ref_no_pseudo_root(L,1);
	GtkTreeIter ret;

	if (gtk_tree_model_iter_parent(GTK_TREE_MODEL(gui_cardview_get_store()),&ret,iter))
		lua_push_node_ref(L,&ret);
	else
		lua_pushnil(L);
	return 1;
}


static int subr_nodes_remove(lua_State* L)
{
  GtkTreeIter* iter = luaL_check_node_ref(L,1);

  if (internal_is_pseudo_root(iter)) iter = NULL;

  if (!dyntree_model_iter_remove(gui_cardview_get_store(),iter))
    lua_pushboolean(L,0);
  else
    lua_pushboolean(L,1);
  
  gui_update(1);
  return 1;
}

static int subr_nodes_find_first(lua_State* L)
{
  GtkTreeIter* iter; 
  attr_table_t table;
  GtkTreeIter node;
  
  iter = luaL_check_node_ref(L,1);
  
  if (internal_is_pseudo_root(iter)) iter = NULL;
  

  if (internal_fill_table(L,2,&table))
  {
	if (internal_index_table(&table))
	{

	  if (dyntree_model_iter_find_first(gui_cardview_get_store(),&node,iter,table.indices,table.values,table.size))
		  lua_push_node_ref(L,&node);
	  else
		  lua_pushnil(L);
	}
	else
		lua_pushnil(L);

	internal_empty_table(&table);
  }
  else
	lua_pushnil(L);

  return 1;
}

typedef struct {
	int 	    has_more;
	GtkTreeIter *iter;
	GtkTreeIter node;
	attr_table_t table;
} nodes_iter_data_t;

static int iter_find(lua_State* L)
{
	nodes_iter_data_t *nodes = (nodes_iter_data_t *)lua_touserdata(L,lua_upvalueindex(1));

        if (nodes->has_more)
        {
                lua_push_node_ref(L,&(nodes->node));
                nodes->has_more = dyntree_model_iter_find_next(gui_cardview_get_store(),
								&(nodes->node),
								nodes->iter,
								nodes->table.indices,
								nodes->table.values,
								nodes->table.size);
                return 1;
        }
        return 0;
}

static int iter_find_gc(lua_State *L)
{	
	nodes_iter_data_t *nodes = (nodes_iter_data_t *)lua_touserdata(L,1);

	internal_empty_table(&(nodes->table));
	return 0;
}

static int subr_nodes_find(lua_State* L)
{
	GtkTreeIter *iter;
	nodes_iter_data_t *nodes; 
	attr_table_t table;

	iter = luaL_check_node_ref(L,1);
	
	if (internal_is_pseudo_root(iter)) iter = NULL;

	if (internal_fill_table(L,2,&table))
	{
		if (internal_index_table(&table))
		{
			nodes = (nodes_iter_data_t *)lua_newuserdata(L, sizeof(nodes_iter_data_t));
			luaL_getmetatable(L,"node_iter.type");
			lua_setmetatable(L, -2);

			nodes->table = table;
			nodes->iter = iter;
			nodes->has_more = dyntree_model_iter_find_first(gui_cardview_get_store(),
									&(nodes->node),
									nodes->iter,
									nodes->table.indices,
									nodes->table.values,
									nodes->table.size);
			lua_pushcclosure(L, iter_find, 1);
			return 1;
		}
		internal_empty_table(&table);
	}
	lua_pushnil(L);

	return 1;
}

static int subr_nodes_to_xml(lua_State* L)
{
  GtkTreeIter *iter;
  char *res;
  
  iter = luaL_check_node_ref(L,1);
  if (internal_is_pseudo_root(iter)) iter = NULL;

  res = dyntree_model_iter_to_xml(gui_cardview_get_store(),iter,TRUE);
  if (res==NULL)
  {  
    	lua_pushnil(L);
  }
  else  
  {
    	lua_pushstring(L,res);
    	free(res);
  }
  
  return 1;
}

static int subr_nodes_append_xml(lua_State* L)
{
  GtkTreeIter *iter;
  const char *xml;
  gboolean res;
  
  iter = luaL_check_node_ref(L,1);
  if (internal_is_pseudo_root(iter)) iter = NULL;

  xml = luaL_checkstring(L,2);

  res = dyntree_model_iter_from_xml(gui_cardview_get_store(),iter,TRUE,xml,-1);
  
  lua_pushboolean(L,res);
  
  return 1;
}

static int subr_nodes_tostring(lua_State* L)
{
  GtkTreeIter *iter;
  char buf[20];

  iter = luaL_check_node_ref(L,1);

  if (internal_is_pseudo_root(iter)) 
	  g_sprintf(buf,"@node(ROOT)");
  else
	  g_sprintf(buf,"@node(%p)",iter->user_data);
  lua_pushstring(L,buf);
  return 1;
}


static const struct luaL_Reg nodelib_f [] = {
  {"root", subr_nodes_root },
  {"append", subr_nodes_append },
  {"children", subr_nodes_children },
  {"parent", subr_nodes_parent },
  {"attributes", subr_nodes_attributes },
  {"remove", subr_nodes_remove },
  {"find_first", subr_nodes_find_first },
  {"find", subr_nodes_find },
  {"set_attribute", subr_nodes_set_attribute },
  {"get_attribute", subr_nodes_get_attribute },
  {"to_xml", subr_nodes_to_xml },
  {"append_xml", subr_nodes_append_xml },
  {NULL,NULL}  /* sentinel */
};

static const struct luaL_Reg nodelib_m [] = {
  {"__tostring", subr_nodes_tostring },
  {"append", subr_nodes_append },
  {"children", subr_nodes_children },
  {"parent", subr_nodes_parent },
  {"attributes", subr_nodes_attributes },
  {"remove", subr_nodes_remove },
  {"find_first", subr_nodes_find_first },
  {"find", subr_nodes_find },
  {"set_attribute", subr_nodes_set_attribute },
  {"get_attribute", subr_nodes_get_attribute },
  {"to_xml", subr_nodes_to_xml },
  {"append_xml", subr_nodes_append_xml },
  {NULL,NULL}  /* sentinel */
};

int luaopen_nodes(lua_State* L)
{
  PSEUDO_ROOT.stamp = 0x1B16BEEF;
  PSEUDO_ROOT.user_data = NULL;

  luaL_newmetatable(L, "node_iter.type");
  lua_pushstring(L, "__gc");
  lua_pushcfunction(L, iter_find_gc);
  lua_settable(L,-3);

  luaL_newmetatable(L, "node_ref.type");
  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2);  /* pushes the metatable */
  lua_settable(L, -3);  /* metatable.__index = metatable */
  luaL_setfuncs(L, nodelib_m, 0);
  luaL_newlib(L, nodelib_f);
  lua_setglobal(L,"nodes");
  lua_pop(L,2); /* pop the two metatables */
  return 1;
}

