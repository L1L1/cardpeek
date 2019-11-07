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

#include "smartcard.h"
#include "a_string.h"
#include "misc.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <glib/gstdio.h>
#include "pathconfig.h"
#include "lua_ext.h"
#include "iso7816.h"

#ifdef _WIN32
#include "win32/win32compat.h"
#else
#include <ctype.h>
#endif

int cardmanager_search_pcsc_readers(cardmanager_t *cm);
int cardmanager_search_usbserial_readers(cardmanager_t *cm);
int cardmanager_search_replay_readers(cardmanager_t *cm);

/********************************************************************
 * CARDMANAGER
 */

cardmanager_t *cardmanager_new(void)
{
  cardmanager_t *cm = (cardmanager_t *)malloc(sizeof(cardmanager_t));
  memset(cm,0,sizeof(cardmanager_t));
  cm->readers_count=0;
  cardmanager_search_pcsc_readers(cm);
  #if defined(__unix__)
    cardmanager_search_usbserial_readers(cm);
  #endif
  cardmanager_search_replay_readers(cm);
  return cm;
}

void cardmanager_free(cardmanager_t* cm)
{
  unsigned u;
  for (u=0;u<cm->readers_count;u++)
    free(cm->readers[u]);
  if (cm->readers)
    free(cm->readers);
  free(cm);
}

unsigned cardmanager_count_readers(cardmanager_t* cm)
{
  return cm->readers_count;
}

const char *cardmanager_reader_name(cardmanager_t* cm, unsigned n_index)
{
  if (n_index<cm->readers_count)
    return cm->readers[n_index];
  return NULL;
}

const char **cardmanager_reader_name_list(cardmanager_t* cm)
{
  return (const char **)cm->readers;
}

/********************************************************************
 * CARDREADER
 */

#include "ui.h"
#include "drivers/null_driver.c"
#include "drivers/pcsc_driver.c"
#include "drivers/usbserial_driver.c"
#include "drivers/replay_driver.c"

cardreader_t* cardreader_new(const char *card_reader_name)
{
  cardreader_t* reader = (cardreader_t *)malloc(sizeof(cardreader_t));
  memset(reader,0,sizeof(cardreader_t));

  if (card_reader_name==NULL || strcmp(card_reader_name,"none")==0)
  {
    reader->name=strdup("(none)");
    if (null_initialize(reader)==0) return NULL;
  }
  else if (strncmp(card_reader_name,"pcsc://",7)==0)
  {
    reader->name=strdup(card_reader_name);
    if (pcsc_initialize(reader)==0) return NULL;
  }
  else if (strncmp(card_reader_name,"usbserial://",12)==0)
  {
    reader->name=strdup(card_reader_name);
    if (usbserial_initialize(reader)==0) return NULL;
  }
  else if (strncmp(card_reader_name,"replay://",9)==0)
  {
    reader->name=strdup(card_reader_name);
    if (replay_initialize(reader)==0) return NULL;
  }
  else 
  {
    free(reader);
    log_printf(LOG_ERROR,"Unknown reader type : %s",card_reader_name);
    return NULL;
  }
 
  reader->atr=bytestring_new(8);
  reader->cardlog=cardreplay_new();
  return reader;
}

int cardreader_connect(cardreader_t *reader, unsigned protocol)
{
  int retval = reader->connect(reader,protocol);

  reader->last_atr(reader);

  cardreplay_add_reset(reader->cardlog,reader->atr);
  if (reader->cb_func)
    reader->cb_func(CARDREADER_EVENT_CONNECT,reader->atr,0,NULL,reader->cb_data);
  return retval;
}

int cardreader_disconnect(cardreader_t *reader)
{
  if (reader->cb_func)
    reader->cb_func(CARDREADER_EVENT_DISCONNECT,NULL,0,NULL,reader->cb_data);
  return reader->disconnect(reader);
}

int cardreader_warm_reset(cardreader_t *reader)
{
  int retval = reader->reset(reader);
  
  cardreader_last_atr(reader);

  cardreplay_add_reset(reader->cardlog,reader->atr); 
  if (reader->cb_func)
    reader->cb_func(CARDREADER_EVENT_RESET,reader->atr,0,NULL,reader->cb_data);
  return retval;
}

