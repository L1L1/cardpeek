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

#ifndef PATH_CONFIG_H
#define PATH_CONFIG_H

enum {
  CONFIG_FOLDER_CARDTREES,
  CONFIG_FOLDER_SCRIPTS_OTHER,
  CONFIG_FOLDER_HOME,
  CONFIG_FOLDER_CARDPEEK,
  CONFIG_FOLDER_SCRIPTS,
  CONFIG_FOLDER_LOGS,
  CONFIG_FILE_CONFIG,
  CONFIG_FILE_LOG,
  CONFIG_FILE_SCRIPT_VERSION,
  NUM_CONFIG_OPTIONS
};


int config_init();
const char *config_get_string(unsigned index);
int config_set_string(unsigned index, const char *path);
void config_release();

#endif
