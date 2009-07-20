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

#include "lua_ext.h"
#include <stdlib.h>
#include <stdio.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "smartcard.h"
#include "misc.h"
#include "asn1.h"
#include "gui.h"
#include "config.h"

cardreader_t* READER=NULL;

/*******************************************/

bytestring_t* lua_tobytestring(lua_State* L, int pos)
{
  return bytestring_new_from_hex(lua_tostring(L,pos));
}

void lua_pushbytestring(lua_State* L, const bytestring_t* bs)
{
  char *tmp=bytestring_to_hex_string(bs);
  lua_pushstring(L, tmp);
  free(tmp);
}

/*******************************************/

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
  bytestring_free(atr);
  return 1;
}

int subr_card_status(lua_State* L)
{
  /*
  cardInformation cri = READER.information();
  int index;

  lua_newtable(L);

  lua_pushstring(L,"reader");
  lua_pushstring(L,cri.cri_name.c_str());
  lua_settable(L,-3);

  lua_pushstring(L,"state");
  lua_newtable(L);
  index=1;
  if (cri.cri_state&SCARD_ABSENT)
  {
    lua_pushinteger(L,index++);
    lua_pushstring(L,"absent");
    lua_settable(L,-3);
  }

  if (cri.cri_state&SCARD_PRESENT)
  {
    lua_pushinteger(L,index++);
    lua_pushstring(L,"present");
    lua_settable(L,-3);
  }

  if (cri.cri_state&SCARD_SWALLOWED)
  {
    lua_pushinteger(L,index++);
    lua_pushstring(L,"swallowed");
    lua_settable(L,-3);
  }

  if (cri.cri_state&SCARD_POWERED)
  {
    lua_pushinteger(L,index++);
    lua_pushstring(L,"powered");
    lua_settable(L,-3);
  }

  if (cri.cri_state&SCARD_NEGOTIABLE)
  {
    lua_pushinteger(L,index++);
    lua_pushstring(L,"negotiable");
    lua_settable(L,-3);
  }

  if (cri.cri_state&SCARD_SPECIFIC)
  {
    lua_pushinteger(L,index++);
    lua_pushstring(L,"specific");
    lua_settable(L,-3);
  }

  if (cri.cri_state&SCARD_ABSENT)
  {
    lua_pushinteger(L,index++);
    lua_pushstring(L,"absent");
    lua_settable(L,-3);
  }
  lua_settable(L,-3);

  lua_pushstring(L,"protocol");
  if (cri.cri_protocol==SCARD_PROTOCOL_T0) lua_pushstring(L,"T0");
  else if (cri.cri_protocol==SCARD_PROTOCOL_T1) lua_pushstring(L,"T1");
  else lua_pushstring(L,"unknown");
  lua_settable(L,-3);

  lua_pushstring(L,"atr");
  lua_pushbytestring(L,cri.cri_atr);
  lua_settable(L,-3);
  return 1;
  */
	  return 0;
}

int subr_card_send(lua_State* L)
{
  bytestring_t *command = lua_tobytestring(L,1);
  bytestring_t *result = bytestring_new();
  unsigned short SW;

  SW=cardreader_transmit(READER,command,result);

  lua_pushinteger(L,SW);
  lua_pushbytestring(L,result);
  bytestring_free(command);
  bytestring_free(result);
  return 2;
}

int subr_bytes_pack(lua_State* L)
{
  bytestring_t *bs=bytestring_new();

  lua_pushnil(L);  /* first key */
  while (lua_next(L, 1) != 0) {
    /* 'key' is at index -2 and 'value' at index -1 */
    bytestring_pushback(bs,lua_tointeger(L,-1)&0xFF);
    lua_pop(L, 1);  /* removes 'value'; keeps 'key' for next iteration */
  }

  lua_pushbytestring(L,bs);
  bytestring_free(bs);
  return 1;
}

int subr_bytes_unpack(lua_State* L)
{
  bytestring_t *bs = lua_tobytestring(L,1);
  unsigned len = bytestring_get_size(bs);
  unsigned index;

  lua_newtable(L);
  for (index=0;index<len;index++)
  {
    lua_pushinteger(L,index+1);
    lua_pushinteger(L,bytestring_get_uchar(bs,index));
    lua_settable(L,-3);
  }
  bytestring_free(bs);
  return 1;
}

