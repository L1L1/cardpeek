/********************************************************************** 
*
* This file is part of Cardpeek, the smart card reader utility.
*
* Copyright 2009-2014 by Alain Pannetrat <L1L1@gmx.com>
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
#include "lua_iconv.h"
#include <iconv.h>
#include "a_string.h"
#include <string.h>
#include <errno.h>
#include "misc.h"
#include "win32/config.h"

static void lua_push_iconv(lua_State* L, iconv_t ic)
{
    iconv_t *ptr = (iconv_t*)lua_newuserdata(L,sizeof(iconv_t));
    *ptr = ic;
    luaL_getmetatable(L, "iconv.type");
    lua_setmetatable(L,-2);
}

static iconv_t luaL_check_iconv(lua_State *L, int p)
{
  void *ud = luaL_checkudata(L,p,"iconv.type");
  luaL_argcheck(L, ud != NULL, p, "`iconv' expected");
  return *(iconv_t *)ud;
}

static int subr_iconv_open(lua_State *L)
{
    const char *fromcode = luaL_checkstring(L,1);
    const char *tocode = luaL_checkstring(L,2);
    iconv_t ic = iconv_open(tocode,fromcode);
   
    if (ic==(iconv_t)(-1))
    {
        lua_pushnil(L);
        return 1;
    }
    lua_push_iconv(L,ic);
    return 1;
}

static int subr_iconv_iconv(lua_State *L)
{
    iconv_t ic = luaL_check_iconv(L,1);
    ICONV_CONST char *src = (ICONV_CONST char *)luaL_checkstring(L,2);
    size_t src_len = strlen(src);
    char conv_block[16];
    size_t conv_len;
    char *conv_block_ptr;
    a_string_t *dst = a_strnew(NULL);
        
    while (src_len>0)
    {
        conv_len = 16;
        conv_block_ptr = conv_block;
        if (iconv(ic, &src, &src_len, &conv_block_ptr, &conv_len)==(size_t)(-1))
        {                    
            if (errno!=E2BIG)            
            {
                a_strfree(dst);
                lua_pushnil(L);
                return 1;
            }
        }
        a_strncat(dst,16-conv_len,conv_block);
    } 
    lua_pushstring(L,a_strval(dst));
    a_strfree(dst);
    return 1;
}

static int subr_iconv_close(lua_State *L)
{
    iconv_t ic = luaL_check_iconv(L,1);

    iconv_close(ic);

    return 0;
}

static const struct luaL_Reg iconvlib_m [] = {
    {"__gc",subr_iconv_close},
    {"open",subr_iconv_open},
    {"iconv",subr_iconv_iconv},
    {NULL,NULL}
};

static const struct luaL_Reg iconvlib_f [] = {
    {"open",subr_iconv_open},
    {"iconv",subr_iconv_iconv},
    {NULL,NULL}
};

int luaopen_iconv(lua_State* L) 
{
  luaL_newmetatable(L, "iconv.type");
  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2);  /* pushes the metatable */
  lua_settable(L, -3);  /* metatable.__index = metatable */
  luaL_setfuncs(L, iconvlib_m, 0);
  luaL_newlib(L, iconvlib_f);
  lua_setglobal(L,"iconv");
  lua_pop(L,1); /* pop the metatable */ 
  return 1;
}



