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
#include "http_download.h"

static int update_smartcard_list_txt(void)
{
    const char* smartcard_list_txt = path_config_get_string(PATH_CONFIG_FILE_SMARTCARD_LIST_TXT);
    char *url;
    time_t now = time(NULL);
    unsigned next_update = (unsigned)luax_variable_get_integer("cardpeek.smartcard_list.next_update");
    int retval = 0;
    time_t next_update_distance;

    if (!luax_variable_is_defined("cardpeek.smartcard_list"))
    {
        luax_variable_set_boolean("cardpeek.smartcard_list.auto_update",FALSE);
        luax_variable_set_integer("cardpeek.smartcard_list.next_update",0);
        luax_variable_set_strval("cardpeek.smartcard_list.url",
                                 "http://ludovic.rousseau.free.fr/softwares/pcsc-tools/smartcard_list.txt");
    }

    if (luax_variable_get_boolean("cardpeek.smartcard_list.auto_update")!=TRUE)
    {
        log_printf(LOG_INFO,"smartcard_list.txt auto-update is disabled.");
        return 0;
    }

    if (now<next_update) 
    {
        next_update_distance = (next_update-now)/(3600*24);
        if (next_update_distance<1)
            log_printf(LOG_DEBUG,"smartcard_list.txt will be scheduled for update in less than a day.");
        else
            log_printf(LOG_DEBUG,"smartcard_list.txt will be scheduled for update in about %u day(s).",next_update_distance);
        return 0;
    }

    switch (gui_question("The local copy of the ATR database may be outdated.\nDo you whish to do an online update?",
                         "Yes","No, ask me again later","No, always use the local copy",NULL))
    {
        case 0:
            break;
        case 1:
            luax_variable_set_integer("cardpeek.smartcard_list.next_update",(int)(now+(24*3600)));
            return 0;
        case 2:
            luax_variable_set_boolean("cardpeek.smartcard_list.auto_update",FALSE);
            return 0;
        default:
            return 0;
    }

    log_printf(LOG_INFO,"Attempting to update smartcard_list.txt");

    url=luax_variable_get_strdup("cardpeek.smartcard_list.url");

    if (url==NULL)
        url = g_strdup("http://ludovic.rousseau.free.fr/softwares/pcsc-tools/smartcard_list.txt");

    if ((retval=http_download(url,smartcard_list_txt))!=0)
    {
        /* update again in a month */
        luax_variable_set_integer("cardpeek.smartcard_list.next_update",(int)(now+30*(24*3600))); 
    } 

    luax_config_table_save();
    g_free(url);
    return retval;
}

