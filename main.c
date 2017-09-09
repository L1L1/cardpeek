/**********************************************************************
*
* This file is part of Cardpeek, the smart card reader utility.
*
* Copyright 2009-2017 by Alain Pannetrat <L1L1@gmx.com>
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
#include "ui.h"
#include "ui/gtk/gui_core.h"
#include "ui/console/console_core.h"
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
#include "cardpeek_update.h"
#include <openssl/err.h>
#include <openssl/ssl.h>
#include "main.h"

static int update_cardpeek(void)
{
    if (cardpeek_update_check())
        return cardpeek_update_perform();
    return 0;
}

static void check_cardpeek_dir_exists(void)
{
    const char* cardpeek_dir = path_config_get_string(PATH_CONFIG_FOLDER_CARDPEEK);
    a_string_t* astr;
    GStatBuf sbuf;

    if (g_stat(cardpeek_dir,&sbuf)==0)
    {
	log_printf(LOG_DEBUG,"Found directory '%s'",cardpeek_dir);
    }
    else
    {
        astr = a_strnew(NULL);
        a_sprintf(astr,"Could not find the directory '%s'.\nCardpeek is not correctly installed on your system.", cardpeek_dir);
        log_printf(LOG_ERROR,"Cardpeek is not correctly installed: could not find '%s'.", cardpeek_dir);
        ui_question(a_strval(astr),"OK",NULL);
	a_strfree(astr);
    }
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
            if (fscanf(f,"%u",&dot_version)!=1)
                dot_version=0;
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

        if ((response = ui_question(a_strval(astr),"Yes","No","No, don't ask me again",NULL))!=0)
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

        if (ui_question(a_strval(astr),"Yes","No",NULL)!=0)
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
            ui_question(a_strval(astr),"OK",NULL);
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
    dot_cardpeek_tar_gz = g_resources_lookup_data("/com/pannetrat/cardpeek/dot_cardpeek.tar.gz",G_RESOURCE_LOOKUP_FLAGS_NONE,NULL);
    if (dot_cardpeek_tar_gz == NULL)
    {
        log_printf(LOG_ERROR,"Could not load .cardpeek.tar.gz");
        return -1;
    }
    dot_cardpeek_tar_gz_start = (unsigned char *)g_bytes_get_data(dot_cardpeek_tar_gz,&dot_cardpeek_tar_gz_size);

    if (chdir(cardpeek_dir)==-1)
    {
        log_printf(LOG_ERROR,"Could not change directory to '%s'",cardpeek_dir);
        return 0;
    }

    if ((f = g_fopen("dot_cardpeek.tar.gz","wb"))==NULL)
    {
        log_printf(LOG_ERROR,"Could not create dot_cardpeek.tar.gz in %s (%s)", cardpeek_dir, strerror(errno));
        ui_question("Could not create dot_cardpeek.tar.gz, aborting.","Ok",NULL);
        return 0;
    }

    if (fwrite(dot_cardpeek_tar_gz_start,dot_cardpeek_tar_gz_size,1,f)!=1)
    {
        log_printf(LOG_ERROR,"Could not write to dot_cardpeek.tar.gz in %s (%s)", cardpeek_dir, strerror(errno));
        ui_question("Could not write to dot_cardpeek.tar.gz, aborting.","Ok",NULL);
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
        ui_question("Extraction of dot_cardpeek.tar.gz failed, aborting.","Ok",NULL);
        return 0;
    }
    status = system("rm dot_cardpeek.tar.gz");
    log_printf(LOG_INFO,"'rm dot_cardpeek.tar.gz' returned %i",status);

    ui_question("Note: The files have been created.\nIt is recommended that you quit and restart cardpeek, for changes to take effect.","Ok",NULL);
    return 1;
}

#ifdef HAVE_DECL_BACKTRACE
#include <execinfo.h>
static void do_backtrace()
{
    char **btrace;
    void *buffer[32];
    int i,depth;

    depth=backtrace(buffer,32);

    btrace = backtrace_symbols(buffer,depth);

    for (i=0; i<depth; i++)
    {
        log_printf(LOG_DEBUG,"backtrace: %s",btrace[i]);
    }

    free(btrace);
}
#else
static void do_backtrace()
{
    /* void */
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

    if (write(2,message,strlen(message))<=0) return;
    logfile = path_config_get_string(PATH_CONFIG_FILE_CARDPEEK_LOG);
    if (write(2,logfile,strlen(logfile))<=0) return;
    if (write(2,signature,strlen(signature))<=0) return;
    sprintf(buf,"Received signal %i\n",sig_num);
    if (write(2,buf,strlen(buf))<=0) return;

    log_printf(LOG_ERROR,"Received signal %i",sig_num);
    do_backtrace();
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

    fprintf(stdout,"This is %s.\n",system_string_info());
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

