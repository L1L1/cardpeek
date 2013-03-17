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
#include "lua_bytes.h"
#include "bytestring.h"
#include "misc.h"
#include <string.h>
#include <stdlib.h>

/*** LUA_BYTES: lua encapsulation of bytestring.c ***/

bytestring_t* luaL_checkbytestring(lua_State *L, int p)
{
  void *ud = luaL_checkudata(L,p,"bytes.type");
  luaL_argcheck(L, ud != NULL, p, "`bytes' expected");
  return (bytestring_t *)ud;
}

#ifdef _DEAD_CODE
static void stack_dump(lua_State *L)
{
  unsigned i;


  for (i=1;i<=lua_gettop(L);i++)
  {
    fprintf(stderr,"[%d] %s ", i, lua_typename(L,lua_type(L,i)));
    switch (lua_type(L,i)) {
      case LUA_TUSERDATA:
	fprintf(stderr,"%p\n",lua_touserdata(L,i));
	break;
      case LUA_TSTRING:
	fprintf(stderr,"%s\n",lua_tostring(L,i));
        break;
      default:
	fprintf(stderr,"\n");
    }
  }
}
#endif

static bytestring_t* luaL_newbytestring(lua_State *L, unsigned width)
{
  bytestring_t *bs;
  bs = (bytestring_t *)lua_newuserdata(L, sizeof(bytestring_t));
  if (bytestring_init(bs,width)!=BYTESTRING_OK)
  {
    lua_pop(L,1);
    lua_pushnil(L);
    return NULL;
  }
  luaL_getmetatable(L, "bytes.type");
  lua_setmetatable(L, -2);
  return bs;
}

void lua_pushbytestring(lua_State *L, bytestring_t* bs)
{
  bytestring_t *dst;

  if (bs==NULL)
    lua_pushnil(L);
  else
  {
    dst = (bytestring_t *)lua_newuserdata(L, sizeof(bytestring_t));
    luaL_getmetatable(L, "bytes.type");
    lua_setmetatable(L, -2);
    memcpy(dst,bs,sizeof(bytestring_t));
    free(bs);
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
	tmp = luaL_checkbytestring(L,p);
	bytestring_append(bs,tmp);
	break;
      default:
	luaL_error(L,"wrong type for parameter %d (%s)", p, lua_typename(L,lua_type(L,p)));
	return NULL;
    }
  }
  return bs;
}

static int subr_bytes_new(lua_State *L) 
{
  int width = luaL_checkint(L, 1);
  int n = lua_gettop(L);
  bytestring_t *bs;
  bytestring_t *tmp;

  bs=luaL_newbytestring(L,width);
  if (bs==NULL)
    return 1;
  tmp=x_bytes_create(L,width,2,n);
  bytestring_copy(bs,tmp);
  bytestring_free(tmp);
  return 1;
}

static int subr_bytes_new_from_chars(lua_State *L)
{
  const char *str = luaL_checkstring(L, 1);
  bytestring_t *bs = bytestring_new(8);
  
  bytestring_assign_data(bs,strlen(str),(const unsigned char *)str);

  lua_pushbytestring(L,bs);
  return 1;
}

static int subr_bytes_assign(lua_State *L)
{
  bytestring_t *bs  = luaL_checkbytestring(L, 1);
  bytestring_t *tmp = x_bytes_create(L,bs->width,2,lua_gettop(L));
  lua_pushboolean(L,bytestring_copy(bs,tmp)==BYTESTRING_OK);
  bytestring_free(tmp);
  return 1;
}

static int subr_bytes_append(lua_State *L)
{
  bytestring_t *bs = luaL_checkbytestring(L, 1);
  bytestring_t *tmp = x_bytes_create(L,bs->width,2,lua_gettop(L));
  lua_pushboolean(L,bytestring_append(bs,tmp)==BYTESTRING_OK);
  bytestring_free(tmp);
  return 1;
}

static int subr_bytes_insert(lua_State *L)
{
  bytestring_t *bs = luaL_checkbytestring(L, 1);
  int pos = luaL_checkint(L,2);
  bytestring_t *tmp = x_bytes_create(L,bs->width,3,lua_gettop(L));
  if (pos<0) 
    pos=bytestring_get_size(bs)+pos;
  lua_pushboolean(L,bytestring_insert(bs,pos,tmp)==BYTESTRING_OK);
  bytestring_free(tmp);
  return 1;
}