unsigned short cardreader_transmit(cardreader_t *reader,
	      			   const bytestring_t* command, 
				   bytestring_t *result)
{
  /*
    unsigned char SW1;
  unsigned char SW2;
  bytestring_t* command_dup;
  bytestring_t* get_response;
  bytestring_t* tmp_response;
  char *tmp;
  apdu_descriptor_t ad;

  if (iso7816_describe_apdu(&ad,command)!=ISO7816_OK)
  {
    tmp = bytestring_to_format("%D",command);
    log_printf(LOG_ERROR,"Could not parse APDU format: %s",tmp);
    free(tmp);
    return CARDPEEK_ERROR_SW;
  }


  tmp = bytestring_to_format("%D",command);
  if (strlen(tmp)>37)
    strcpy(tmp+32,"(...)");
  log_printf(LOG_INFO,"send: %s [%s]", tmp, iso7816_stringify_apdu_class(ad.apdu_class));
  free(tmp);
    */

  reader->sw = reader->transmit(reader,command,result);
  
  cardreplay_add_command(reader->cardlog,command,reader->sw,result);
  if (reader->cb_func)
    reader->cb_func(CARDREADER_EVENT_TRANSMIT,command,reader->sw,result,reader->cb_data);
  /*
    SW1 = (reader->sw>>8)&0xFF;
  SW2 = reader->sw&0xFF;

  tmp = bytestring_to_format("%D",result);
  if (strlen(tmp)>37)
    strcpy(tmp+32,"(...)");
  log_printf(LOG_INFO,"Recv: %04X %s [%s]", reader->sw, tmp, iso7816_stringify_sw(reader->sw));
  free(tmp);



  if (SW1==0x6C) 
  {
    command_dup = bytestring_duplicate(command);
    if (ad.le_len==3) 
    bytestring_resize(command_dup,bytestring_get_size(command_dup)-2);
    bytestring_set_element(command_dup,-1,SW2);
    reader->sw = cardreader_transmit(reader,command_dup,result);
    bytestring_free(command_dup);
  }

  while (SW1==0x61) 
  {
    get_response = bytestring_new_from_string("8:00C0000000");
    tmp_response = bytestring_new(8);

    bytestring_set_element(get_response,4,SW2);

    reader->sw = cardreader_transmit(reader,get_response,tmp_response);
 
    bytestring_append_data(result,
                           bytestring_get_size(tmp_response),
                           bytestring_get_data(tmp_response));

    bytestring_free(get_response);
    bytestring_free(tmp_response);
    SW1 = (reader->sw>>8)&0xFF;
    SW2 = reader->sw&0xFF;
  }
    */
  if (reader->command_interval)
    usleep(reader->command_interval);

  return reader->sw; 
}

unsigned short cardreader_get_sw(cardreader_t *reader)
{
  return reader->sw;
}

const bytestring_t *cardreader_last_atr(cardreader_t *reader)
{
  const bytestring_t *atr = reader->last_atr(reader);
  char *tmp;

  if (atr)
  {
	tmp = bytestring_to_format("%D",atr);
        log_printf(LOG_INFO,"ATR is %i bytes: %s",bytestring_get_size(atr),tmp);
        free(tmp);
  }
  return atr;
}

char** cardreader_get_info(cardreader_t *reader)
{
  char SW_string[5];
  unsigned info_count;  
  char **info = reader->get_info(reader);

  if (info==NULL)
	info_count = 0;
  else
	for (info_count=0; info[info_count]!=NULL; info_count++);

  info = g_realloc(info, sizeof(char*)*(11+info_count));

  info[info_count+0]=g_strdup("reader");
  info[info_count+1]=g_strdup(reader->name);

  info[info_count+2]=g_strdup("connected");
  if (reader->connected)
    info[info_count+3]=g_strdup("TRUE");
  else
    info[info_count+3]=g_strdup("FALSE");

  info[info_count+4]=g_strdup("protocol");
  if (reader->protocol==PROTOCOL_T0)
    info[info_count+5]=g_strdup("T0");
  else if (reader->protocol==PROTOCOL_T1)
    info[info_count+5]=g_strdup("T1");
  else
    info[info_count+5]=g_strdup("Undefined");

  info[info_count+6]=g_strdup("sw");
  sprintf(SW_string,"%u",(unsigned)(reader->sw & 0xFFFF));
  info[info_count+7]=g_strdup(SW_string);

  info[info_count+8]=g_strdup("command_interval");
  info[info_count+9]=g_strdup("0");

  info[info_count+10]=NULL;
  return info;
}

int cardreader_fail(cardreader_t *reader)
{
  return reader->fail(reader);
}

void cardreader_free(cardreader_t *reader)
{
  reader->finalize(reader);
  bytestring_free(reader->atr);
  cardreplay_free(reader->cardlog);
  free(reader->name);
  free(reader);
}


void cardreader_set_command_interval(cardreader_t *reader, unsigned interval)
{
  reader->command_interval=interval;
}

void cardreader_log_clear(cardreader_t *reader)
{
  if (reader->cb_func)
    reader->cb_func(CARDREADER_EVENT_CLEAR_LOG,NULL,0,NULL,reader->cb_data);
  cardreplay_free(reader->cardlog);
  reader->cardlog=cardreplay_new();
}

int cardreader_log_save(const cardreader_t *reader, const char *filename)
{
  if (reader->cb_func)
    reader->cb_func(CARDREADER_EVENT_SAVE_LOG,NULL,0,NULL,reader->cb_data);
  return cardreplay_save_to_file(reader->cardlog,filename);
}

int cardreader_log_count_records(const cardreader_t *reader)
{
  return cardreplay_count_records(reader->cardlog);
}

/************************************************/

/* this should not be here but in pcsc_driver.c */
static int cardmanager_check_pcscd_is_running(void)
{
  int fd=g_open("/var/pid/pcscd.pid",O_RDONLY,0);
  if (fd<0) return 0;
  close(fd);
  return 1;
}


