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
#include "lua_crypto.h"
#include "crypto.h"
#include "lua_bytes.h"

/***********************************************************
 * BASIC CRYPTO FUNCTIONS
 */

static int subr_crypto_create_context(lua_State* L)
{
  int alg           = luaL_checkinteger(L,1);
  bytestring_t* bs;
  bytestring_t* res = bytestring_new(8);
  crypto_error_t e;

  if (lua_isnoneornil(L,2))
    bs = NULL;
  else
    bs  = luaL_check_bytestring(L,2);

  if ((e=crypto_create_context(res,alg,bs))==CRYPTO_OK)
    lua_push_bytestring(L,res);
  else
  {
    bytestring_free(res);
    return luaL_error(L,crypto_stringify_error(e));
  }
  return 1;
}

static int subr_crypto_encrypt(lua_State* L)
{
  bytestring_t* ctx = luaL_check_bytestring(L,1);
  bytestring_t* src = luaL_check_bytestring(L,2);
  bytestring_t* iv;
  bytestring_t* res = bytestring_new(8);
  crypto_error_t e;

  if (lua_isnoneornil(L,3))
    iv = NULL;
  else
    iv = luaL_check_bytestring(L,3);

  if ((e=crypto_encrypt(res,ctx,src,iv))==CRYPTO_OK)
    lua_push_bytestring(L,res);
  else
  {
    bytestring_free(res);
    return luaL_error(L,crypto_stringify_error(e));
  }
  return 1;
}

static int subr_crypto_decrypt(lua_State* L)
{
  bytestring_t* ctx = luaL_check_bytestring(L,1);
  bytestring_t* src = luaL_check_bytestring(L,2);
  bytestring_t* iv;
  bytestring_t* res = bytestring_new(8);
  crypto_error_t e;

  if (lua_isnoneornil(L,3))
    iv = NULL;
  else
    iv = luaL_check_bytestring(L,3);

  if ((e=crypto_decrypt(res,ctx,src,iv))==CRYPTO_OK)
    lua_push_bytestring(L,res);
  else
  {
    bytestring_free(res);
    return luaL_error(L,crypto_stringify_error(e));
  }
  return 1;
}

static int subr_crypto_mac(lua_State* L)
{
  bytestring_t* ctx = luaL_check_bytestring(L,1);
  bytestring_t* src = luaL_check_bytestring(L,2);
  bytestring_t* res = bytestring_new(8);
  crypto_error_t e;

  if ((e=crypto_mac(res,ctx,src))==CRYPTO_OK)
    lua_push_bytestring(L,res);
  else
  {
    bytestring_free(res);
    return luaL_error(L,crypto_stringify_error(e));
  }
  return 1;
}

static int subr_crypto_digest(lua_State* L)
{
  bytestring_t* ctx = luaL_check_bytestring(L,1);
  bytestring_t* src = luaL_check_bytestring(L,2);
  bytestring_t* res = bytestring_new(8);
  crypto_error_t e;

  if ((e=crypto_digest(res,ctx,src))==CRYPTO_OK)
    lua_push_bytestring(L,res);
  else
  {
    bytestring_free(res);
    return luaL_error(L,crypto_stringify_error(e));
  }
  return 1;
}

static const struct luaL_reg cryptolib [] = {
  { "create_context", subr_crypto_create_context },
  { "encrypt", subr_crypto_encrypt },
  { "decrypt", subr_crypto_decrypt },
  { "mac", subr_crypto_mac },
  { "digest", subr_crypto_digest },
  { NULL, NULL}  /* sentinel */
};

struct luaL_reg_integer {
  const char* name;
  int         value;
};

static const struct luaL_reg_integer cryptoconstants [] = {
  { "ALG_DES_ECB", CRYPTO_ALG_DES_ECB },
  { "ALG_DES_CBC", CRYPTO_ALG_DES_CBC },
  { "ALG_DES2_EDE_ECB", CRYPTO_ALG_DES2_EDE_ECB },
  { "ALG_DES2_EDE_CBC", CRYPTO_ALG_DES2_EDE_CBC },
  { "ALG_ISO9797_M3", CRYPTO_ALG_ISO9797_M3 },
  { "ALG_SHA1", CRYPTO_ALG_SHA1 },
  { "PAD_ZERO", CRYPTO_PAD_ZERO },
  { "PAD_OPT_80_ZERO", CRYPTO_PAD_OPT_80_ZERO },
  { "PAD_ISO9797_P2", CRYPTO_PAD_ISO9797_P2 },
  { NULL, 0}  /* sentinel */
};

int luaopen_crypto(lua_State* L)
{
  int i;
  luaL_openlib(L, "crypto", cryptolib, 0);

  lua_getglobal(L, "crypto");
  for (i=0;cryptoconstants[i].name;i++)
  {
    lua_pushstring (L, cryptoconstants[i].name);    
    lua_pushinteger(L, cryptoconstants[i].value); 
    lua_settable(L,-3);
  }
  lua_pop(L,1);

  return 1;
}


