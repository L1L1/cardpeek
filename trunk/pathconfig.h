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

#ifndef PATH_CONFIG_H
#define PATH_CONFIG_H

enum {
  PATH_CONFIG_FOLDER_WORKING,               /* Current Working directory */
  PATH_CONFIG_FILE_CARDPEEK_LOG,            /* PATH_CONFIG_FOLDER_CARDPEEK + /.cardpeek.log */
  PATH_CONFIG_FOLDER_HOME,                  /* $HOME on Linux or equivalent in Windows */
  PATH_CONFIG_FOLDER_CARDPEEK,              /* $CARDPEEK_DIR if defined, otherwise $HOME */
  PATH_CONFIG_FOLDER_SCRIPTS,               /* PATH_CONFIG_FOLDER_CARDPEEK + "/.cardpeek/scripts" */
  PATH_CONFIG_FOLDER_REPLAY,                /* PATH_CONFIG_FOLDER_CARDPEEK + "/.cardpeek/replay" */
  PATH_CONFIG_FOLDER_OLD_REPLAY,            /* legacy */
  PATH_CONFIG_FILE_CONFIG_LUA,              /* PATH_CONFIG_FOLDER_CARDPEEK + "/.cardpeek/config.lua" */
  PATH_CONFIG_FILE_CARDPEEK_RC,             /* PATH_CONFIG_FOLDER_CARDPEEK + "/.cardpeek/cardpeek_rc.lua" */
  PATH_CONFIG_FILE_VERSION,                 /* PATH_CONFIG_FOLDER_CARDPEEK + "/.cardpeek/version" */
  PATH_CONFIG_FILE_SMARTCARD_LIST_TXT,      /* legacy */
  PATH_CONFIG_FILE_SMARTCARD_LIST_DOWNLOAD, /* legacy */
  PATH_CONFIG_FILE_CARDPEEK_UPDATE,         /* PATH_CONFIG_FOLDER_CARDPEEK + "/.cardpeek/cardpeek.update" */
  NUM_PATH_CONFIG_OPTIONS
};


int path_config_init(void);
    /* Populate the mapping between PATH_* identfiers and strings */

const char *path_config_get_string(unsigned index);
    /* Translate the PATH_* numerical identifiers above into strings */

int path_config_set_string(unsigned index, const char *path);
    /* Changes any PATH_* numeral identifier to a specicifc path */

void path_config_release(void);
    /* Release allocated mapping created by path_config_init() */

#endif
