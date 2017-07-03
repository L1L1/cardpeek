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
#include "lua_bytes.h"
#include "bytestring.h"
#include "misc.h"
#include <string.h>
#include <stdlib.h>

/*** LUA_BYTES: lua encapsulation of bytestring.c ***/

bytestring_t* luaL_check_bytestring(lua_State *L, int p)
{
  void *ud = luaL_checkudata(L,p,"bytes.type");
  luaL_argcheck(L, ud != NULL, p, "`bytes' expected");
  return *(bytestring_t **)ud;
}

int luaL_is_bytestring(lua_State *L, int idx)
{
	int retval;
	void *p;
	
	if (idx<0) idx=lua_gettop(L)+idx+1;

	if ((p = lua_touserdata(L, idx)) == NULL)
	{	
		return 0;
	}

	luaL_getmetatable(L, "bytes.type");  /* pushes one on stack */

	if (!lua_getmetatable(L, idx))
	{
		lua_pop(L,1);
		return 0;
	}

	if (!lua_rawequal(L, -1, -2))
		retval = 0;
	else
		retval = 1;

	lua_pop(L, 2);  
 
	return retval;
}

void lua_push_bytestring(lua_State *L, bytestring_t* bs)
{
  bytestring_t **dst;

  if (bs==NULL)
    lua_pushnil(L);
  else
  {
    dst = (bytestring_t **)lua_newuserdata(L, sizeof(bytestring_t *)); /* -2 */
    luaL_getmetatable(L, "bytes.type"); /* -1 */
    lua_setmetatable(L, -2);
    *dst = bs;
  }
}

static bytestring_t* x_bytes_create(lua_State *L, unsigned width, int start, int end)
{
  bytestring_t *bs=bytestring_new(width);
  int p;
  bytestring_t *tmp;

  for (p=start;p<=end;p++)
  {
    switch (lua_type(L,p)) {
      case LUA_TSTRING:
	tmp = bytestring_new(width);
	bytestring_assign_digit_string(tmp,lua_tostring(L,p));
	bytestring_append(bs,tmp);
	bytestring_free(tmp);
	break;
      case LUA_TNUMBER:
	bytestring_pushback(bs,lua_tointeger(L,p));
	break;
      case LUA_TUSERDATA:
	tmp = luaL_check_bytestring(L,p);
	bytestring_append(bs,tmp);
	break;
      default:
	luaL_error(L,"wrong type for parameter %d (%s)", p, lua_typename(L,lua_type(L,p)));
	return NULL;
    }
  }
  return bs;
}

/*******************************************************
 *
 * CONSTRUCTORS
 */

static int subr_bytes_new(lua_State *L) 
{
  int width = luaL_checkinteger(L, 1);
  int n = lua_gettop(L);
  bytestring_t *bs;

  bs=x_bytes_create(L,width,2,n);
  lua_push_bytestring(L,bs);
  return 1;
}

static int subr_bytes_new_from_chars(lua_State *L)
{
  const char *str = luaL_checkstring(L, 1);
  bytestring_t *bs = bytestring_new(8);
  
  bytestring_assign_data(bs,strlen(str),(const unsigned char *)str);

  lua_push_bytestring(L,bs);
  return 1;
}

static int subr_bytes_concat(lua_State *L)
{
  int i;
  bytestring_t *bs;
  int width = 8;

  /* find the first bytestring and get its width */
  for (i=1;i<=lua_gettop(L);i++)
  {
    if (lua_isuserdata(L,i))
    {
      bs = luaL_check_bytestring(L, i);
      width = bs->width;
      break;
    }
  }

  /* create the result and push it on the stack */
  bs = x_bytes_create(L,width,1,lua_gettop(L)); 
  lua_push_bytestring(L,bs);
  return 1;
}

/*******************************************************
 *
 * DESTRUCTOR
 */

static int subr_bytes_gc(lua_State *L)
{
  bytestring_t **bs=(bytestring_t **)lua_touserdata(L,1);
	
  if (*bs)
    bytestring_free(*bs);
  return 0;
}

/*******************************************************
 *
 * META-METHODS
 */ 

