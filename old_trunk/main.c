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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "smartcard.h"
#include "misc.h"
#include "gui.h"
#include "config.h"
#include "lua_ext.h"

extern unsigned char _binary_dot_cardpeek_tar_gz_start;
extern unsigned char _binary_dot_cardpeek_tar_gz_end;

int install_dot_file()
{
  const char* dot_dir = config_get_string(CONFIG_PATH_CARDMAN);
  const char* home_dir = config_get_string(CONFIG_PATH_HOME);
  struct stat sbuf;
  FILE* f;
  int status;
  a_string_t* astr;

  if (stat(dot_dir,&sbuf)==0)
  {
    log_printf(LOG_DEBUG,"Found directory '%s'",dot_dir);
    return 1;
  }
  
  astr = a_strnew(NULL);
  a_sprintf(astr,"It seems this is the first time you run Cardpeek, because \n'%s' does not exit.\n"
	    "Do you want to create '%s' ?",dot_dir,dot_dir);

  if (gui_message_box(a_strval(astr),"Yes","No",NULL)!=1)
  {
    a_strfree(astr);
    return 0;
  }
  a_strfree(astr);
    
  chdir(home_dir);
  f = fopen("dot_cardpeek.tar.gz","w");
  if (fwrite(&_binary_dot_cardpeek_tar_gz_start,
	     &_binary_dot_cardpeek_tar_gz_end-&_binary_dot_cardpeek_tar_gz_start,
	     1,f)!=1)
    return 0;
  fclose(f);
  log_printf(LOG_INFO,"Created dot_cardpeek.tar.gz");
  status = system("tar xzvf dot_cardpeek.tar.gz");
  log_printf(LOG_INFO,"'tar xzvf dot_cardpeek.tar.gz' returned %i",status);
  status = system("rm dot_cardpeek.tar.gz");
  log_printf(LOG_INFO,"'rm dot_cardpeek.tar.gz' returned %i",status);
  return 1;
}


int main(int argc, char **argv)
{
  cardmanager_t* CTX;
  cardreader_t* READER;
  /*int i;*/
  char* reader_name;

  config_init();
  
  gui_init(&argc,&argv);
  
  gui_create(luax_run_script_cb,luax_run_command_cb);
  
  install_dot_file();
  
  CTX = cardmanager_new();

  reader_name = gui_select_reader(cardmanager_count_readers(CTX),cardmanager_reader_name_list(CTX));

  READER = cardreader_new(reader_name);
  if (reader_name) g_free(reader_name);
  cardmanager_free(CTX);

  luax_init();

  luax_set_card_reader(READER);

  gui_run();

  config_release();

  luax_release();

  cardreader_free(READER);
  return 0;
}