/* this should not be here but in usbserial_driver.c */
int cardmanager_search_usbserial_readers(cardmanager_t *cm)
{
  unsigned r;
  a_string_t *rlist;
  struct dirent **readers;
  int usbserial_readers_count;
  unsigned int old_readers_count;

  usbserial_readers_count = scandir("/dev/serial/by-id", &readers, NULL, alphasort);
  if (usbserial_readers_count<2) {
    log_printf(LOG_DEBUG, "No usbserial readers found.");
    return 0;
  }
  
  old_readers_count=cm->readers_count;
  cm->readers_count+=usbserial_readers_count-2; // //first two entries from scandir are '.' and '..'

  cm->readers=(char **)realloc(cm->readers,sizeof(char*)*cm->readers_count);

  rlist = a_strnew(NULL);
  for (r = 2; r < usbserial_readers_count; r++)
  {
    a_strcpy(rlist,"usbserial://");
    a_strcat(rlist,readers[r]->d_name);
    cm->readers[old_readers_count + r - 2 ]=strdup(a_strval(rlist));
  }


  a_strfree(rlist);
  for (int i = 0; i < usbserial_readers_count; i++)
  {
    free(readers[i]);
  }
  free(readers);

  log_printf(LOG_DEBUG,"Found %i usbserial readers", usbserial_readers_count-2);
  log_printf(LOG_DEBUG,"Found %i readers",cm->readers_count);

  return cm->readers_count;
}

/* this should not be here but in pcsc_driver.c */
int cardmanager_search_pcsc_readers(cardmanager_t *cm)
{
  DWORD dwReaders = 0;
  char *p;
  LONG hcontext = 0;
  long status;
  a_string_t *rlist;
  char *readers;
  unsigned r;

  
  status = SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, 
				 &hcontext);
  
  if (status!=SCARD_S_SUCCESS)
  {
    log_printf(LOG_INFO,"Failed to establish PCSC card manager context");
    log_printf(LOG_INFO,"PCSC error code 0x%08X: %s",status,pcsc_stringify_error(status));
    if (cardmanager_check_pcscd_is_running()==0)
      log_printf(LOG_INFO,"The pcscd daemon does not seem to be running.");
    else
      log_printf(LOG_INFO,"The pcscd daemon seems to be running.");
    return 0;
  }

  status = SCardListReaders(hcontext, NULL, NULL, &dwReaders);
  if (status!=SCARD_S_SUCCESS)
  {
    log_printf(LOG_WARNING,"No PCSC reader connected");
    log_printf(LOG_INFO,"PCSC error code 0x%08X: %s",status,pcsc_stringify_error(status));
    return 0;
  }

  readers=(char *)malloc(dwReaders);
  status = SCardListReaders(hcontext, NULL, readers, &dwReaders);

  if (status!=SCARD_S_SUCCESS)
  {
    log_printf(LOG_WARNING,"PCSC Reader list failed");
    log_printf(LOG_INFO,"PCSC error code %lX: %s",status,pcsc_stringify_error(status));
    return 0;
  }

  p=readers;
  while (*p)
  {
    cm->readers_count++;
    p+=strlen(p)+1;
  }
  cm->readers=(char **)realloc(cm->readers,sizeof(char*)*cm->readers_count);

  rlist = a_strnew(NULL);
  p=readers;
  for (r=0;r<cm->readers_count;r++)
  {
    a_strcpy(rlist,"pcsc://");
    a_strcat(rlist,p);
    cm->readers[r]=strdup(a_strval(rlist));
    p+=strlen(p)+1;
  }
  a_strfree(rlist);
  free(readers);
  log_printf(LOG_DEBUG,"Found %i PCSC readers",cm->readers_count);

  status = SCardReleaseContext(hcontext);
  if (status!=SCARD_S_SUCCESS) 
  {
    log_printf(LOG_ERROR,"Failed to release PCSC context");
    log_printf(LOG_INFO,"PCSC error code %lX: %s",status,pcsc_stringify_error(status));
  }

  return cm->readers_count;
}

static int select_clf(DIRENT_T* de)
{
  char *ext=rindex(de->d_name,'.');
  if (ext && strcmp(ext,".clf")==0)
    return 1;
  return 0;
}


int cardmanager_search_replay_readers(cardmanager_t *cm)
{
  a_string_t* fn;
  struct dirent **namelist;
  const char* log_folder = path_config_get_string(PATH_CONFIG_FOLDER_REPLAY); 
  int count,n;

  n = scandir(log_folder,&namelist,select_clf,alphasort);

  if (n<=0)
    return 0;
  count=0;
  
  cm->readers=(char **)realloc(cm->readers,sizeof(char*)*(cm->readers_count+n));
  while (n--)
  {
    count++;
    fn = a_strnew(NULL);
    a_sprintf(fn,"replay://%s",namelist[n]->d_name);
    cm->readers[cm->readers_count++]=a_strfinalize(fn);
    free(namelist[n]);
  }
  free(namelist);
  return count;
}

void cardreader_set_callback(cardreader_t *reader, cardreader_callback_t func, void *user_data)
{
  reader->cb_func=func;
  reader->cb_data = user_data;
}