int subr_bytes_substr(lua_State* L)
{
  bytestring_t *bs = lua_tobytestring(L,1);
  unsigned pos = (unsigned)lua_tointeger(L,2);
  unsigned len;
  bytestring_t *res = bytestring_new();

  if (lua_isnil(L,3))
    len = BYTESTRING_NPOS;
  else
    len = (unsigned)lua_tointeger(L,3);

  /*log_printf(LOG_INFO,"Calling substr %i %i",pos,len);
  */
  bytestring_substr(bs,pos,len,res);

  /*log_printf(LOG_INFO,"Got %s",bytestring::hex::output(res).c_str());
  */
  lua_pushbytestring(L,res);
  bytestring_free(bs);
  bytestring_free(res);
  return 1;
}

int subr_bytes_size(lua_State* L)
{
  bytestring_t *bs = lua_tobytestring(L,1);
  int length = bytestring_get_size(bs);

  lua_pushinteger(L,length);
  bytestring_free(bs);
  return 1;
}


/***********************************************************/

int subr_tlv_split(lua_State* L)
{
  bytestring_t *tlv = lua_tobytestring(L,1);
  unsigned tag;
  bytestring_t *value = bytestring_new();
  unsigned index=1;
  unsigned pos=0;

  lua_newtable(L);
  while (pos<bytestring_get_size(tlv))
  {
    if (asn1_decode_tlv(&pos,tlv,&tag,value))
    {
      lua_pushinteger(L,index++);
      lua_newtable(L);

      lua_pushinteger(L,1);
      lua_pushinteger(L,tag);
      lua_settable(L,-3);

      lua_pushinteger(L,2);
      lua_pushbytestring(L,value);
      lua_settable(L,-3);

      lua_settable(L,-3);
    }
  }
  bytestring_free(tlv);
  bytestring_free(value);
  return 1;
}


int subr_tlv_make(lua_State* L)
{
  unsigned tag;
  bytestring_t *value;
  bytestring_t *tlv = bytestring_new();
  
  tag = (unsigned) lua_tointeger(L,1);
  value = lua_tobytestring(L,2);
  
  asn1_encode_tlv(tag,value,tlv);
  lua_pushbytestring(L,tlv);

  bytestring_free(value);
  bytestring_free(tlv);
  return 1;
}

int subr_log(lua_State* L)
{
  int level = lua_tointeger(L,1);
  const char *message = lua_tostring(L,2);
  log_printf(level,"%s",message);
  return 0;
}


int subr_tlv_enable_single_byte_length(lua_State* L)
{
  int enable = lua_toboolean(L,1);
  asn1_force_single_byte_length_parsing(enable);
  /*log_printf(LOG_INFO,"");*/
  return 0;
}


/***********************************************************/

int subr_ui_tree_append(lua_State* L)
{
  const char *path;                       /*1*/
  int leaf=lua_toboolean(L,2);            /*2*/
  const char *node=lua_tostring(L,3);     /*3*/
  const char *id;                         /*4*/
  int length;                             /*5*/
  const char *comment;                    /*6*/
  const char *res;

  if (lua_isnil(L,1))
    path = NULL;
  else
    path=lua_tostring(L,1);

  if (lua_isnil(L,4))
    id = NULL;
  else
    id = lua_tostring(L,4);

  if (lua_isnil(L,5))
    length = -1;
  else
    length = lua_tointeger(L,5);

  if (lua_isnil(L,6))
    comment = NULL;
  else
    comment=lua_tostring(L,6);

  res=cardtree_append(CARDTREE,path,leaf,node,id,length,comment);

  if (res!=NULL)
    lua_pushstring(L,res);
  else
    lua_pushnil(L);

  gui_update();
  return 1;
}

