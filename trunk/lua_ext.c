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
    UNUSED(L);
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

void luax_run_script(const char* scriptname)
{
    chdir(path_config_get_string(PATH_CONFIG_FOLDER_SCRIPTS));
    log_printf(LOG_INFO,"Running script %s (please wait)",scriptname);
    run_file(LUA_STATE,scriptname);
}

void luax_run_command(const char* command)
{
    int error;

    chdir(path_config_get_string(PATH_CONFIG_FOLDER_SCRIPTS));
    log_printf(LOG_DEBUG,"Executing '%s'",command);

    error = luaL_loadbuffer(LUA_STATE, command, strlen(command), "command line") ||
            lua_pcall(LUA_STATE, 0, 0, 0);
    if (error)
    {
        log_printf(LOG_ERROR,"%s", lua_tostring(LUA_STATE, -1));
        lua_pop(LUA_STATE, 1);  /* pop error message from the stack */
    }
}


int luax_init(void)
{
    GStatBuf st;
    const char *config_lua;
    const char *cardpeekrc_lua;

    LUA_STATE= x_lua_begin();

    luaopen_bytes(LUA_STATE);
    luaopen_asn1(LUA_STATE);
    luaopen_bit(LUA_STATE);
    luaopen_card(LUA_STATE);
    luaopen_log(LUA_STATE);
    luaopen_ui(LUA_STATE);
    luaopen_crypto(LUA_STATE);
    luaopen_nodes(LUA_STATE);

    lua_newtable(LUA_STATE);
    lua_setglobal(LUA_STATE,"cardpeek");

    config_lua = path_config_get_string(PATH_CONFIG_FILE_CONFIG);
    cardpeekrc_lua = path_config_get_string(PATH_CONFIG_FILE_RC);

    chdir(path_config_get_string(PATH_CONFIG_FOLDER_CARDPEEK));

    if (g_stat(config_lua,&st)==0)
    {
        log_printf(LOG_DEBUG,"Loading configuration script %s",config_lua);
        run_file(LUA_STATE,config_lua);
    }

    if (g_stat(cardpeekrc_lua,&st)==0)
    {
        log_printf(LOG_DEBUG,"Running user configuration script %s",cardpeekrc_lua);
        run_file(LUA_STATE,cardpeekrc_lua);
    }

    return 0;
}

void luax_release(void)
{
    x_lua_end(LUA_STATE);
}

char *luax_escape_string(const char *src)
{
    char *res;
    const char *s;
    char *p;
    unsigned alloc_count = 1;

    for (s=src; *s; s++)
    {
        if (*s=='\\' || *s=='\'' || *s=='"')
            alloc_count+=2;
        else
            alloc_count++;
    }
    p = res = g_malloc(alloc_count);
    while (*src)
    {
        switch (*src)
        {
        case '\\':
            *p++='\\';
            *p++='\\';
            break;
        case '\'':
            *p++='\\';
            *p++='\'';
            break;
        case '\"':
            *p++='\\';
            *p++='\"';
            break;
        default:
            *p++=*src;
        }
        src++;
    }
    *p='\0';
    return res;
}

/*************************************/

const char *config_table_header =
    "-- \n"
    "-- This file is automatically generated by Cardpeek.\n"
    "-- It holds cardpeek configuration parameters, which are stored in the\n"
    "-- lua 'cardpeek' table.\n"
    "-- \n"
    "-- See cardpeekrc.lua for adding your own functions and variables to\n"
    "-- cardpeek.\n"
    "-- \n";

void internal_save_table(FILE *cf, int depth)
{
    int i;

    g_assert(lua_istable(LUA_STATE,-1));
    
    lua_pushnil(LUA_STATE);
    while (lua_next(LUA_STATE,-2))
    {
        if (lua_type(LUA_STATE,-2)==LUA_TSTRING || lua_type(LUA_STATE,-2)==LUA_TNUMBER) 
	{

	    for (i=0;i<depth;i++) fprintf(cf,"  ");

	    if (lua_type(LUA_STATE,-2)==LUA_TSTRING)
	    {
                char *key = luax_escape_string(lua_tostring(LUA_STATE,-2));
		fprintf(cf,"['%s'] = ",key);
		g_free(key);
	    }
	    else
 	    {
	        fprintf(cf,"[%i] = ",(int)lua_tointeger(LUA_STATE,-2));
	    }

	    switch (lua_type(LUA_STATE,-1))
	    {
	    case LUA_TNUMBER:
                fprintf(cf,"%i,\n",(int)lua_tointeger(LUA_STATE,-1));
                break;
            case LUA_TBOOLEAN:
                if (lua_toboolean(LUA_STATE,-1))
                    fprintf(cf,"true,\n");
                else
                    fprintf(cf,"false,\n");
                break;
            case LUA_TSTRING:
		{
                    char *val = luax_escape_string(lua_tostring(LUA_STATE,-1));
                    fprintf(cf,"\"%s\",\n",val);
                    g_free(val);
                }
                break;
            case LUA_TTABLE:
		fprintf(cf,"{\n");
		internal_save_table(cf,depth+1);	
		for (i=0;i<depth;i++) fprintf(cf,"  ");
		fprintf(cf,"},\n");
		break;	
       /*   case LUA_TFUNCTION:
            case LUA_TUSERDATA:
            case LUA_TTHREAD:
            case LUA_TLIGHTUSERDATA:
            case LUA_TNIL: */
            default:
                fprintf(cf,"nil, -- %s is not a serializable format.\n",
			lua_typename(LUA_STATE,lua_type(LUA_STATE,-1)));
		break;
	    }
	} 
        lua_pop(LUA_STATE,1);
    }
}