static int subr_bytes_tostring(lua_State *L) 
{
  bytestring_t *bs = luaL_check_bytestring(L, 1);
  char *s=bytestring_to_format("%D",bs);
  lua_pushstring(L,s);
  free(s);
  return 1;
}

static int subr_bytes_len(lua_State *L)
{
  bytestring_t *bs = luaL_check_bytestring(L, 1);
  lua_pushnumber(L,bytestring_get_size(bs));
  return 1;
}


static int subr_bytes_eq(lua_State *L)
{
  bytestring_t *a = luaL_check_bytestring(L, 1);
  bytestring_t *b = luaL_check_bytestring(L, 2);

  lua_pushboolean(L,bytestring_is_equal(a,b));
  return 1;
}

/******************************************************
 *
 * ITERATOR
 */

typedef struct {
	bytestring_t* bs;
	unsigned      index;
} bytes_iter_data_t;

static int iter_bytes(lua_State *L)
{
	unsigned char c;

	bytes_iter_data_t *iter = (bytes_iter_data_t *)lua_touserdata(L,lua_upvalueindex(1));

	if (iter->index<bytestring_get_size(iter->bs))
	{
		lua_pushnumber(L,iter->index);
		bytestring_get_element(&c,iter->bs,iter->index++);
		lua_pushnumber(L,c);
		return 2;
	}
	return 0;
}

static int subr_bytes_ipairs(lua_State *L)
{
	bytestring_t *bs = luaL_check_bytestring(L, 1);
	bytes_iter_data_t *iter = (bytes_iter_data_t *)lua_newuserdata(L,sizeof(bytestring_t));

	iter->bs = bs;
	iter->index = 0;

	lua_pushcclosure(L, iter_bytes, 1);
        return 1;
}

/********************************************************
 * 
 * ORDINARY METHODS
 */

static int subr_bytes_get(lua_State *L)
{
  bytestring_t *bs = luaL_check_bytestring(L, 1);
  int b_index;
  int e_index;
  int i;
  unsigned char c;

  if (lua_isnoneornil(L,2))
	b_index = 0;
  else
  {
	b_index=luaL_checkinteger(L,2);
	if (b_index<0) b_index = 0;
  }

  if (lua_isnoneornil(L,3))
	e_index = b_index;
  else
  {
	e_index=luaL_checkinteger(L,2);
	if (e_index>=(int)bytestring_get_size(bs)) e_index = bytestring_get_size(bs)-1;
  }

  if (e_index<b_index)
  {
	lua_pushnil(L);
        return 1;
  }

  for (i=b_index;i<=e_index;i++)
  {
	bytestring_get_element(&c,bs,i);
	lua_pushnumber(L,c);
  }
  return e_index-b_index+1;
}

static int subr_bytes_set(lua_State *L)
{
  bytestring_t *bs = luaL_check_bytestring(L, 1);
  unsigned b_index;
  unsigned e_index;
  unsigned i;
  unsigned char c;

  b_index=(unsigned)luaL_checkinteger(L,2);

  e_index = b_index + lua_gettop(L)-3;

  for (i=b_index;i<=e_index;i++)
  {
	  c = (unsigned char)luaL_checkinteger(L,i-b_index+3);
	  if (i<bytestring_get_size(bs))
	  {
		  bytestring_set_element(bs,i,c);
	  }
	  else if (i==bytestring_get_size(bs))
	  {
		  bytestring_pushback(bs,c);
	  }
      else
      {
          char error_msg[100];
          sprintf(error_msg,"Index %i is out of bounds in 'bytes': acceptable range is 0 to %i.", i, bytestring_get_size(bs));
          return luaL_error(L,error_msg);
      }
  }
  lua_settop(L,1);
  return 1;
}



static int subr_bytes_pad_left(lua_State *L)
{
  bytestring_t *bs = bytestring_duplicate(luaL_check_bytestring(L, 1));
  unsigned pad_length = luaL_checkinteger(L,2);
  unsigned pad_value =  luaL_checkinteger(L,3);
  if (bytestring_pad_left(bs,pad_length,pad_value)!=BYTESTRING_OK)
  {
	bytestring_free(bs);
	lua_pushnil(L);
	return 1;
  }
  lua_push_bytestring(L,bs);
  return 1;
}  