int subr_ui_tree_get(lua_State* L)
{
  const char *path;
  int leaf;
  char* node;
  char* id;
  int length;
  char* comment;

  path = lua_tostring(L,1);
  if (!cardtree_get(CARDTREE,path,&leaf,&node,&id,&length,&comment))
  {
    lua_pushnil(L);
    return 1;
  }

  lua_newtable(L);

  lua_pushstring(L,"leaf");
  lua_pushboolean(L,leaf);
  lua_settable(L,-3);

  if (node)
  {
    lua_pushstring(L,"node");
    lua_pushstring(L,node);
    lua_settable(L,-3);
    free(node);
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

  return 1;
}

int subr_ui_tree_delete(lua_State* L)
{
  const char* path = lua_tostring(L,1);
  if (!cardtree_delete(CARDTREE,path))
    lua_pushboolean(L,0);
  else
    lua_pushboolean(L,1);
  
  gui_update();
  return 1;
}

int subr_ui_find(lua_State* L)
{
  const char* path=NULL;
  const char* node=NULL;
  const char* id=NULL;
  const char* retval;

  if (!lua_isnil(L,1))
    path = lua_tostring(L,1);
  if (!lua_isnil(L,2))
    node = lua_tostring(L,2);
  if (!lua_isnil(L,3))
    id = lua_tostring(L,3);
  if ((retval = cardtree_find(CARDTREE,path,node,id))!=NULL)
    lua_pushstring(L,retval);
  else
    lua_pushnil(L);
  return 1;
}

int subr_ui_tree_to_xml(lua_State* L)
{
  const char* path;
  char *res;
  
  if (lua_isnil(L,1))
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
  const char *filename = lua_tostring(L,1);

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
  const char *filename = lua_tostring(L,1);
  int retval =  cardtree_from_xml_file(CARDTREE,filename);
  lua_pushboolean(L,retval);
  return 1;
}
 

/***********************************************************/

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

int subr_usleep(lua_State* L)
{
  unsigned u = (unsigned)lua_tointeger(L,1);
  usleep(u);
  return 0;
}

/***********************************************************/

int print_debug_message(lua_State* L)
{
  const char* err_msg = lua_tostring(L,1);
  lua_Debug ar;

  log_printf(LOG_ERROR,"(DEBUG) %s",err_msg);

  lua_getstack(L,1,&ar);
  if (lua_getinfo(L,"n",&ar))
    log_printf(LOG_ERROR,"Called from %s",ar.name);
  else
    log_printf(LOG_ERROR,"No further information available");

  return 0;
}

unsigned line_num = 0;
char line_buf[1024];

const char* read_chunk(lua_State* L, void* input, size_t* sz)
{
  if (fgets(line_buf,1024,(FILE*)input)==NULL)
  {
    *sz=0;
    return NULL;
  }
  line_num++;
  *sz=strlen(line_buf);
  return line_buf;
}

int run_file(lua_State* L, const char *filename)
{
  FILE* input = fopen(filename,"r");
  if (input==NULL)
  {
    log_printf(LOG_ERROR,"Could not load %s (%s)",filename,strerror(errno));
    return 0;
  }
  line_num = 0;
  lua_pushcfunction(L,print_debug_message);
  if (lua_load(L,read_chunk,input,"cardfile")!=0)
  {
    log_printf(LOG_ERROR,"Syntax error on line %i in %s",line_num,filename);
    return 0;
  }
  fclose(input);
  if (lua_pcall(L, 0, LUA_MULTRET, -2)!=0)
  {
    log_printf(LOG_ERROR,"Runtime error in %s",filename);
    lua_pop(L,1);
    return 0;
  }
  lua_pop(L,1);
  return 1;
}

void run_card_shell(lua_State* L)
{
  int error;
  char inputline[1024];

  printf("cardpeek> ");
  while (fgets(inputline, sizeof(inputline), stdin) != NULL) {
    if (strncmp(inputline,"quit()",6)==0)
      break;
    error = luaL_loadbuffer(L, inputline, strlen(inputline), "cardpeek") ||
	    lua_pcall(L, 0, 0, 0);
    if (error) {
      fprintf(stderr, "%s\n", lua_tostring(L, -1));
      lua_pop(L, 1);  /* pop error message from the stack */
    }
    printf("cardpeek> ");
  }
  fprintf(stderr,"bye.\n");
}

/****************************************************/


static const struct luaL_reg cardlib [] = {
  {"connect", subr_card_connect },
  {"disconnect", subr_card_disconnect },
  {"warm_reset", subr_card_warm_reset },
  {"last_atr", subr_card_last_atr },
  {"status", subr_card_status },
  {"send", subr_card_send },
  {"bytes_pack", subr_bytes_pack },
  {"bytes_unpack", subr_bytes_unpack },
  {"bytes_substr", subr_bytes_substr },
  {"bytes_size", subr_bytes_size },
  {"tlv_make", subr_tlv_make },
  {"tlv_split", subr_tlv_split },
  {"tlv_enable_single_byte_length", subr_tlv_enable_single_byte_length },
  {NULL, NULL}  /* sentinel */
};

static const struct luaL_reg uilib [] = {
  {"tree_append", subr_ui_tree_append },
  {"tree_delete", subr_ui_tree_delete },
  {"tree_get", subr_ui_tree_get },
  {"tree_to_xml", subr_ui_tree_to_xml },
  {"tree_find", subr_ui_find },
  {"tree_save",subr_ui_tree_save },
  {"tree_load",subr_ui_tree_load },
  {NULL,NULL}  /* sentinel */
};

static const struct luaL_reg loglib [] = {
  { "print", subr_log },
  { NULL, NULL}  /* sentinel */
};

static const struct luaL_reg globalfuncs [] = {
  { "bit_and", subr_bit_and },
  { "bit_or", subr_bit_or },
  { "bit_xor", subr_bit_xor },
  { "bit_shl", subr_bit_shl },
  { "bit_shr", subr_bit_shr },
  { "usleep", subr_usleep },
  { NULL, NULL }  /* sentinel */
};

/***************************************************/

lua_State* LUA_STATE = NULL;


void luax_set_card_reader(cardreader_t* r)
{
  READER = r;
}

void luax_run_script_cb(const char* scriptname)
{
  chdir(config_get_string(CONFIG_PATH_SCRIPT));
  log_printf(LOG_INFO,"Running script %s",scriptname);
  run_file(LUA_STATE,scriptname);
}

void luax_run_command_cb(const char* command)
{
  int error;
  
  chdir(config_get_string(CONFIG_PATH_SCRIPT));
  log_printf(LOG_INFO,"Exec command '%s'",command);

  error = luaL_loadbuffer(LUA_STATE, command, strlen(command), "cardman") ||
	  lua_pcall(LUA_STATE, 0, 0, 0);
  if (error) {
    log_printf(LOG_ERROR,"%s", lua_tostring(LUA_STATE, -1));
    lua_pop(LUA_STATE, 1);  /* pop error message from the stack */
  }
}

int luax_init()
{
  int i;
  const char *config_lua = config_get_string(CONFIG_FILE_CONFIG);

  log_printf(LOG_DEBUG,"Loading lua...");
  
  LUA_STATE = lua_open();

  luaL_openlibs(LUA_STATE);
  luaL_register(LUA_STATE, "card", cardlib);
  luaL_register(LUA_STATE, "log", loglib);
  luaL_register(LUA_STATE, "ui", uilib);
  for (i=0;globalfuncs[i].name;i++) lua_register(LUA_STATE,globalfuncs[i].name,globalfuncs[i].func);

  lua_getglobal(LUA_STATE, "card");
  lua_pushstring(LUA_STATE, "CLA"); lua_pushinteger(LUA_STATE, 0); lua_settable(LUA_STATE,-3);
  lua_pop(LUA_STATE,1);

  lua_getglobal(LUA_STATE, "log");
  lua_pushstring(LUA_STATE,"GOOD");    lua_pushinteger(LUA_STATE, 0); lua_settable(LUA_STATE,-3);
  lua_pushstring(LUA_STATE,"INFO");    lua_pushinteger(LUA_STATE, 1); lua_settable(LUA_STATE,-3);
  lua_pushstring(LUA_STATE,"WARNING"); lua_pushinteger(LUA_STATE, 2); lua_settable(LUA_STATE,-3);
  lua_pushstring(LUA_STATE,"ERROR");   lua_pushinteger(LUA_STATE, 3); lua_settable(LUA_STATE,-3);
  lua_pop(LUA_STATE,1);

  run_file(LUA_STATE,config_lua);

  log_printf(LOG_DEBUG,"Lua is loaded");
  return 1;
}

void luax_release()
{
  lua_close(LUA_STATE);
}


