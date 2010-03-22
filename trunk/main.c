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
#include <string.h>
#include "smartcard.h"
#include "misc.h"
#include "gui.h"
#include "pathconfig.h"
#include "lua_ext.h"
#include "script_version.h"
#include <sys/wait.h>
#include <signal.h> 


extern unsigned char _binary_dot_cardpeek_tar_gz_start;
extern unsigned char _binary_dot_cardpeek_tar_gz_end;

int install_dot_file()
{
  const char* dot_dir = config_get_string(CONFIG_FOLDER_CARDPEEK);
  const char* home_dir = config_get_string(CONFIG_FOLDER_HOME);
  const char* version_file = config_get_string(CONFIG_FILE_SCRIPT_VERSION);
  struct stat sbuf;
  FILE* f;
  int status;
  a_string_t* astr;
  unsigned dot_version=0;
  int response;

  if (stat(dot_dir,&sbuf)==0)
  {
    log_printf(LOG_DEBUG,"Found directory '%s'",dot_dir);

    if ((f = fopen(version_file,"r"))!=NULL) 
    {
      fscanf(f,"%u",&dot_version);
      fclose(f);
      if (dot_version>=SCRIPT_VERSION) 
      {
	log_printf(LOG_DEBUG,"Scripts are up to date.");
	return 1;
      }
    }
    astr = a_strnew(NULL);
    a_sprintf(astr,"Some scripts in '%s' seem to come from an older version of Cardpeek\n"
	      "Do you want to upgrade these scripts ?",dot_dir);

    if ((response = gui_question(a_strval(astr),"Yes","No","No, don't ask me again",NULL))!=0)
    {
      log_printf(LOG_DEBUG,"The scripts in '%s' will not be upgraded.",dot_dir);
      a_strfree(astr);

      if (response==2)
      {
	if ((f=fopen(version_file,"w"))!=NULL)
	{
	  fprintf(f,"%u\n",SCRIPT_VERSION);
	  fclose(f);
	}
      }
      return 0;
    }
    a_strfree(astr);
  }
  else
  {
    astr = a_strnew(NULL);
    a_sprintf(astr,"It seems this is the first time you run Cardpeek, because \n'%s' does not exit.\n"
  	      "Do you want to create '%s' ?",dot_dir,dot_dir);

    if (gui_question(a_strval(astr),"Yes","No",NULL)!=0)
    {
      log_printf(LOG_DEBUG,"'%s' will not be created",dot_dir);
      a_strfree(astr);

      return 0;
    }
    a_strfree(astr);
  }
    
  chdir(home_dir);
  f = fopen("dot_cardpeek.tar.gz","w");
  if (fwrite(&_binary_dot_cardpeek_tar_gz_start,
	     &_binary_dot_cardpeek_tar_gz_end-&_binary_dot_cardpeek_tar_gz_start,
	     1,f)!=1)
    return 0;
  fclose(f);
  log_printf(LOG_INFO,"Created dot_cardpeek.tar.gz");
  printf("Creating files in %s :\n", home_dir);
  status = system("tar xzvf dot_cardpeek.tar.gz");
  if (status!=0)
    printf("Failed\n");
  else
    printf("Done\n");
  log_printf(LOG_INFO,"'tar xzvf dot_cardpeek.tar.gz' returned %i",status);
  status = system("rm dot_cardpeek.tar.gz");
  log_printf(LOG_INFO,"'rm dot_cardpeek.tar.gz' returned %i",status);

  gui_question("Note: The files have been created.\nIt is recommended that you quit and restart cardpeek, for changes to take effect.","Ok",NULL);
  return 1;
}


static char *message = 
"+----------------------------------------------------------------+\n"
"|  Oups...                                                       |\n"
"|  Cardpeek has encoutered a problem and has exited abnormally.  |\n"
"|  Additionnal information may be available in the file          |\n"
"|    $HOME/.cardpeek.log                                         |\n"
"|                                                                |\n"
"|  L1L1@gmx.com                                                  |\n"
"+----------------------------------------------------------------+\n"
;

void save_what_can_be_saved(int sig_num) 
{ 
  write(2,message,strlen(message));
  log_close_file();
  exit(-2);
} 


int main(int argc, char **argv)
{
  cardmanager_t* CTX;
  cardreader_t* READER;
  /*int i;*/
  char* reader_name;

  signal(SIGSEGV, save_what_can_be_saved); 

  config_init();
    
  log_open_file();
 
  gui_init(&argc,&argv);
  
  gui_create(luax_run_script_cb,luax_run_command_cb);
  
  install_dot_file();

  luax_init();

  CTX = cardmanager_new();

  reader_name = gui_select_reader(cardmanager_count_readers(CTX),cardmanager_reader_name_list(CTX));

  READER = cardreader_new(reader_name);
  if (reader_name) g_free(reader_name);
  cardmanager_free(CTX);

  luax_set_card_reader(READER);

  cardreader_set_callback(READER,gui_reader_print_data,NULL);

  gui_run();

  luax_release();

  log_close_file();

  config_release();  

  cardreader_free(READER);

  return 0;
}