static int subr_bytes_pad_right(lua_State *L)
{
  bytestring_t *bs = bytestring_duplicate(luaL_check_bytestring(L, 1));
  unsigned pad_length = luaL_checkinteger(L,2);
  unsigned pad_value =  luaL_checkinteger(L,3);
  if (bytestring_pad_right(bs,pad_length,pad_value)!=BYTESTRING_OK)
  {
	bytestring_free(bs);
	lua_pushnil(L);
	return 1;
  }
  lua_push_bytestring(L,bs);
  return 1;
}  

static int subr_bytes_reverse(lua_State *L)
{
  bytestring_t *bs = bytestring_duplicate(luaL_check_bytestring(L, 1));
  if (bytestring_invert(bs)!=BYTESTRING_OK)
  {
	bytestring_free(bs);
	lua_pushnil(L);
	return 1;
  }
  lua_push_bytestring(L,bs);
  return 1;
}  

static int subr_bytes_clone(lua_State *L)
{
  bytestring_t *bs = luaL_check_bytestring(L, 1);
  bytestring_t *res = bytestring_duplicate(bs);
  lua_push_bytestring(L,res);
  return 1;
}

static int subr_bytes_sub(lua_State *L)
{
  bytestring_t *bs = luaL_check_bytestring(L, 1);
  int start=luaL_checkinteger(L,2);
  int end=-1;
  bytestring_t *ret;

  if (lua_gettop(L)>2)
    end=luaL_checkinteger(L,3);
  if (start<0)
    start=bytestring_get_size(bs)+start;
  if (end<0)
    end=bytestring_get_size(bs)+end;

  ret = bytestring_new(bs->width);

  if (start>end || bytestring_substr(ret,start,end-start+1,bs)!=BYTESTRING_OK)
  {
    bytestring_free(ret);
    lua_pushnil(L);
    return 1;
  }
  
  lua_push_bytestring(L,ret);
  return 1;
}

static int subr_bytes_width(lua_State *L)
{
  bytestring_t *bs = luaL_check_bytestring(L, 1);
  lua_pushnumber(L,bs->width);
  return 1;
}

static int subr_bytes_is_printable(lua_State *L)
{
  bytestring_t *bs = luaL_check_bytestring(L, 1);

  lua_pushboolean(L,bytestring_is_printable(bs)==BYTESTRING_OK);
  return 1;
}

static int subr_bytes_convert(lua_State *L)
{
  static int warn_obsolete=0; 
  bytestring_t *bs;
  int width;
  bytestring_t *ret; 

  if (lua_type(L,2)==LUA_TUSERDATA && lua_type(L,1)==LUA_TNUMBER)
  {
      if (!warn_obsolete)
      {
          warn_obsolete = 1;
          log_printf(LOG_WARNING,"The calling convention of bytes.convert() has changed in cardpeek 0.8, please update your scripts\n"
                                 "\tchange bytes.convert(A,B) to bytes.convert(B,A)\n"
                                 "\tThis warning will only appear once.");
      }
      width=luaL_checkinteger(L,1);
      bs = luaL_check_bytestring(L, 2);
  } 
  else
  {
      bs = luaL_check_bytestring(L, 1);
      width=luaL_checkinteger(L,2);
  }

  if (width!=8 && width!=4 && width!=1)
  	return luaL_error(L,"bytes.convert() only recognizes 8, 4 or 1 as parameter value.");

  ret = bytestring_new(width);

  if (bytestring_convert(ret,bs)!=BYTESTRING_OK)
  {
    bytestring_free(ret);
    lua_pushnil(L);
    return 1;
  }

  lua_push_bytestring(L,ret);
  return 1;
}

static int subr_bytes_to_number(lua_State *L)
{
  bytestring_t *bs = luaL_check_bytestring(L, 1);
  double ret=bytestring_to_number(bs);
  
  lua_pushnumber(L,ret);
  return 1;
}

