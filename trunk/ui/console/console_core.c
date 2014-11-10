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

#include <stdio.h>
#include <glib.h>
#include <stdlib.h>
#include "misc.h"
#include "lua_ext.h"
#include "console_core.h"
#include "console_inprogress.h"
#include "console_about.h"
#include "dyntree_model.h"
#include "config.h"
#include <string.h>
#include "pathconfig.h"

#ifdef HAVE_LIBREADLINE
#  if defined(HAVE_READLINE_READLINE_H)
#    include <readline/readline.h>
#  elif defined(HAVE_READLINE_H)
#    include <readline.h>
#  else /* !defined(HAVE_READLINE_H) */
  extern char *readline ();
#  endif /* !defined(HAVE_READLINE_H) */
  char *cmdline = NULL;
#else /* !defined(HAVE_READLINE_READLINE_H) */
#error "Missing readline library"
#endif /* HAVE_LIBREADLINE */

#ifdef HAVE_READLINE_HISTORY
#  if defined(HAVE_READLINE_HISTORY_H)
#    include <readline/history.h>
#  elif defined(HAVE_HISTORY_H)
#    include <history.h>
#  else /* !defined(HAVE_HISTORY_H) */
  extern void add_history ();
  extern int write_history ();
  extern int read_history ();
#  endif /* defined(HAVE_READLINE_HISTORY_H) */
    /* no history */
#endif /* HAVE_READLINE_HISTORY */

int CONSOLE_RUNNING = 0;

static const char *console_driver_name(void)
{
    return "console";
}

static int console_initialize(int *argc, char ***argv)
{
    CONSOLE_RUNNING = 1;
    dyntree_model_new();
    using_history();
    return 1;
}

static int console_run(const char *optional_command)
{
    if (optional_command)
    { 
        add_history(optional_command);
        luax_run_command(optional_command);
    }

    while (CONSOLE_RUNNING)
    {
        char *line = readline("cardpeek> ");
    
        if (line)
        {
            if (*line)
            {
                luax_run_command(line);
                add_history(line);
            }
            free(line);
        }
    }

    CONSOLE_RUNNING = 0;
    return 1;
}
    
static void console_exit(void)
{
    clear_history();
    CONSOLE_RUNNING = 0;
}
    
static void console_update()
{
    /* void */
}
    
static int console_question_l(const char *message, unsigned item_count, const char** items)
{
    int i;
    char line[8];
    int selection;

    printf("%s:\n",message);
    for (i=0;i<item_count;i++)
    {
        printf("  %i) %s\n",i+1,items[i]); 
    }
    for (;;) {
        printf("Your selection (1-%i): ",item_count);
        fgets(line,8,stdin);
        if (*line>='0' && *line<='9')
        {
            selection = atoi(line);
            if (selection<=0 || selection>item_count)
                printf("Invalid selection, please try again\n");
            else
                break;
        }    
    }
    return selection;
}

static char* console_select_reader(unsigned list_size, const char** list)
{
    int i;
    char line[8];
    int selection;

    printf("Select a card reader:\n");
    printf("  0) No reader\n");
    for (i=0;i<list_size;i++)
    {
        printf("  %i) %s\n",i+1,list[i]); 
    }
    for (;;) {
        printf("Your selection (0-%i): ",list_size);
        fgets(line,8,stdin);
        if (*line>='0' && *line<='9')
        {
            selection = atoi(line);
            if (selection<0 || selection>list_size)
                printf("Invalid selection, please try again\n");
            else
                break;
        }  
    }
    if (selection==0)
        return g_strdup("none");
    return g_strdup(list[selection-1]);
}

char *last_line = NULL;
static void cb_linehandler(char *line)
{
    last_line = line;
}

static int console_readline(const char *message, unsigned input_max, char* input)
{
    char *prompt = g_malloc(strlen(message)+3);
    
    sprintf(prompt,"%s: ",message);

    rl_callback_handler_install (prompt, cb_linehandler);
    
    rl_insert_text(input);
    
    rl_redisplay();

    while (last_line==NULL)
    {
        rl_callback_read_char();
    }
    g_strlcpy(input,last_line,input_max+1);
    free(last_line);
    last_line = NULL;
    rl_callback_handler_remove();
    g_free(prompt);
    return 1;
}

static char** console_select_file(const char *title, const char *path, const char *filename)
{
    static char *retval[2];
    retval[0]=NULL;
    retval[1]=NULL;

    printf("%s\n",title);
    
    if (filename==NULL)
        rl_callback_handler_install ("Open: ", cb_linehandler);
    else   
        rl_callback_handler_install ("Save as: ", cb_linehandler);

    if (path)
        rl_insert_text(path);
    else
        rl_insert_text(path_config_get_string(PATH_CONFIG_FOLDER_HOME));

    if (filename!=NULL)
    {
        rl_insert_text("/");
        rl_insert_text(filename);
    }

    rl_redisplay();

    while (last_line==NULL)
    {
        rl_callback_read_char();
    }

    if (*last_line==0)
    {
        printf("No selection.\n");
    }
    else
    {
        retval[0]=g_path_get_basename(last_line);    
        retval[1]=g_path_get_dirname(last_line);    
    }
    free(last_line);
    last_line = NULL;
    rl_callback_handler_remove();

    return retval;
}
    
static void console_set_title(const char *title)
{
    /* void */
}
    
static void console_card_event_print(unsigned event, 
                              const bytestring_t *command,
                              unsigned short sw,
                              const bytestring_t *response,
                              void *extra_data)
{
    /* void */
}

ui_driver_t UI_DRIVER_CONSOLE = {
    console_driver_name,
    console_initialize,
    console_run,
    console_exit,
    console_update,
    console_select_reader,
    console_question_l,
    console_readline,
    console_select_file,
    console_set_title,
    console_about,
    console_inprogress_new,
    console_inprogress_pulse,
    console_inprogress_set_fraction,
    console_inprogress_free,
    console_card_event_print
};

ui_driver_t *ui_driver_for_console(void)
{
    return &UI_DRIVER_CONSOLE;
}
