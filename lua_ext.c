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

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <glib/gstdio.h>
#include "pathconfig.h"
#include "iso7816.h"
#include "misc.h"
#include "lua_ext.h"

/****************************************/

#include "lua_log.h"
#include "lua_bytes.h"
#include "lua_asn1.h"
#include "lua_card.h"
#include "lua_bit.h"
#include "lua_crypto.h"
#include "lua_ui.h" 
#include "lua_nodes.h" 


/****************************************/
/* FIXME: this whole file needs cleanup */
/****************************************/

static int print_debug_message(lua_State* L)
{
  const char* err_msg = lua_tostring(L,1);
  lua_Debug ar;
  int depth = 1;

  log_printf(LOG_ERROR,"%s",err_msg);

  while (lua_getstack(L,depth++,&ar))
  {
	  if (lua_getinfo(L,"lnS",&ar))
	  {
		  if (ar.name)
		  {
			if (ar.currentline>=0)
				log_printf(LOG_ERROR,"backtrace: called from %s() in %s[%i]",ar.name,ar.short_src,ar.currentline);
			else
				log_printf(LOG_ERROR,"backtrace: called from %s()",ar.name);
		  }
		  else
			  log_printf(LOG_ERROR,"backtrace: called at %s",ar.short_src);
	  }
	  else
		  log_printf(LOG_ERROR,"backtrace: no further information available");
  }

  return 0;
}

unsigned line_num = 0;
char line_buf[1024];

static const char* read_chunk(lua_State* L, void* input, size_t* sz)
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

static int run_file(lua_State* L, const char *filename)
{
  FILE* input = g_fopen(filename,"r");
  if (input==NULL)
  {
    log_printf(LOG_ERROR,"Could not load %s (%s)",filename,strerror(errno));
    return 0;
  }
  line_num = 0;
  lua_pushcfunction(L,print_debug_message);
  if (lua_load(L,read_chunk,input,filename)!=0)
  {
    log_printf(LOG_ERROR,"Syntax error on line %i in %s",line_num,filename);
    return 0;
  }
  fclose(input);
  if (lua_pcall(L, 0, LUA_MULTRET, -2)!=0)
  {
    lua_pop(L,1);
    return 0;
  }
  lua_pop(L,1);
  return 1;
}

/*
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
      lua_pop(L, 1);  
    }
    printf("cardpeek> ");
  }
  fprintf(stderr,"bye.\n");
}
*/

/******************************************************************/

static lua_State* x_lua_begin(void)
{
  lua_State* L = lua_open();
  luaL_openlibs(L);
  log_printf(LOG_DEBUG,"Lua is loaded.");
  return L;
}

static int x_lua_end(lua_State* L)
{
  lua_close(L);
  log_printf(LOG_DEBUG,"Lua has stopped."); 
  return 1;
}

/*
void x_lua_shell(lua_State* L, const char* prompt)
{
  int error;
  char *inputline;

  printf("-- Lua shell, type '!' alone to quit.\n");
  while ((inputline=readline(prompt))!=NULL) 
  {
    if (strncmp(inputline,"!",1)==0)
      break;
    error = luaL_loadbuffer(L, inputline, strlen(inputline), prompt) ||
            lua_pcall(L, 0, 0, 0);
    if (error) {
      fprintf(stderr, "%s\n", lua_tostring(L, -1));
      lua_pop(L, 1); 
    }
    add_history(inputline);
    free(inputline);
  }
  fprintf(stderr,"-- bye.\n");
}
*/


/******************************************************************/
 
lua_State* LUA_STATE= NULL;

void luax_run_script_cb(const char* scriptname)
{
  chdir(config_get_string(CONFIG_FOLDER_SCRIPTS));
  log_printf(LOG_INFO,"Running script %s (please wait)",scriptname);
  run_file(LUA_STATE,scriptname);
}

void luax_run_command_cb(const char* command)
{
  int error;

  chdir(config_get_string(CONFIG_FOLDER_SCRIPTS));
  log_printf(LOG_DEBUG,"Executing '%s'",command);

  error = luaL_loadbuffer(LUA_STATE, command, strlen(command), "command line") ||
          lua_pcall(LUA_STATE, 0, 0, 0);
  if (error) {
    log_printf(LOG_ERROR,"%s", lua_tostring(LUA_STATE, -1));
    lua_pop(LUA_STATE, 1);  /* pop error message from the stack */
  }
}


int luax_init(void)
{
  LUA_STATE= x_lua_begin();

  luaopen_bytes(LUA_STATE);
  luaopen_asn1(LUA_STATE);
  luaopen_bit(LUA_STATE);
  luaopen_card(LUA_STATE);
  luaopen_log(LUA_STATE);
  luaopen_ui(LUA_STATE);
  luaopen_crypto(LUA_STATE);
  luaopen_nodes(LUA_STATE);
  chdir(config_get_string(CONFIG_FOLDER_CARDPEEK));
  log_printf(LOG_DEBUG,"Running configuration script %s",config_get_string(CONFIG_FILE_CONFIG));
  run_file(LUA_STATE,config_get_string(CONFIG_FILE_CONFIG));
  return 0;
}

void luax_release(void)
{
  x_lua_end(LUA_STATE);
}

const char *luax_get_string_value(const char *identifier)
{
  static char result_buffer[256];
  char *result = result_buffer;

  lua_getglobal(LUA_STATE,identifier);

  switch (lua_type(LUA_STATE,-1)) {
    case LUA_TNIL:
      result = NULL;
      break;
    case LUA_TNUMBER:
      sprintf(result,"%f",lua_tonumber(LUA_STATE,-1));
      break;
    case LUA_TBOOLEAN:
      if (lua_toboolean(LUA_STATE,-1))
	strcpy(result,"true");
      else
	strcpy(result,"false");
      break;
    case LUA_TSTRING:
      strncpy(result,lua_tostring(LUA_STATE,-1),255);
      result[255]=0;
      break;
    case LUA_TTABLE:
    case LUA_TFUNCTION:
    case LUA_TUSERDATA:
    case LUA_TTHREAD:
    case LUA_TLIGHTUSERDATA:
      sprintf(result,"[%s]",lua_typename(LUA_STATE,-1));
      break;
    default:
      result = NULL;
      break;
  }
  lua_pop(LUA_STATE,1);
  return result;
}