static int install_dot_file(void)
{
    const char* cardpeek_dir = path_config_get_string(PATH_CONFIG_FOLDER_CARDPEEK);
    const char* old_replay_dir = path_config_get_string(PATH_CONFIG_FOLDER_OLD_REPLAY);
    const char* new_replay_dir = path_config_get_string(PATH_CONFIG_FOLDER_REPLAY);
    const char* version_file = path_config_get_string(PATH_CONFIG_FILE_VERSION);
    GStatBuf sbuf;
    FILE* f;
    int status;
    a_string_t* astr;
    unsigned dot_version=0;
    int response;
    GResource* cardpeek_resources;
    GBytes* dot_cardpeek_tar_gz;
    unsigned char *dot_cardpeek_tar_gz_start;
    gsize dot_cardpeek_tar_gz_size;

    if (g_stat(cardpeek_dir,&sbuf)==0)
    {
        log_printf(LOG_DEBUG,"Found directory '%s'",cardpeek_dir);

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

        if (dot_version==0 && f==NULL)
            a_sprintf(astr,"This seems to be the first time you run Cardpeek, because '%s' does not exist\n"
                      "Do you want to install the necessary files in '%s'?",version_file,cardpeek_dir);
        else
            a_sprintf(astr,"Some scripts in '%s' seem to be outdated or missing\n"
                      "Do you want to upgrade these scripts?",cardpeek_dir);

        if ((response = gui_question(a_strval(astr),"Yes","No","No, don't ask me again",NULL))!=0)
        {
            log_printf(LOG_DEBUG,"The files in '%s' will not be upgraded.",cardpeek_dir);
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
                  "Do you want to create '%s'?",cardpeek_dir,strerror(errno),cardpeek_dir);

        if (gui_question(a_strval(astr),"Yes","No",NULL)!=0)
        {
            log_printf(LOG_DEBUG,"'%s' will not be created",cardpeek_dir);
            a_strfree(astr);

            return 0;
        }

        a_strfree(astr);

#ifndef _WIN32
        if (mkdir(cardpeek_dir,0770)!=0)
#else
        if (mkdir(cardpeek_dir)!=0)
#endif
	{
            astr = a_strnew(NULL);
            a_sprintf(astr,"'%s' could not be created: %s",cardpeek_dir,strerror(errno));
            log_printf(LOG_ERROR,a_strval(astr));
            gui_question(a_strval(astr),"OK",NULL);
            a_strfree(astr);
            return 0;
        }
    }

    if (g_stat(old_replay_dir,&sbuf)==0)
    {
        if (rename(old_replay_dir,new_replay_dir)==0)
        {
            log_printf(LOG_INFO,"Renamed %s to %s.",
                       old_replay_dir, new_replay_dir);
        }
        else
        {
            log_printf(LOG_WARNING,"Failed to rename %s to %s: %s",
                       old_replay_dir, new_replay_dir, strerror(errno));
        }
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

    chdir(cardpeek_dir);
    if ((f = g_fopen("dot_cardpeek.tar.gz","wb"))==NULL)
    {
        log_printf(LOG_ERROR,"Could not create dot_cardpeek.tar.gz in %s (%s)", cardpeek_dir, strerror(errno));
        gui_question("Could not create dot_cardpeek.tar.gz, aborting.","Ok",NULL);
        return 0;
    }

    if (fwrite(dot_cardpeek_tar_gz_start,dot_cardpeek_tar_gz_size,1,f)!=1)
    {
        log_printf(LOG_ERROR,"Could not write to dot_cardpeek.tar.gz in %s (%s)", cardpeek_dir, strerror(errno));
        gui_question("Could not write to dot_cardpeek.tar.gz, aborting.","Ok",NULL);
        fclose(f);
        return 0;
    }
    log_printf(LOG_DEBUG,"Wrote %i bytes to dot_cardpeek.tar.gz",dot_cardpeek_tar_gz_size);
    fclose(f);

    g_bytes_unref(dot_cardpeek_tar_gz);

    log_printf(LOG_INFO,"Created dot_cardpeek.tar.gz");
    log_printf(LOG_INFO,"Creating files in %s", cardpeek_dir);
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

static gboolean run_command_from_cli(gpointer data)
{
    luax_run_command((const char *)data);
    return FALSE;
}

/*
static gboolean run_update_checks(gpointer data)
{
    update_smartcard_list_txt();
	return FALSE;
}
*/

#ifndef _WIN32
#include <execinfo.h>
static void do_backtrace()
{
    char **btrace;
    void *buffer[32];
    int i,depth;
    
    depth=backtrace(buffer,32);

    btrace = backtrace_symbols(buffer,depth);

    for (i=0;i<depth;i++)
    {
        log_printf(LOG_DEBUG,"backtrace: %s",btrace[i]);    
    }

    free(btrace);
}
#endif

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
    logfile = path_config_get_string(PATH_CONFIG_FILE_CARDPEEK_LOG);
    write(2,logfile,strlen(logfile));
    write(2,signature,strlen(signature));
    sprintf(buf,"Received signal %i\n",sig_num);
    write(2,buf,strlen(buf));
   
    log_printf(LOG_ERROR,"Received signal %i",sig_num); 
#ifndef _WIN32
    do_backtrace();
#endif
    log_close_file();
    exit(-2);
}

static void display_readers_and_version(void)
{
    cardmanager_t *CTX;
    unsigned i;
    unsigned reader_count;
    const char **reader_list;

    log_set_function(NULL);

    luax_init();

    fprintf(stdout,"%sThis is %s.%s\n",ANSI_GREEN,system_string_info(),ANSI_RESET);
    fprintf(stdout,"Cardpeek path is %s\n",path_config_get_string(PATH_CONFIG_FOLDER_CARDPEEK));

    CTX = cardmanager_new();
    reader_count = cardmanager_count_readers(CTX);
    reader_list  = cardmanager_reader_name_list(CTX);
    if (reader_count == 0)
        fprintf(stdout,"There are no readers detected\n");
    else if (reader_count==1)
        fprintf(stdout,"There is 1 reader detected:\n");
    else
        fprintf(stdout,"There are %i readers detected:\n",reader_count);
    for (i=0; i<reader_count; i++)
        fprintf(stdout," -> %s\n", reader_list[i]);
    fprintf(stdout,"\n");
    cardmanager_free(CTX);
    luax_release();
}

static void display_help(char *progname)
{
    fprintf(stderr, "Usage: %s [-r|--reader reader-name] [-e|--exec lua-command] [-v|--version]\n",
            progname);
}

static struct option long_options[] =
{
    {"reader",  	required_argument, 0,  'r' },
    {"exec",    	required_argument, 0,  'e' },
    {"version", 	no_argument, 	   0,  'v' },
    {"help", 		no_argument, 	   0,  'h' },
    {0,        	 	0,                 0,   0 }
};

int main(int argc, char **argv)
{
    cardmanager_t* CTX;
    cardreader_t* READER;
    int opt;
    int opt_index = 0;
    int run_gui = 1;
    char* reader_name = NULL;
    char* exec_command = NULL;

    signal(SIGSEGV, save_what_can_be_saved);

    path_config_init();

    log_open_file();

    while ((opt = getopt_long(argc,argv,"r:e:vh",long_options,&opt_index))!=-1)
    {
        switch (opt)
        {
        case 'r':
            reader_name = g_strdup(optarg);
            break;
        case 'e':
            exec_command = optarg;
            break;
        case 'v':
            display_readers_and_version();
            run_gui = 0;
            break;
        default:
            display_help(argv[0]);
            run_gui = 0;
        }
    }

    if (run_gui)
    {
        /* if we want threads:
           gdk_threads_init();
           gdk_threads_enter();
         */

        gui_init(&argc,&argv);

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

            if (exec_command)
                g_idle_add(run_command_from_cli,exec_command);
            else
                update_smartcard_list_txt();
            /*else
              g_idle_add(run_update_checks,NULL);
             */
            gui_run();

            cardreader_free(READER);
        }
        else
        {
            fprintf(stderr,"Failed to open smart card reader '%s'.\n",reader_name);
            log_printf(LOG_ERROR,"Failed to open smart card reader '%s'.", reader_name);
        }

        luax_config_table_save();

        luax_release();

        /* if we want threads:
           gdk_threads_leave();
         */
    }

    if (reader_name) g_free(reader_name);

    log_close_file();

    path_config_release();

    return 0;
}


