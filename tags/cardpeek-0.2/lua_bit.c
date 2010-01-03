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

/***********************************************************
 * GENERAL FUNCTIONS
 * - Binary integer ops that LUA lacks 
 */

int subr_bit_and(lua_State* L)
{
  unsigned a = (unsigned)lua_tointeger(L,1);
  unsigned b = (unsigned)lua_tointeger(L,2);
  lua_pushinteger(L,(int)(a&b));
  return 1;
}

int subr_bit_or(lua_State* L)
{
  unsigned a = (unsigned)lua_tointeger(L,1);
  unsigned b = (unsigned)lua_tointeger(L,2);
  lua_pushinteger(L,(int)(a|b));
  return 1;
}

int subr_bit_xor(lua_State* L)
{
  unsigned a = (unsigned)lua_tointeger(L,1);
  unsigned b = (unsigned)lua_tointeger(L,2);
  lua_pushinteger(L,(int)(a^b));
  return 1;
}

int subr_bit_shl(lua_State* L)
{
  unsigned a = (unsigned)lua_tointeger(L,1);
  unsigned b = (unsigned)lua_tointeger(L,2);
  lua_pushinteger(L,(int)((a<<b)&0xFFFFFFFF));
  return 1;
}

int subr_bit_shr(lua_State* L)
{
  unsigned a = (unsigned)lua_tointeger(L,1);
  unsigned b = (unsigned)lua_tointeger(L,2);
  lua_pushinteger(L,(int)(a>>b));
  return 1;
}

static const struct luaL_reg bitlib [] = {
  { "AND", subr_bit_and },
  { "OR", subr_bit_or },
  { "XOR", subr_bit_xor },
  { "SHL", subr_bit_shl },
  { "SHR", subr_bit_shr },
  { NULL, NULL }  /* sentinel */
};

int luaopen_bit(lua_State* L)
{
  luaL_openlib(L, "bit", bitlib, 0);
  return 1;
}


