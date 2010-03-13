/********************************************************************** 
*
* This file is part of Cardpeek, the smartcard reader utility.
*
* Copyright 2009-2010 by 'L1L1'
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
#include "cardtree.h"
#include "misc.h"
#include <string.h>
#include <stdlib.h>
#include "bytestring.h"
#include "lua_bytes.h"

/***********************************************************
 * USER INTERFACE FUNCTIONS
 */

int subr_ui_tree_add_node(lua_State* L)
{
  const char *path;                       /*1*/
  const char *name=lua_tostring(L,2);     /*2*/
  const char *id;                         /*3*/
  int length;                             /*4*/
  const char *comment;                    /*5*/
  const char *res;

  if (lua_isnoneornil(L,1))
    path = NULL;
  else
    path=lua_tostring(L,1);

  if (lua_isnoneornil(L,3))
    id = NULL;
  else
    id = lua_tostring(L,3);

  if (lua_isnoneornil(L,4))
    length = -1;
  else
    length = lua_tointeger(L,4);

  if (lua_isnoneornil(L,5))
    comment = NULL;
  else
    comment=lua_tostring(L,5);

  res = cardtree_add_node(CARDTREE,path,name,id,length,comment);

  if (res!=NULL)
    lua_pushstring(L,res);
  else
    lua_pushnil(L);

  gui_update(1);
  return 1;
}

int subr_ui_tree_set_attribute(lua_State* L)
{
  const char *path;
  const char *attr_list[33]; 
  /* we will limit to 16 settable attributes (33=2*16+1) for the sake of simplicity for now */
  int i;

  path = lua_tostring(L,1);
  
  for (i=0;i<16;i++)
  {
    attr_list[2*i] = lua_tostring(L,2+i);
    if (lua_isnoneornil(L,3+i))
      attr_list[2*i+1] = NULL;
    else
      attr_list[2*i+1] = lua_tostring(L,3+i);
  }
  attr_list[2*i]=NULL;

  if (cardtree_set_attribute(CARDTREE,path,attr_list))
    lua_pushboolean(L,1);
  else
    lua_pushboolean(L,0);

  return 1;
}

int subr_ui_tree_get_attribute(lua_State* L)
{
  const char *path;
  char *attr_list[3];

  path = lua_tostring(L,1);
  attr_list[0] = (char *)lua_tostring(L,2);
  attr_list[1] = NULL;
  attr_list[2] = NULL;
  
  if (cardtree_get_attribute(CARDTREE,path,attr_list))
  {
    lua_pushstring(L,attr_list[1]);
    g_free(attr_list[1]);
  }
  else
    lua_pushnil(L);

  return 1;
}

int subr_ui_tree_set_value(lua_State* L)
{
  const char *path;
  char *value;
  bytestring_t *bs;

  path = lua_tostring(L,1);
  if (lua_isnoneornil(L,2))
    value = NULL;
  else
  {
    bs = luaL_checkbytestring(L,2);
    value = bytestring_to_alloc_string(bs);
  }

  if (cardtree_set_value(CARDTREE,path,value))
    lua_pushboolean(L,1);
  else
    lua_pushboolean(L,0);
  if (value)
    free(value);
  return 1;
}

int subr_ui_tree_get_value(lua_State* L)
{
  const char *path;
  char *value;

  path = lua_tostring(L,1);
  
  if (cardtree_get_value(CARDTREE,path,&value))
  {
    lua_pushbytestring(L,bytestring_new_from_string(8,value));
    g_free(value);
  }
  else
    lua_pushnil(L);

  return 1;
}

int subr_ui_tree_set_alt_value(lua_State* L)
{
  const char *path;
  const char *value;

  path = lua_tostring(L,1);
  if (lua_isnoneornil(L,2))
    value = NULL;
  else
    value = lua_tostring(L,2);

  if (cardtree_set_alt_value(CARDTREE,path,value))
    lua_pushboolean(L,1);
  else
    lua_pushboolean(L,0);
  return 1;
}

int subr_ui_tree_get_alt_value(lua_State* L)
{
  const char *path;
  char *value;

  path = lua_tostring(L,1);
  
  if (cardtree_get_alt_value(CARDTREE,path,&value))
  {
    lua_pushstring(L,value);
    g_free(value);
  }
  else
    lua_pushnil(L);

  return 1;
}

int subr_ui_tree_get_node(lua_State* L)
{
  const char *path;
  char* name;
  char* id;
  int length;
  char* comment;
  int num_children;

  path = lua_tostring(L,1);
  if (!cardtree_get_node(CARDTREE,path,&name,&id,&length,&comment,&num_children))
  {
    lua_pushnil(L);
    return 1;
  }

  lua_newtable(L);

  if (name)
  {
    lua_pushstring(L,"name");
    lua_pushstring(L,name);
    lua_settable(L,-3);
    free(name);
  }

  if (id)
  {
    lua_pushstring(L,"id");
    lua_pushstring(L,id);
    lua_settable(L,-3);
    free(id);
  }
  
  if (length>=0)
  {
    lua_pushstring(L,"length");
    lua_pushinteger(L,length);
    lua_settable(L,-3);
  }

  if (comment)
  {
    lua_pushstring(L,"comment");
    lua_pushstring(L,comment);
    lua_settable(L,-3);
    free(comment);
  }

  lua_pushstring(L,"num_children");
  lua_pushinteger(L,num_children);
  lua_settable(L,-3);

  return 1;
}

int subr_ui_tree_delete_node(lua_State* L)
{
  const char* path = lua_tostring(L,1);
  if (!cardtree_delete_node(CARDTREE,path))
    lua_pushboolean(L,0);
  else
    lua_pushboolean(L,1);
  
  gui_update(1);
  return 1;
}

