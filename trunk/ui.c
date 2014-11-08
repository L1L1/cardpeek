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

#include <string.h>
#include <stdarg.h>
#include <glib.h>
#include "ui.h"

ui_driver_t *UI_DRIVER = NULL;

const char *ui_driver_name(void)
{
    if (UI_DRIVER==NULL)
        return NULL;
    return UI_DRIVER->ui_driver_name();
}

int ui_initialize(ui_driver_t *driver, int *argc, char ***argv)
{
    UI_DRIVER = driver;
    return UI_DRIVER->ui_initialize(argc, argv);
}

int ui_question(const char *message, ...)
{
    va_list al;
    const char *item;
    unsigned item_count=0;
    const char **item_table;
    int retval;

    va_start(al,message);
    while (va_arg(al,const char *)) item_count++;
    va_end(al);

    item_table=g_malloc(sizeof(const char *)*item_count);
    item_count=0;

    va_start(al,message);
    while ((item=va_arg(al,const char *)))
        item_table[item_count++]=item;
    va_end(al);

    retval = ui_question_l(message,item_count,item_table);
    g_free(item_table);
    return retval;
}

int ui_question_l(const char *message, unsigned item_count, const char **items)
{
    return UI_DRIVER->ui_question_l(message,item_count,items);
}

int ui_readline(const char *message, unsigned input_max, char *input)
/* input_max does not include final '\0' */
{
    return UI_DRIVER->ui_readline(message,input_max,input);    
}

char **ui_select_file(const char *title,
                       const char *path,
                       const char *filename)
{
    return UI_DRIVER->ui_select_file(title,path,filename);
}

void ui_update(void)
{
    UI_DRIVER->ui_update();
}

void ui_exit(void)
{
    UI_DRIVER->ui_exit();
}

char *ui_select_reader(unsigned list_size, const char **list)
{
    return UI_DRIVER->ui_select_reader(list_size,list);
}

int ui_run(const char *optional_command)
{
    return UI_DRIVER->ui_run(optional_command);
}

void ui_set_title(const char *title)
{
    char atitle[80];

    g_snprintf(atitle,80,"cardpeek: %s",title);
    atitle[79]=0;
    UI_DRIVER->ui_set_title(atitle);
}

void ui_about(void)
{
    UI_DRIVER->ui_about();
}

void *ui_inprogress_new(const char *title, const char *message)
{
    return UI_DRIVER->ui_inprogress_new(title,message);
}

unsigned ui_inprogress_pulse(void *pulser)
{
    return UI_DRIVER->ui_inprogress_pulse(pulser);
}

unsigned ui_inprogress_set_fraction(void *pulser, double level)
{
    return UI_DRIVER->ui_inprogress_set_fraction(pulser,level);
}

void ui_inprogress_free(void *pulser)
{
    UI_DRIVER->ui_inprogress_free(pulser);
}

void ui_card_event_print(unsigned event,
        const bytestring_t *command,     
        unsigned short sw,                                                
        const bytestring_t *response,                                                             
        void *extra_data)
{
    UI_DRIVER->ui_card_event_print(event,command,sw,response,extra_data);
}
/************/


