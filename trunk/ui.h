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

#ifndef UI_H
#define UI_H

#include "bytestring.h"

typedef struct {
    const char *(* ui_driver_name)(void);
    int (* ui_initialize)(int *argc, char ***argv);
    int (* ui_run)(const char *optional_command);
    void (* ui_exit)(void);
    void (* ui_update)();
    char* (* ui_select_reader)(unsigned list_size, const char** list);
    int (* ui_question_l)(const char *message, unsigned item_count, const char** items);
    int (* ui_readline)(const char *message, unsigned input_max, char* input);
    char** (* ui_select_file)(const char *title, const char *path, const char *filename);
    void (* ui_set_title)(const char *title);
    void (* ui_about)(void);
    void *(* ui_inprogress_new)(const char *title, const char *message);
    unsigned (* ui_inprogress_pulse)(void *pulser);
    unsigned (* ui_inprogress_set_fraction)(void *pulser, double level);
    void (* ui_inprogress_free)(void *pulser); 
    void (* ui_card_event_print)(unsigned event, 
                                 const bytestring_t *command,
                                 unsigned short sw,
                                 const bytestring_t *response,
                                 void *extra_data);
} ui_driver_t;

const char *ui_driver_name(void);

int ui_initialize(ui_driver_t *driver, int *argc, char ***argv);

int ui_run(const char *optinal_command);

void ui_exit(void);

void ui_update();

char* ui_select_reader(unsigned list_size, const char** list);

int ui_question_l(const char *message, unsigned item_count, const char** items);

int ui_question(const char *message, ...);

int ui_readline(const char *message, unsigned input_max, char* input);


char** ui_select_file(const char *title,
   		       const char *path,
   		       const char *filename);

void ui_set_title(const char *title);

void ui_about(void);

void *ui_inprogress_new(const char *title, const char *message);

unsigned ui_inprogress_pulse(void *pulser);

unsigned ui_inprogress_set_fraction(void *pulser, double level);

void ui_inprogress_free(void *pulser);

void ui_card_event_print(unsigned event, 
        const bytestring_t *command,
        unsigned short sw,
        const bytestring_t *response, 
        void *extra_data);

#endif