gboolean luax_config_table_save(void)
{
    FILE *cf = fopen(path_config_get_string(PATH_CONFIG_FILE_CONFIG),"w");

    if (cf==NULL)
    {
        log_printf(LOG_ERROR,"Could not create file '%s' to save configurationd data.");
        return FALSE;
    }
    fprintf(cf,"%s",config_table_header);

    lua_getglobal(LUA_STATE,"cardpeek");
    if (!lua_istable(LUA_STATE,-1))
    {
        log_printf(LOG_WARNING,"Could not find 'config' table variable, no configuration data will be saved.");
        lua_pop(LUA_STATE,1);
	fprintf(cf,"-- empty --\n");
	fclose(cf);
        return FALSE;
    }

    fprintf(cf,"cardpeek = {\n");
    internal_save_table(cf,1); 
    fprintf(cf,"}\n\n");
    fprintf(cf,"dofile('scripts/lib/apdu.lua')\n");
    fprintf(cf,"-- end --\n");
    fclose(cf);
    lua_pop(LUA_STATE,1); /* remove global */
    return TRUE;
}

static void internal_get_variable(lua_State *L, const char *vname)
{
    int i;
    char **vparts = g_strsplit(vname,".",0);

    for (i=0;vparts[i]!=NULL;i++)
    {
        if (i==0)
        {
            lua_getglobal(L,vparts[0]);
        }
        else
        {
            if (!lua_istable(L,-1))
            {
                lua_pop(L,-1);
                lua_pushnil(L);
                break;
            }

            lua_getfield(L,-1,vparts[i]);
	    lua_remove(L,-2);
        }
    }
    g_strfreev(vparts);
}

static gboolean internal_set_variable(lua_State *L, const char *vname)
{
    int i;
    char **vparts = g_strsplit(vname,".",0);
    int vindex = lua_gettop(L);
    gboolean retval = FALSE;

    for (i=0;vparts[i]!=NULL;i++)
    {
        if (i==0)
        {
            if (vparts[1]==NULL)
            {
                lua_setglobal(L,vparts[0]);
                retval =  TRUE;
            }
            else
            {
                lua_getglobal(L,vparts[0]);
                if (lua_isnil(L,-1))
                {
                    lua_newtable(L);
                    lua_setglobal(L,vparts[0]);
                    lua_getglobal(L,vparts[0]);
                }
                if (!lua_istable(L,-1))
                {
                    break;
                }
            }
        }
        else
        {
            if (vparts[i+1]==NULL)
            {
                lua_pushvalue(L,vindex); 
                /* st: table, ..., table, value */
                lua_setfield(L,-2,vparts[i]);
                retval = TRUE;
            }
            else
            {
                lua_getfield(L,-1,vparts[i]);
                if (lua_isnil(L,-1))
                {
		    lua_pop(LUA_STATE,1);
                    lua_newtable(L);
                    lua_setfield(L,-2,vparts[i]);
                    lua_getfield(L,-1,vparts[i]);
                }
                if (!lua_istable(L,-1))
                {
                    break;
                }
            }
        }
    }

    lua_settop(L,vindex-1);
    g_strfreev(vparts);
    return retval;
}

char *luax_variable_get_strdup(const char *vname)
{
    char *retval;
    internal_get_variable(LUA_STATE,vname);

    retval = g_strdup(lua_tostring(LUA_STATE,-1)); /* x==NULL => g_strdup(x)==NULL */ 
    lua_pop(LUA_STATE,1);
    return retval;
}

gboolean luax_variable_set_strval(const char *vname, const char *value)
{
    lua_pushstring(LUA_STATE,value);
    
    return internal_set_variable(LUA_STATE,vname);
}

int luax_variable_get_integer(const char *vname)
{
    int retval;
    internal_get_variable(LUA_STATE,vname);

    retval = (int)lua_tointeger(LUA_STATE,-1);
    lua_pop(LUA_STATE,1);
    return retval;
}

gboolean luax_variable_set_integer(const char *vname, int value)
{
    lua_pushinteger(LUA_STATE,value);

    return internal_set_variable(LUA_STATE,vname); 
}

gboolean luax_variable_get_boolean(const char *vname)
{
    gboolean retval;
    internal_get_variable(LUA_STATE,vname);

    retval = lua_toboolean(LUA_STATE,-1);
    lua_pop(LUA_STATE,1);
    return retval;
}

gboolean luax_variable_set_boolean(const char *vname, gboolean value)
{
    lua_pushboolean(LUA_STATE,value);

    return internal_set_variable(LUA_STATE,vname); 
}

gboolean luax_variable_is_defined(const char *vname)
{
    gboolean retval;
    internal_get_variable(LUA_STATE,vname);
 
    retval = (lua_isnoneornil(LUA_STATE,-1)==0);
    lua_pop(LUA_STATE,-1);

    return retval;
}