int subr_ui_find_node(lua_State* L)
{
  const char* path=NULL;
  const char* node=NULL;
  const char* id=NULL;
  const char* retval;

  if (!lua_isnoneornil(L,1))
    path = lua_tostring(L,1);
  if (!lua_isnoneornil(L,2))
    node = lua_tostring(L,2);
  if (!lua_isnoneornil(L,3))
    id = lua_tostring(L,3);
  if ((retval = cardtree_find_node(CARDTREE,path,node,id))!=NULL)
    lua_pushstring(L,retval);
  else
    lua_pushnil(L);
  return 1;
}

int subr_ui_tree_to_xml(lua_State* L)
{
  const char* path;
  char *res;
  
  if (lua_isnoneornil(L,1))
	path = NULL;
  else
  	path = lua_tostring(L,1);
  res = cardtree_to_xml(CARDTREE,path);
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

int subr_ui_tree_save(lua_State* L)
{
  const char *filename;

  if (lua_isnoneornil(L,1))
    return luaL_error(L,"Expecting one parameter: a filename (string)");

  filename= lua_tostring(L,1);

  if (cardtree_to_xml_file(CARDTREE,filename,NULL)==0)
  {
    log_printf(LOG_ERROR,"Could not write xml data to '%s'",filename);
    lua_pushboolean(L,0);
  }
  else
  {
    log_printf(LOG_INFO,"Wrote card data to '%s'",filename);
    lua_pushboolean(L,1);
  }
  return 1;
}

int subr_ui_tree_load(lua_State* L)
{
  const char *filename;
  int retval;

  if (lua_isnoneornil(L,1))
    return luaL_error(L,"Expecting one parameter: a filename (string)");
  
  filename = lua_tostring(L,1);
  retval =  cardtree_from_xml_file(CARDTREE,filename);
  lua_pushboolean(L,retval);
  return 1;
}

int subr_ui_question(lua_State* L)
{
  const char* message;
  const char** items;
  unsigned item_count;
  unsigned i;
  int result;

  if (!lua_isstring(L,1) || !lua_istable(L,2))
    return luaL_error(L,"expecting a string and a table as arguments to this function");

  if (!lua_isnil(L,1))
    message = lua_tostring(L,1);
  else
    message = "";

  item_count = lua_objlen(L,2);
  items = malloc(sizeof(char *)*item_count);
  for (i=0;i<item_count;i++)
  {
    lua_rawgeti(L,2,i+1);
    if (lua_isstring(L,-1))
      items[i] = lua_tostring(L,-1);
    else
      items[i] = "(error)";
    lua_pop(L,1);
  }

  result = gui_question_l(message,item_count,items);
  free(items);

  if (result<0)
    lua_pushnil(L);
  else
    lua_pushinteger(L,result+1);
  return 1;
}
 
int subr_ui_readline(lua_State* L)
{
  const char* message;
  unsigned len;
  const char* default_value;
  char *value;

  if (!lua_isnoneornil(L,1))
    message = lua_tostring(L,1);
  else
    message = "Input";
  if (!lua_isnoneornil(L,2))
    len = lua_tointeger(L,2);
  else
    len = 40;
  if (!lua_isnoneornil(L,3))
    default_value = lua_tostring(L,3);
  else
    default_value = "";

  value = malloc(len+1);
  strncpy(value,default_value,len);
  gui_readline(message,len,value);
  lua_pushstring(L,value);
  free(value);
  return 1;
}

int subr_ui_select_file(lua_State* L)
{
  const char* title;
  const char* path;
  const char* filename;
  char **pair;

  title = lua_tostring(L,1);
  if (!lua_isnoneornil(L,2))
    path = lua_tostring(L,2);
  else
    path = NULL;
  if (!lua_isnoneornil(L,3))
    filename = lua_tostring(L,3);
  else
    filename = NULL;
  pair = gui_select_file(title,path,filename);
  if (pair[0])
  {
    lua_pushstring(L,pair[0]);
    g_free(pair[0]);
  }
  else
    lua_pushnil(L);

  if (pair[1])
  {
    lua_pushstring(L,pair[1]);
    g_free(pair[1]);
  }
  else
    lua_pushnil(L);
  return 2;
}

int subr_ui_about(lua_State* L)
{
  gui_about();
  return 0;
}

static const struct luaL_reg uilib [] = {
  {"tree_add_node", subr_ui_tree_add_node },
  {"tree_set_attribute", subr_ui_tree_set_attribute },
  {"tree_get_attribute", subr_ui_tree_get_attribute },
  {"tree_set_value", subr_ui_tree_set_value },
  {"tree_get_value", subr_ui_tree_get_value },
  {"tree_set_alt_value", subr_ui_tree_set_alt_value },
  {"tree_get_alt_value", subr_ui_tree_get_alt_value },
  {"tree_get_node", subr_ui_tree_get_node },
  {"tree_delete_node", subr_ui_tree_delete_node },
  {"tree_find_node", subr_ui_find_node },
  {"tree_to_xml", subr_ui_tree_to_xml },
  {"tree_save",subr_ui_tree_save },
  {"tree_load",subr_ui_tree_load },
  {"readline",subr_ui_readline },
  {"question",subr_ui_question },
  {"select_file",subr_ui_select_file },
  {"about", subr_ui_about },
  {NULL,NULL}  /* sentinel */
};

int luaopen_ui(lua_State* L)
{
  luaL_openlib(L, "ui", uilib, 0);
  return 1;
}