static int subr_bytes_format(lua_State *L)
{
  static int warn_obsolete=0; 
    bytestring_t *bs;
    const char *format;
    char *s;

    if (lua_type(L,2)==LUA_TUSERDATA && lua_type(L,1)==LUA_TSTRING)
    {
        if (!warn_obsolete)
        {
            warn_obsolete = 1;
            log_printf(LOG_WARNING,"The calling convention of bytes.format() has changed in version 0.8, please update your scripts\n"
                                   "\tchange bytes.format(A,B) to bytes.format(B,A)\n"
                                   "\tThis warning will only appear once.");
        }
        format = lua_tostring(L,1);
        bs = luaL_check_bytestring(L,2);
    } 
    else
    { 
        bs = luaL_check_bytestring(L,1);
        format = lua_tostring(L,2);
    }

    s=bytestring_to_format(format,bs);
    lua_pushstring(L,s);
    free(s);
    return 1;
}

static int subr_bytes_index(lua_State *L)
{
  if (lua_gettop(L)<2)
    return luaL_error(L,"missing arguement to bytes.__index() function."); 

  switch (lua_type(L,2)) 
  {
    case LUA_TNUMBER:
        return subr_bytes_get(L);
    case LUA_TSTRING:
        luaL_getmetatable(L, "bytes.type"); /* st: mt */ 
        lua_pushvalue(L,2);                 /* st: mt, key */
        lua_rawget(L,-2);                   /* st: mt, val */
        lua_remove(L,-2);                   /* st: val */
        return 1;
  } 
  return luaL_error(L,"Invalid index type for bytes.");
}

static int subr_bytes_newindex(lua_State *L)
{
  if (lua_gettop(L)<2)
    return luaL_error(L,"missing arguement to bytes.__newindex() function."); 

  switch (lua_type(L,2))
  {
    case LUA_TNUMBER:
        return subr_bytes_set(L);
    case LUA_TSTRING:
        return luaL_error(L,"The 'bytes' type does not accept new indices.");
  } 
  return luaL_error(L,"Invalid index type for bytes.");
}

/**********************************************
 *
 * LUA REGISTRATION
 */


static const struct luaL_Reg byteslib_m [] = {
  {"__gc",subr_bytes_gc},
  {"__tostring",subr_bytes_tostring},
  {"__len",subr_bytes_len},
  {"__concat",subr_bytes_concat},
  {"__eq", subr_bytes_eq},
  {"__index", subr_bytes_index },
  {"__newindex", subr_bytes_newindex },

  {"ipairs",subr_bytes_ipairs},
  {"get",subr_bytes_get},
  {"set",subr_bytes_set},
  {"pad_left",subr_bytes_pad_left},
  {"pad_right",subr_bytes_pad_right},
  {"reverse",subr_bytes_reverse},
  {"clone",subr_bytes_clone},
  {"sub",subr_bytes_sub},
  {"width",subr_bytes_width},
  {"is_printable",subr_bytes_is_printable},
  {"convert",subr_bytes_convert},
  {"tonumber",subr_bytes_to_number},
  {"format",subr_bytes_format},
  {NULL,NULL}
};

static const struct luaL_Reg byteslib_f [] = {
  {"new", subr_bytes_new},
  {"new_from_chars",subr_bytes_new_from_chars},
  {"concat", subr_bytes_concat},

  {"ipairs",subr_bytes_ipairs},
  {"get",subr_bytes_get},
  {"set",subr_bytes_set},
  {"pad_left",subr_bytes_pad_left},
  {"pad_right",subr_bytes_pad_right},
  {"reverse",subr_bytes_reverse},
  {"clone",subr_bytes_clone},
  {"sub",subr_bytes_sub},
  {"width",subr_bytes_width},
  {"is_printable",subr_bytes_is_printable},
  {"convert",subr_bytes_convert},
  {"tonumber",subr_bytes_to_number},
  {"format",subr_bytes_format},
  {NULL, NULL}
};

int luaopen_bytes(lua_State* L) 
{
  luaL_newmetatable(L, "bytes.type");
  lua_pushstring(L, "__index");
  lua_pushvalue(L, -2);  /* pushes the metatable */
  lua_settable(L, -3);  /* metatable.__index = metatable */
  luaL_setfuncs(L, byteslib_m, 0);
  luaL_newlib(L, byteslib_f);
  lua_setglobal(L,"bytes");
  lua_pop(L,1); /* pop the metatable */ 
  return 1;
}


