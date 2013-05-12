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
#include <lauxlib.h>
#include "lua_ui.h"
#include "gui.h"
#include "gui_about.h"
#include "gui_cardview.h"
#include "dyntree_model.h"
#include "misc.h"
#include <string.h>
#include <stdlib.h>
#include "bytestring.h"
#include "lua_bytes.h"
#include <glib.h>
#include <glib/gprintf.h>

static int subr_ui_save_view(lua_State* L)
{
  const char *filename;

  if (lua_isnoneornil(L,1))
    return luaL_error(L,"Expecting one parameter: a filename (string)");

  filename= lua_tostring(L,1);

  if (dyntree_model_iter_to_xml_file(gui_cardview_get_store(),NULL,filename)==0)
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

static int subr_ui_load_view(lua_State* L)
{
  const char *filename;
  int retval;

  if (lua_isnoneornil(L,1))
    return luaL_error(L,"Expecting one parameter: a filename (string)");

  filename = lua_tostring(L,1);

  if (strcmp(filename_extension(filename),".lua")==0)
  {
    log_printf(LOG_WARNING,"%s seems to be a card LUA script: perhaps you should use the 'Analyzer'"
        " menu instead to open this file.",filename_base(filename));
  }

  retval =  dyntree_model_iter_from_xml_file(gui_cardview_get_store(),NULL,filename);
  
  lua_pushboolean(L,retval);
  return 1;
}

static int subr_ui_question(lua_State* L)
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
 
static int subr_ui_readline(lua_State* L)
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

  value = g_malloc(len+1);
  strncpy(value,default_value,len);
  
  if (gui_readline(message,len,value)) 
	 lua_pushstring(L,value);
  else
	 lua_pushnil(L);
  g_free(value);
  return 1;
}

static int subr_ui_select_file(lua_State* L)
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

static int subr_ui_about(lua_State* L)
{
  UNUSED(L);
  gui_about();
  return 0;
}

static int subr_ui_exit(lua_State* L)
{
  UNUSED(L);
  gui_exit(); 
  return 0;
}

static const struct luaL_reg uilib [] = {
  {"save_view",subr_ui_save_view },
  {"load_view",subr_ui_load_view },
  {"readline",subr_ui_readline },
  {"question",subr_ui_question },
  {"select_file",subr_ui_select_file },
  {"about", subr_ui_about },
  {"exit", subr_ui_exit },
  {NULL,NULL}  /* sentinel */
};

int luaopen_ui(lua_State* L)
{
  luaL_openlib(L, "ui", uilib, 0);
  return 1;
}