char *cardpeek_help =
   " --reader   Select a reader by name (see -v option to get a list of readers).\n" 
   " \n"
   " --exec     Execute the command.\n" 
   " \n"
   " --version  Print version of cardpeek, configuration path and list of detected card readers and replay files.\n"
   " \n"
   " --console Run in console mode, disabling the GTK GUI."
   " \n";


static void display_help(char *progname)
{
    fprintf(stderr, "Usage: %s [-r|--reader reader-name] [-e|--exec lua-command] [-v|--version] [-c|--console]\n\n%s\n",
            progname,cardpeek_help);
}

#ifdef _WIN32
/* #define _WIN32_WINNT 0x0501 */
#include <windows.h>
void init_console(int detach)
{
   /* Enable console output when run from CLI */
    if (!detach)
    	AttachConsole(ATTACH_PARENT_PROCESS);
    else
    	AllocConsole();
    
    freopen("CONIN$", "r",stdin);
    freopen("CONOUT$","w",stdout);
    freopen("CONOUT$","w",stderr);
}
#else
void init_console(int detach)
{
    if (detach)
        log_printf(LOG_WARNING,"The detach option is not implemented on this platform."); 
	/* void */
}
#endif

static struct option long_options[] =
{
    {"reader",  	required_argument, 0,  'r' },
    {"exec",    	required_argument, 0,  'e' },
    {"version", 	no_argument, 	   0,  'v' },
    {"help", 		no_argument, 	   0,  'h' },
    {"console",         no_argument,       0,  'c' },
    {"detach",		no_argument,	   0,  'D' },
    {"generate-dot-cardpeek",		no_argument,	   0,  'G' },
    {0,        	 	0,                 0,   0 }
};

int cardpeek_main(int argc, char **argv)
{
    cardmanager_t* CTX;
    cardreader_t* READER;
    int opt;
    int opt_index = 0;
    char* reader_name = NULL;
    char* exec_command = NULL;
    ui_driver_t *ui_driver = ui_driver_for_gtk();
    int detach_flag = 0;
    int generate_dot_cardpeek_flag = 0;
   

#if !GLIB_CHECK_VERSION(2,36,0)
    g_type_init();
#endif

#ifndef _WIN32
    SSL_load_error_strings();
#endif
    path_config_init();
    
    while ((opt = getopt_long(argc,argv,"r:e:vhcD",long_options,&opt_index))!=-1)
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
		init_console(detach_flag);
                display_readers_and_version();
                path_config_release();
                exit(0);
                break;
            case 'c':
                ui_driver = ui_driver_for_console();
                break;
	    case 'D':
		detach_flag = 1;
		break;
	    case 'G':
		generate_dot_cardpeek_flag = 1;
		break;
            default:
		init_console(detach_flag);
                display_help(argv[0]);
                path_config_release();
                exit(1);
        }
    }

    init_console(detach_flag);

    signal(SIGSEGV, save_what_can_be_saved);

    log_open_file();

    ui_initialize(ui_driver,&argc,&argv);

    log_printf(LOG_INFO,"Running in %s mode on %s",ui_driver_name(),system_string_info());

    luax_init();

#ifndef _WIN32
    install_dot_file();
#else
    if (generate_dot_cardpeek_flag)
        install_dot_file();
    else
        check_cardpeek_dir_exists();
#endif

    if (reader_name == NULL)
    {
        CTX = cardmanager_new();

        reader_name = ui_select_reader(cardmanager_count_readers(CTX),
                cardmanager_reader_name_list(CTX));

        cardmanager_free(CTX);
    }

    READER = cardreader_new(reader_name);

    if (READER)
    {
        luax_set_card_reader(READER);

        cardreader_set_callback(READER,ui_card_event_print,NULL);

        if (exec_command==NULL)
            update_cardpeek();

        ui_run(exec_command);

        cardreader_free(READER);
    }
    else
    {
        fprintf(stderr,"Failed to open smart card reader '%s'.\n",reader_name);
        log_printf(LOG_ERROR,"Failed to open smart card reader '%s'.", reader_name);
    }

    luax_config_table_save();

    luax_release();

    if (reader_name) g_free(reader_name);

    log_close_file();

    path_config_release();

    ERR_free_strings();

    return 0;
}


