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

#include "smartcard.h"
#include "misc.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include "pathconfig.h"
#include "lua_ext.h"
#include "iso7816.h"
#ifdef _WIN32
#include "win32/win32compat.h"
#else
#include <ctype.h>
#endif

int cardmanager_search_pcsc_readers(cardmanager_t *cm);
int cardmanager_search_emul_readers(cardmanager_t *cm);
#ifndef _WIN32
int cardmanager_search_acg_readers(cardmanager_t *cm);
#endif

/********************************************************************
 * CARDMANAGER
 */

cardmanager_t *cardmanager_new()
{
  cardmanager_t *cm = (cardmanager_t *)malloc(sizeof(cardmanager_t));
  memset(cm,0,sizeof(cardmanager_t));

  cardmanager_search_pcsc_readers(cm);
#ifndef _WIN32
  cardmanager_search_acg_readers(cm);
#endif

  cardmanager_search_emul_readers(cm);
  return cm;
}

void cardmanager_free(cardmanager_t* cm)
{
  unsigned u;
  for (u=0;u<cm->readers_count;u++)
    free(cm->readers[u]);
  if (cm->readers)
    free(cm->readers);
}

unsigned cardmanager_count_readers(cardmanager_t* cm)
{
  return cm->readers_count;
}

const char *cardmanager_reader_name(cardmanager_t* cm, unsigned index)
{
  if (index<cm->readers_count)
    return cm->readers[index];
  return NULL;
}

const char **cardmanager_reader_name_list(cardmanager_t* cm)
{
  return (const char **)cm->readers;
}

/********************************************************************
 * CARDREADER
 */


#include "drivers/null_driver.c"
#include "drivers/pcsc_driver.c"
#include "drivers/emul_driver.c"
#ifndef _WIN32
#include "drivers/acg_driver.c"
#endif

cardreader_t* cardreader_new(const char *card_reader_name)
{
  cardreader_t* reader = (cardreader_t *)malloc(sizeof(cardreader_t));
  memset(reader,0,sizeof(cardreader_t));

  if (card_reader_name==NULL)
  {
    reader->name=strdup("(none)");
    null_initialize(reader);
  }
  else if (strncmp(card_reader_name,"pcsc://",7)==0)
  {
    reader->name=strdup(card_reader_name);
    pcsc_initialize(reader);
  }
  else if (strncmp(card_reader_name,"emulator://",11)==0)
  {
    reader->name=strdup(card_reader_name);
    emul_initialize(reader);
  }
#ifndef _WIN32
  else if (strncmp(card_reader_name,"acg://",6)==0)
  {
    reader->name=strdup(card_reader_name);
    acg_initialize(reader);
  }
#endif
  else {
    free(reader);
    log_printf(LOG_ERROR,"Unknown reader type : %s",card_reader_name);
    return NULL;
  }
 
  reader->atr=bytestring_new(8);
  reader->cardlog=cardemul_new();
  return reader;
}

int cardreader_connect(cardreader_t *reader, unsigned protocol)
{
  int retval = reader->connect(reader,protocol);

  cardreader_last_atr(reader);

  cardemul_add_reset(reader->cardlog,reader->atr);
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

  cardemul_add_reset(reader->cardlog,reader->atr); 
  if (reader->cb_func)
    reader->cb_func(CARDREADER_EVENT_RESET,reader->atr,0,NULL,reader->cb_data);
  return retval;
}

unsigned short cardreader_transmit(cardreader_t *reader,
	      			   const bytestring_t* command, 
				   bytestring_t *result)
{
  unsigned char SW1;
  unsigned char SW2;
  bytestring_t* command_dup;
  bytestring_t* get_response;
  bytestring_t* tmp_response;
  char *tmp;
  apdu_descriptor_t ad;

  if (iso7816_describe_apdu(&ad,command)!=ISO7816_OK)
  {
    log_printf(LOG_ERROR,"Could not parse APDU format");
    return CARDPEEK_ERROR_SW;
  }


  tmp = bytestring_to_format("%D",command);
  if (strlen(tmp)>37)
    strcpy(tmp+32,"(...)");
  log_printf(LOG_INFO,"send: %s [%s]", tmp, iso7816_stringify_apdu_class(ad.apdu_class));
  free(tmp);


  reader->sw = reader->transmit(reader,command,result);
  
  cardemul_add_command(reader->cardlog,command,reader->sw,result);
  if (reader->cb_func)
    reader->cb_func(CARDREADER_EVENT_TRANSMIT,command,reader->sw,result,reader->cb_data);
  SW1 = (reader->sw>>8)&0xFF;
  SW2 = reader->sw&0xFF;

  tmp = bytestring_to_format("%D",result);
  if (strlen(tmp)>37)
    strcpy(tmp+32,"(...)");
  log_printf(LOG_INFO,"Recv: %04X %s [%s]", reader->sw, tmp, iso7816_stringify_sw(reader->sw));
  free(tmp);



  if (SW1==0x6C) /* Re-issue with right length */
  {
    command_dup = bytestring_duplicate(command);
    if (ad.le_len==3) /* in case of extended le */
      bytestring_resize(command_dup,bytestring_get_size(command_dup)-2);
    bytestring_set_element(command_dup,-1,SW2);
    reader->sw = cardreader_transmit(reader,command_dup,result);
    bytestring_free(command_dup);
  }

  while (SW1==0x61) /* use Get Response */
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
  return reader->last_atr(reader);
}

