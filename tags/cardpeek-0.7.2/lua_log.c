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
#include "lua_log.h"
#include "misc.h"

/***********************************************************
 * LOG FUNCTIONS
 */

static int subr_log_print(lua_State* L)
{
  int level = lua_tointeger(L,1);
  const char *message = luaL_checkstring(L,2);
  log_printf(level,"%s",message);
  return 0;
}

static const struct luaL_reg loglib [] = {
  { "print", subr_log_print },
  { NULL, NULL}  /* sentinel */
};

int luaopen_log(lua_State* L)
{
  luaL_openlib(L, "log", loglib, 0);

  lua_getglobal(L, "log");
  lua_pushstring(L,"GOOD");    lua_pushinteger(L, 0); lua_settable(L,-3);
  lua_pushstring(L,"INFO");    lua_pushinteger(L, 1); lua_settable(L,-3);
  lua_pushstring(L,"WARNING"); lua_pushinteger(L, 2); lua_settable(L,-3);
  lua_pushstring(L,"ERROR");   lua_pushinteger(L, 3); lua_settable(L,-3);
  lua_pop(L,1);

  return 1;
}


