/********************************************************************** 
*
* This file is part of Cardpeek, the smartcard reader utility.
*
* Copyright 2009-2013 by 'L1L1'
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
#include <glib/gstdio.h>
#include "smartcard.h"
#include "misc.h"
#include "a_string.h"
#include "gui.h"
#include "gui_readerview.h"
#include "pathconfig.h"
#include "lua_ext.h"
#include "script_version.h"
#include "system_info.h"
#include <errno.h>
#ifndef _WIN32
#include <sys/wait.h>
#endif
#include <signal.h> 
#include <getopt.h>
#include "cardpeek_resources.gresource"

/*extern unsigned char _binary_dot_cardpeek_tar_gz_start;
  extern int _binary_dot_cardpeek_tar_gz_size;*/
/* extern unsigned char _binary_dot_cardpeek_tar_gz_end; */

static int install_dot_file(void)
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
  GResource* cardpeek_resources;
  GBytes* dot_cardpeek_tar_gz;
  unsigned char *dot_cardpeek_tar_gz_start;
  gsize dot_cardpeek_tar_gz_size;

  if (stat(dot_dir,&sbuf)==0)
  {
    log_printf(LOG_DEBUG,"Found directory '%s'",dot_dir);

    if ((f = g_fopen(version_file,"r"))!=NULL) 
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
	if ((f=g_fopen(version_file,"w"))!=NULL)
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
    a_sprintf(astr,"It seems this is the first time you run Cardpeek, because \n'%s' does not exit (%s).\n"
  	      "Do you want to create '%s' ?",dot_dir,strerror(errno),dot_dir);

    if (gui_question(a_strval(astr),"Yes","No",NULL)!=0)
    {
      log_printf(LOG_DEBUG,"'%s' will not be created",dot_dir);
      a_strfree(astr);

      return 0;
    }
    a_strfree(astr);
  }

  cardpeek_resources = cardpeek_resources_get_resource();
  if (cardpeek_resources == NULL)
  {
	log_printf(LOG_ERROR,"Could not load cardpeek internal resources. This is not good.");
	return -1; 
  }
  dot_cardpeek_tar_gz = g_resources_lookup_data("/cardpeek/dot_cardpeek.tar.gz",G_RESOURCE_LOOKUP_FLAGS_NONE,NULL);
  if (dot_cardpeek_tar_gz == NULL)
  {
	log_printf(LOG_ERROR,"Could not load .cardpeek.tar.gz");
	return -1;
  }
  dot_cardpeek_tar_gz_start = (unsigned char *)g_bytes_get_data(dot_cardpeek_tar_gz,&dot_cardpeek_tar_gz_size);  
  
  chdir(home_dir);
  if ((f = g_fopen("dot_cardpeek.tar.gz","wb"))==NULL)
  {
	  log_printf(LOG_ERROR,"Could not create dot_cardpeek.tar.gz in %s (%s)", home_dir, strerror(errno));
	  gui_question("Could not create dot_cardpeek.tar.gz, aborting.","Ok",NULL);
	  return 0;
  }
  
  if (fwrite(dot_cardpeek_tar_gz_start,dot_cardpeek_tar_gz_size,1,f)!=1)
  {
	  log_printf(LOG_ERROR,"Could not write to dot_cardpeek.tar.gz in %s (%s)", home_dir, strerror(errno));
	  gui_question("Could not write to dot_cardpeek.tar.gz, aborting.","Ok",NULL);
	  fclose(f);
	  return 0;
  }
  log_printf(LOG_DEBUG,"Wrote %i bytes to dot_cardpeek.tar.gz",dot_cardpeek_tar_gz_size);
  fclose(f);

  g_bytes_unref(dot_cardpeek_tar_gz);

  log_printf(LOG_INFO,"Created dot_cardpeek.tar.gz");
  log_printf(LOG_INFO,"Creating files in %s", home_dir);
  status = system("tar xzvf dot_cardpeek.tar.gz");
  log_printf(LOG_INFO,"'tar xzvf dot_cardpeek.tar.gz' returned %i",status);
  if (status!=0)
  {
	gui_question("Extraction of dot_cardpeek.tar.gz failed, aborting.","Ok",NULL);
	return 0;
  }
  status = system("rm dot_cardpeek.tar.gz");
  log_printf(LOG_INFO,"'rm dot_cardpeek.tar.gz' returned %i",status);

  gui_question("Note: The files have been created.\nIt is recommended that you quit and restart cardpeek, for changes to take effect.","Ok",NULL);
  return 1;
}


static const char *message = 
"***************************************************************\n"
" Oups...                                                       \n"
"  Cardpeek has encoutered a problem and has exited abnormally. \n"
"  Additionnal information may be available in the file         \n"
"                                                               \n"
"  "
;

static const char *signature =
"\n"
"                                                               \n"
"  L1L1@gmx.com                                                 \n"
"***************************************************************\n"
;

static void save_what_can_be_saved(int sig_num) 
{
  const char *logfile;	
  char buf[32];

  write(2,message,strlen(message));
  logfile = config_get_string(CONFIG_FILE_LOG);
  write(2,logfile,strlen(logfile));
  write(2,signature,strlen(signature));
  sprintf(buf,"Recieved signal %i\n",sig_num); 
  write(2,buf,strlen(buf));
  log_close_file();
  exit(-2);
} 

static struct option long_options[] = {
            {"reader",  required_argument, 0,  'r' },
            {"exec",    required_argument, 0,  'e' },
            {0,         0,                 0,  0 }
        };

int main(int argc, char **argv)
{
  cardmanager_t* CTX;
  cardreader_t* READER;
  int opt;
  int opt_index = 0;
  int options_ok = 1;
  char* reader_name = NULL;
  char* exec_command = NULL;

  signal(SIGSEGV, save_what_can_be_saved); 
  
  config_init();
    
  log_open_file();

  gui_init(&argc,&argv);

  while ((opt = getopt_long(argc,argv,"r:e:",long_options,&opt_index))!=-1) 
  {
	  switch (opt) {
		case 'r':
			reader_name = g_strdup(optarg);
                        break;
		case 'e':
			exec_command = optarg;
			break;
		default:
			log_printf(LOG_ERROR, "Usage: %s [-r|--reader reader-name] [-e|--exec lua_command]\n",
				argv[0]);
			options_ok = 0;
	  }
  }
   
  if (options_ok)
  {
	  gui_create();

	  log_printf(LOG_INFO,"Running %s",system_string_info());

	  install_dot_file(); 

	  luax_init();

	  CTX = cardmanager_new();

	  if (reader_name == NULL)
	  {
		  reader_name = gui_select_reader(cardmanager_count_readers(CTX),
				                  cardmanager_reader_name_list(CTX));
	  }

	  READER = cardreader_new(reader_name);
	  cardmanager_free(CTX);
	  if (READER)
	  {
		  luax_set_card_reader(READER);

		  cardreader_set_callback(READER,gui_readerview_print,NULL);

		  if (exec_command) luax_run_command(exec_command);

		  gui_run();

		  cardreader_free(READER);
	  }
	  else
	  {
		  fprintf(stderr,"Failed to open smart card reader '%s'.\n",reader_name);
		  log_printf(LOG_ERROR,"Failed to open smart card reader '%s'.", reader_name);
	  }
	  luax_release();
  }

  if (reader_name) g_free(reader_name);
  
  log_close_file();

  config_release();  

  return 0;
}