char** cardreader_get_info(cardreader_t *reader)
{
  char SW_string[5];
  char **prev = malloc(sizeof(char*)*(2*5+1));

  prev[0]=strdup("Name");
  prev[1]=strdup(reader->name);
  prev[2]=strdup("Connected");
  if (reader->connected)
    prev[3]=strdup("TRUE");
  else
    prev[3]=strdup("FALSE");
  prev[4]=strdup("Protocol");
  if (reader->protocol==PROTOCOL_T0)
    prev[5]=strdup("T0");
  else if (reader->protocol==PROTOCOL_T1)
    prev[5]=strdup("T1");
  else
    prev[5]=strdup("Undefined");
  prev[6]=strdup("SW");
  sprintf(SW_string,"%04X",reader->sw);
  prev[7]=strdup(SW_string);
  prev[8]=strdup("CommandInterval");
  prev[9]=strdup("0");
  prev[10]=NULL;
  return reader->get_info(reader, prev);
}

int cardreader_fail(cardreader_t *reader)
{
  return reader->fail(reader);
}

void cardreader_free(cardreader_t *reader)
{
  reader->finalize(reader);
  bytestring_free(reader->atr);
  cardemul_free(reader->cardlog);
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
  cardemul_free(reader->cardlog);
  reader->cardlog=cardemul_new();
}

int cardreader_log_save(const cardreader_t *reader, const char *filename)
{
  if (reader->cb_func)
    reader->cb_func(CARDREADER_EVENT_SAVE_LOG,NULL,0,NULL,reader->cb_data);
  return cardemul_save_to_file(reader->cardlog,filename);
}

int cardreader_log_count_records(const cardreader_t *reader)
{
  return cardemul_count_records(reader->cardlog);
}

/************************************************/

/* this should not be here but in pcsc_driver.c */
int cardmanager_check_pcscd_is_running()
{
  int fd=open("/var/pid/pcscd.pid",O_RDONLY);
  if (fd<0) return 0;
  close(fd);
  return 1;
}

/* this should not be here but in pcsc_driver.c */
int cardmanager_search_pcsc_readers(cardmanager_t *cm)
{
  DWORD dwReaders;
  char *p;
  LONG hcontext;
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
      log_printf(LOG_INFO,"The pcscd deamon does not seem to be running.");
    else
      log_printf(LOG_INFO,"The pcscd deamon seems to be running.");
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
  cm->readers_count=0;
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

int select_clf(const struct dirent* de)
{
  char *ext=rindex(de->d_name,'.');
  if (ext && strcmp(ext,".clf")==0)
    return 1;
  return 0;
}


int cardmanager_search_emul_readers(cardmanager_t *cm)
{
  a_string_t* fn;
  struct dirent **namelist;
  const char* log_folder = config_get_string(CONFIG_FOLDER_LOGS); 
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
    a_sprintf(fn,"emulator://%s",namelist[n]->d_name);
    cm->readers[cm->readers_count++]=a_strfinalize(fn);
    free(namelist[n]);
  }
  free(namelist);
  return count;
}
#ifndef _WIN32
int cardmanager_search_acg_readers(cardmanager_t *cm)
{
  a_string_t* reader;
  const char *reader_name;
  int count=0;
  const char *device_name;

  device_name = luax_get_string_value("ACG_MULTI_ISO_TTY");
  if (device_name)
  {
    reader_name=acg_detect(device_name);
    if (reader_name)
    {
      count++;
      cm->readers=(char **)realloc(cm->readers,sizeof(char*)*(cm->readers_count+1));
      reader=a_strnew("acg://");
      a_strcat(reader,device_name);
      a_strcat(reader,"/");
      a_strcat(reader,reader_name);
      cm->readers[cm->readers_count++]=a_strfinalize(reader);
    }
  }
 
  device_name = luax_get_string_value("ACG_LFX_TTY");
  if (device_name)
  {
    reader_name=acg_detect(device_name);
    if (reader_name)
    {
      count++;
      cm->readers=(char **)realloc(cm->readers,sizeof(char*)*(cm->readers_count+1));
      reader=a_strnew("acg://");
      a_strcat(reader,device_name);
      a_strcat(reader,"/");
      a_strcat(reader,reader_name);
      cm->readers[cm->readers_count++]=a_strfinalize(reader);
    }
  }
  return count;
}
#endif
void cardreader_set_callback(cardreader_t *reader, cardreader_callback_t func, void *user_data)
{
  reader->cb_func=func;
  reader->cb_data = user_data;
}
