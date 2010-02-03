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


cardreader_t* READER=NULL;

void luax_set_card_reader(cardreader_t* r)
{
  READER = r;
}

int subr_card_connect(lua_State* L)
{
  if(!cardreader_connect(READER,PROTOCOL_T0 | PROTOCOL_T1))
    lua_pushboolean(L,0);
  else
    lua_pushboolean(L,1);
  return 1;
}

int subr_card_disconnect(lua_State* L)
{
  cardreader_disconnect(READER);
  
  lua_pushboolean(L,1);
  return 1;
}

int subr_card_warm_reset(lua_State* L)
{
  if (cardreader_warm_reset(READER))
    lua_pushboolean(L,1);
  else
    lua_pushboolean(L,0);
  return 1;
}

int subr_card_last_atr(lua_State* L)
{
  bytestring_t *atr = cardreader_last_atr(READER);

  lua_pushbytestring(L,atr);
  return 1;
}

int subr_card_info(lua_State* L)
{
  char **info=cardreader_get_info(READER);
  int index;

  lua_newtable(L);

  for (index=0;info[index]!=NULL;index+=2)
  {
    lua_pushstring(L,info[index]);
    lua_pushstring(L,info[index+1]);
    lua_settable(L,-3);
    free(info[index]);
    free(info[index+1]);
  }
  free(info);
  return 1;
}

int subr_card_send(lua_State* L)
{
  bytestring_t *command = luaL_checkbytestring(L,1);
  bytestring_t *result = bytestring_new(8);
  unsigned short SW;

  SW=cardreader_transmit(READER,command,result);

  lua_pushinteger(L,SW);
  lua_pushbytestring(L,result);
  return 2;
}

int subr_card_set_command_interval(lua_State* L)
{
  unsigned interval=(unsigned)lua_tointeger(L,1);
  cardreader_set_command_interval(READER,interval);
  return 0;
}

int subr_make_file_path(lua_State* L)
{
  int path_type;
  bytestring_t *file_path = bytestring_new(8);
  const char *path;

  if (lua_isnoneornil(L,1))
    path = "";
  else
    path = lua_tostring(L,1);

  if (iso7816_make_file_path(file_path,&path_type,path)!=ISO7816_OK)
  {
    bytestring_free(file_path);
    lua_pushnil(L);
    lua_pushnil(L);
    log_printf(LOG_DEBUG,"Could not parse path %s",path);
  }
  else {
    lua_pushbytestring(L,file_path);
    lua_pushinteger(L,path_type);
  }
  return 2;
}



static const struct luaL_reg cardlib [] = {
  {"connect", subr_card_connect },
  {"disconnect", subr_card_disconnect },
  {"warm_reset", subr_card_warm_reset },
  {"last_atr", subr_card_last_atr },
  {"info", subr_card_info },
  {"send", subr_card_send },
  {"set_command_interval", subr_card_set_command_interval },
  {"make_file_path", subr_make_file_path },
  {NULL, NULL}  /* sentinel */
};

int luaopen_card(lua_State* L)
{
  luaL_openlib(L, "card", cardlib, 0);

  lua_getglobal(L, "card");
  lua_pushstring(L, "CLA"); lua_pushinteger(L, 0); lua_settable(L,-3);
  lua_pop(L,1);

  return 1;
}