static int subr_bytes_pad_left(lua_State *L)
{
  bytestring_t *bs = luaL_checkbytestring(L, 1);
  unsigned pad_length = luaL_checkint(L,2);
  unsigned pad_value =  luaL_checkint(L,3);
  lua_pushboolean(L,bytestring_pad_left(bs,pad_length,pad_value)==BYTESTRING_OK);
  return 1;
}  

static int subr_bytes_pad_right(lua_State *L)
{
  bytestring_t *bs = luaL_checkbytestring(L, 1);
  unsigned pad_length = luaL_checkint(L,2);
  unsigned pad_value =  luaL_checkint(L,3);
  lua_pushboolean(L,bytestring_pad_right(bs,pad_length,pad_value)==BYTESTRING_OK);
  return 1;
}  

static int subr_bytes_invert(lua_State *L)
{
  bytestring_t *bs = luaL_checkbytestring(L, 1);
  lua_pushboolean(L,bytestring_invert(bs)==BYTESTRING_OK);
  return 1;
}  

static int subr_bytes_clone(lua_State *L)
{
  bytestring_t *bs = luaL_checkbytestring(L, 1);
  bytestring_t *res = luaL_newbytestring(L,bs->width);
  bytestring_copy(res,bs);
  return 1;
}

static int subr_bytes_sub(lua_State *L)
{
  bytestring_t *bs = luaL_checkbytestring(L, 1);
  int start=luaL_checkint(L,2);
  int end=-1;
  bytestring_t *ret;

  if (lua_gettop(L)>2)
    end=luaL_checkint(L,3);
  if (start<0)
    start=bytestring_get_size(bs)+start;
  if (end<0)
    end=bytestring_get_size(bs)+end;

  ret = luaL_newbytestring(L,bs->width);

  if (bytestring_substr(ret,start,end-start+1,bs)!=BYTESTRING_OK)
  {
    lua_pop(L,1);
    lua_pushnil(L);
  }
  return 1;
}

static int subr_bytes_remove(lua_State *L)
{
  bytestring_t *bs = luaL_checkbytestring(L, 1);
  int start=luaL_checkint(L,2);
  int end=-1;

  if (lua_gettop(L)>2)
    end=luaL_checkint(L,3);
  if (start<0)
    start=bytestring_get_size(bs)+start;
  if (end<0)
    end=bytestring_get_size(bs)+end;

  lua_pushboolean(L,bytestring_erase(bs,start,end-start+1)==BYTESTRING_OK);
  return 1;
}

static int subr_bytes_gc(lua_State *L)
{
  bytestring_t *bs=lua_touserdata(L,1);
  /* fprintf(stderr,"lua is garbage collecting %p\n",(void*)bs); */
  bytestring_release(bs);
  return 0;
}

static int subr_bytes_tostring(lua_State *L) 
{
  bytestring_t *bs = luaL_checkbytestring(L, 1);
  char *s=bytestring_to_format("%D",bs);
  lua_pushstring(L,s);
  free(s);
  return 1;
}

static int subr_bytes_format(lua_State *L)
{
  const char *format = lua_tostring(L,1);
  bytestring_t *bs = luaL_checkbytestring(L,2);
  char *s=bytestring_to_format(format,bs);
  lua_pushstring(L,s);
  free(s);
  return 1;
}

static int subr_bytes_index(lua_State *L)
{
  bytestring_t *bs = luaL_checkbytestring(L, 1);
  int b_index=luaL_checkint(L,2);
  unsigned char c;

  if (bytestring_get_element(&c,bs,b_index)!=BYTESTRING_OK)
  {
    log_printf(LOG_WARNING,"bytestring index must be between 0 and %d",bytestring_get_size(bs)-1);
    return luaL_error(L,"index %d is out of range",b_index);
  }
  lua_pushnumber(L,c);
  return 1;
}

static int subr_bytes_newindex(lua_State *L)
{
  bytestring_t *bs = luaL_checkbytestring(L, 1);
  int b_index=luaL_checkint(L,2);
  unsigned val=luaL_checkint(L,3);

  if (bytestring_set_element(bs,b_index,val)!=BYTESTRING_OK)
    return luaL_error(L,"index %d out of range",b_index);
  lua_pushnumber(L,val);
  return 1;
}

