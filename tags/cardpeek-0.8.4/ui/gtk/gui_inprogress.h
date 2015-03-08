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

#ifndef GUI_INPROGRESS_H
#define GUI_INPROGRESS_H

#include <gtk/gtk.h>

void* gui_inprogress_new(const char *title, const char *message);

unsigned gui_inprogress_pulse(void *pulser);

unsigned gui_inprogress_set_fraction(void *pulser, gdouble level);

void gui_inprogress_free(void *pulser);

#endif
