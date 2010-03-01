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
#include "lua_asn1.h"
#include "asn1.h"
#include "lua_bytes.h"
#include "misc.h"

int subr_asn1_split(lua_State* L)
{
  const bytestring_t *tlv = luaL_checkbytestring(L,1);
  unsigned tag;
  bytestring_t *value = bytestring_new(8);
  bytestring_t *tail  = bytestring_new(8);
  unsigned pos=0;

  if (asn1_decode_tlv(&pos,tlv,&tag,value)==BYTESTRING_OK)
  {
    lua_pushinteger(L,tag);
    
    lua_pushbytestring(L,value);
    
    if (pos<bytestring_get_size(tlv))
    {
      bytestring_substr(tail,pos,BYTESTRING_NPOS,tlv);
      lua_pushbytestring(L,tail);
    }
    else
    {
      bytestring_free(tail);
      lua_pushnil(L);
    }
  }
  else
  {
    bytestring_free(value);
    bytestring_free(tail);
    lua_pushnil(L);
    lua_pushnil(L);
    lua_pushnil(L);
  }
  return 3;
}

int subr_asn1_split_tag(lua_State* L)
{
  const bytestring_t *tlv = luaL_checkbytestring(L,1);
  unsigned tag;
  bytestring_t *tail  = bytestring_new(8);
  unsigned pos=0;

  if (asn1_decode_tag(&pos,tlv,&tag)==BYTESTRING_OK)
  {
    lua_pushinteger(L,tag);
    
    if (pos<bytestring_get_size(tlv))
    {
      bytestring_substr(tail,pos,BYTESTRING_NPOS,tlv);
      lua_pushbytestring(L,tail);
    }
    else
    {
      bytestring_free(tail);
      lua_pushnil(L);
    }
  }
  else
  {
    bytestring_free(tail);
    lua_pushnil(L);
    lua_pushnil(L);
  }
  return 2;
}

int subr_asn1_split_length(lua_State* L)
{
  const bytestring_t *tlv = luaL_checkbytestring(L,1);
  unsigned len;
  bytestring_t *tail  = bytestring_new(8);
  unsigned pos=0;

  if (asn1_decode_length(&pos,tlv,&len)==BYTESTRING_OK)
  {
    lua_pushinteger(L,len);
    
    if (pos<bytestring_get_size(tlv))
    {
      bytestring_substr(tail,pos,BYTESTRING_NPOS,tlv);
      lua_pushbytestring(L,tail);
    }
    else
    {
      bytestring_free(tail);
      lua_pushnil(L);
    }
  }
  else
  {
    bytestring_free(tail);
    lua_pushnil(L);
    lua_pushnil(L);
  }

  return 2;
}


int subr_asn1_join(lua_State* L)
{
  unsigned tag;
  bytestring_t *value;
  bytestring_t *tail;
  bytestring_t *tlv = bytestring_new(8);
  int clear_value=0;
  int clear_tail=0;

  tag = (unsigned) lua_tointeger(L,1);
  if (lua_isnoneornil(L,2))
    value = NULL;
  else
  {
    if (lua_type(L,2)==LUA_TSTRING)
    {
      value = bytestring_new_from_string(8,lua_tostring(L,2));
      clear_value = (value!=NULL);
    }
    else
      value = luaL_checkbytestring(L,2);
  }
 
  asn1_encode_tlv(tag,value,tlv);

  if (!lua_isnoneornil(L,3))
  {
    if (lua_type(L,3)==LUA_TSTRING)
    {
      tail = bytestring_new_from_string(8,lua_tostring(L,3));
      clear_tail = (tail!=NULL);
    }
    else
      tail = luaL_checkbytestring(L,3);
    bytestring_append(tlv,tail);
  }

  if (clear_value) bytestring_free(value);
  if (clear_tail) bytestring_free(tail);
  lua_pushbytestring(L,tlv);
  return 1;
}

int subr_asn1_enable_single_byte_length(lua_State* L)
{
  int enable = lua_toboolean(L,1);
  asn1_force_single_byte_length_parsing(enable);
  if (enable)
    log_printf(LOG_INFO,"Single byte length ASN1 decoding enabled");
  else
    log_printf(LOG_INFO,"Single byte length ASN1 decoding disabled");
  return 0;
}


static const struct luaL_reg asn1lib [] = {
  {"join",                       subr_asn1_join },
  {"split",                      subr_asn1_split },
  {"split_tag",                  subr_asn1_split_tag },
  {"split_length",               subr_asn1_split_length},
  {"enable_single_byte_length",  subr_asn1_enable_single_byte_length },
  {NULL, NULL} /* sentinel */
};

int luaopen_asn1(lua_State* L)
{
  luaL_openlib(L,"asn1",asn1lib,0);
  return 1;
}