static int subr_bytes_len(lua_State *L)
{
  bytestring_t *bs = luaL_checkbytestring(L, 1);
  lua_pushnumber(L,bytestring_get_size(bs));
  return 1;
}

static int subr_bytes_width(lua_State *L)
{
  bytestring_t *bs = luaL_checkbytestring(L, 1);
  lua_pushnumber(L,bs->width);
  return 1;
}

static int subr_bytes_maxn(lua_State *L)
{
  bytestring_t *bs = luaL_checkbytestring(L, 1);
  if (bytestring_get_size(bs)==0)
    lua_pushnil(L);
  else
    lua_pushnumber(L,bytestring_get_size(bs)-1);
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
    if (lua_isuserdata(L,1))
    {
      bs = luaL_checkbytestring(L, i);
      width = bs->width;
      break;
    }
  }

  /* create the result and push it on the stack */
  bs = x_bytes_create(L,width,1,lua_gettop(L)); 
  lua_pushbytestring(L,bs);
  return 1;
}

static int subr_bytes_eq(lua_State *L)
{
  bytestring_t *a = luaL_checkbytestring(L, 1);
  bytestring_t *b = luaL_checkbytestring(L, 2);

  lua_pushboolean(L,bytestring_is_equal(a,b));
  return 1;
}

static int subr_bytes_is_printable(lua_State *L)
{
  bytestring_t *bs = luaL_checkbytestring(L, 1);

  lua_pushboolean(L,bytestring_is_printable(bs)==BYTESTRING_OK);
  return 1;
}


static int subr_bytes_to_printable(lua_State *L)
{
  bytestring_t *bs = luaL_checkbytestring(L, 1);
  char *ret=bytestring_to_format("%P",bs);
  
  lua_pushstring(L,ret);
  free(ret);
  return 1;
}


static int subr_bytes_convert(lua_State *L)
{
  int base=luaL_checkint(L,1);
  bytestring_t *bs = luaL_checkbytestring(L, 2);
  bytestring_t *ret = luaL_newbytestring(L,base);

  if (bytestring_convert(ret,bs)!=BYTESTRING_OK)
  {
    lua_pop(L,1);
    lua_pushnil(L);
  }
  return 1;
}

static int subr_bytes_to_number(lua_State *L)
{
  bytestring_t *bs = luaL_checkbytestring(L, 1);
  double ret=bytestring_to_number(bs);
  
  lua_pushnumber(L,ret);
  return 1;
}

static const struct luaL_reg byteslib_mt [] = {
  {"__gc",subr_bytes_gc},
  {"__tostring",subr_bytes_tostring},
  {"__index",subr_bytes_index},
  {"__newindex",subr_bytes_newindex},
  {"__len",subr_bytes_len},
  {"__concat",subr_bytes_concat},
  {"__eq", subr_bytes_eq},
  {NULL,NULL}
};

static const struct luaL_reg byteslib [] = {
  {"new", subr_bytes_new},
  {"new_from_chars",subr_bytes_new_from_chars},
  {"assign",subr_bytes_assign},
  {"append",subr_bytes_append},
  {"insert",subr_bytes_insert},
  {"concat",subr_bytes_concat},
  {"pad_left",subr_bytes_pad_left},
  {"pad_right",subr_bytes_pad_right},
  {"invert",subr_bytes_invert},
  {"clone",subr_bytes_clone},
  {"sub",subr_bytes_sub},
  {"remove",subr_bytes_remove},
  {"width",subr_bytes_width},
  {"maxn",subr_bytes_maxn},
  {"is_printable",subr_bytes_is_printable},
  {"toprintable",subr_bytes_to_printable}, 
  {"convert",subr_bytes_convert},
  {"tonumber",subr_bytes_to_number},
  {"format",subr_bytes_format},
  {NULL, NULL}
};

int luaopen_bytes(lua_State* L) 
{
  luaL_newmetatable(L, "bytes.type");
  luaL_openlib(L, NULL, byteslib_mt, 0);
  luaL_openlib(L, "bytes", byteslib, 0);
  return 1;
}


