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
#include <ctype.h>
#include <stdlib.h>

const char* iso7816_error_code(unsigned short sw);

int cardmanager_search_pcsc_readers(cardmanager_t *cm);

/********************************************************************
 * CARDMANAGER
 */

cardmanager_t *cardmanager_new()
{
  cardmanager_t *cm = (cardmanager_t *)malloc(sizeof(cardmanager_t));
  memset(cm,0,sizeof(cardmanager_t));

  cardmanager_search_pcsc_readers(cm);

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

cardreader_t* cardreader_new(const char *card_reader_name)
{
  cardreader_t* reader = (cardreader_t *)malloc(sizeof(cardreader_t));
  memset(reader,0,sizeof(cardreader_t));

  if (card_reader_name==NULL)
  {
    null_initialize(reader);
    return reader;
  }
  if (strncmp(card_reader_name,"pcsc://",7)==0)
  {
    reader->name=strdup(card_reader_name);
    pcsc_initialize(reader);
    return reader;
  }
 
  free(reader);
  log_printf(LOG_ERROR,"Unknown reader type : %s",card_reader_name);
  return NULL;
}

int cardreader_connect(cardreader_t *reader, unsigned protocol)
{
  return reader->connect(reader,protocol);
}

int cardreader_disconnect(cardreader_t *reader)
{
  return reader->disconnect(reader);
}

int cardreader_warm_reset(cardreader_t *reader)
{
  return reader->reset(reader);
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

  tmp = bytestring_to_hex_string(command);
  log_printf(LOG_INFO,"send: %s", tmp);
  free(tmp);

  reader->sw = reader->transmit(reader,command,result);
  SW1 = (reader->sw>>8)&0xFF;
  SW2 = reader->sw&0xFF;

  tmp = bytestring_to_hex_string(result);
  log_printf(LOG_INFO,"recv: %04X %s [%s]", reader->sw, tmp, iso7816_error_code(reader->sw));
  free(tmp);


  if (SW1==0x6C) /* Re-issue with right length */
  {
    command_dup = bytestring_duplicate(command);
    bytestring_set_uchar(command_dup,-1,SW2);
    reader->sw = pcsc_transmit(reader,command_dup,result);
    bytestring_free(command_dup);
  }

  if (SW1==0x61) /* use Get Response */
  {
    get_response = bytestring_new_from_hex("00C0000000");
    tmp_response = bytestring_new();

    bytestring_set_uchar(get_response,4,SW2);

    reader->sw = pcsc_transmit(reader,get_response,tmp_response);
    bytestring_append_data(result,
                           bytestring_get_size(tmp_response),
                           bytestring_get_data(tmp_response));
    bytestring_free(get_response);
    bytestring_free(tmp_response);
  }

  return reader->sw; 
}

unsigned short cardreader_get_sw(cardreader_t *reader)
{
  return reader->sw;
}

bytestring_t *cardreader_last_atr(cardreader_t *reader)
{
  return reader->last_atr(reader);
}

int cardreader_fail(cardreader_t *reader)
{
  return reader->fail(reader);
}

void cardreader_free(cardreader_t *reader)
{
  reader->finalize(reader);
  free(reader->name);
  free(reader);
}

/********************************************************************/

const char* iso7816_error_code(unsigned short sw)
{
  static char msg[200];

  msg[0]=0;

  if (sw==0x9000)
    return strcpy(msg,"Normal processing");

  switch (sw>>8) {
    case 0x61: 
      strcpy(msg,"More bytes available (see SW2)");
      break;
    case 0x62: 
      strcpy(msg,"State of non-volatile memory unchanged - ");
      switch (sw&0xFF) {
	case 0x00: strcat(msg,"No information given"); break;
	case 0x81: strcat(msg,"Part of returned data may be corrupted"); break;
	case 0x82: strcat(msg,"End of file/record reached before reading Le bytes"); break;
	case 0x83: strcat(msg,"Selected file invalidated"); break;
	case 0x84: strcat(msg,"FCI not formatted correctly"); break;
      }
      break;
    case 0x63:
      strcpy(msg,"State of non-volatile memory changed - ");
      switch (sw&0xFF) {
	case 0x00: strcat(msg,"No information given"); break;
	case 0x81: strcat(msg,"File filled up by the last write"); break;
      }
      if ((sw&0xF0)==0xC0) strcat(msg,"Counter value");
      break;
    case 0x64:
      strcpy(msg,"State of non-volatile memory unchanged - ");
      if ((sw&0xFF)==0) strcat(msg,"OK");
      break;
    case 0x65:
      strcpy(msg,"State of non-volatile memory changed - ");
      switch (sw&0xFF) {
	case 0x00: strcat(msg,"No information given"); break;
	case 0x81: strcat(msg,"Memory failure"); break;
      }
      break;
    case 0x66:
      strcpy(msg,"security-related issue - ");
      switch (sw&0xFF) {
	case 0x00: strcat(msg,"No information given"); break;
      }
      break;
    case 0x67:
      if (sw==0x6700) 
	strcpy(msg,"Wrong length");
      else
	strcpy(msg,"Unknown 67XX error code");
      break;
    case 0x68:
      strcpy(msg,"Functions in CLA not supported - ");
      switch (sw&0xFF) {
	case 0x00: strcat(msg,"No information given"); break;
	case 0x81: strcat(msg,"Logical channel not supported"); break;
	case 0x82: strcat(msg,"Secure messaging not supported"); break;
      }
      break;
    case 0x69:
      strcpy(msg,"Command not allowed - ");
      switch (sw&0xFF) {
	case 0x00: strcat(msg,"No information given"); break;
	case 0x81: strcat(msg,"Command incompatible with file structure"); break;
	case 0x82: strcat(msg,"Security status not satisfied"); break;
	case 0x83: strcat(msg,"Authentication method blocked"); break;
	case 0x84: strcat(msg,"Referenced data invalidated"); break;
	case 0x85: strcat(msg,"Conditions of use not satisfied"); break;
	case 0x86: strcat(msg,"Command not allowed (no current EF)"); break;
	case 0x87: strcat(msg,"Expected SM data objects missing"); break;
	case 0x88: strcat(msg,"SM data objects incorrect"); break;
      }
      break;
    case 0x6A:
      strcpy(msg,"Wrong parameter(s) P1-P2 - ");
      switch (sw&0xFF) {
	case 0x00: strcat(msg,"No information given"); break;
	case 0x80: strcat(msg,"Incorrect parameters in the data field"); break;
	case 0x81: strcat(msg,"Function not supported"); break;
	case 0x82: strcat(msg,"File not found"); break;
	case 0x83: strcat(msg,"Record not found"); break;
	case 0x84: strcat(msg,"Not enough memory space in the file"); break;
	case 0x85: strcat(msg,"Lc inconsistent with TLV structure"); break;
	case 0x86: strcat(msg,"Incorrect parameters P1-P2"); break;
	case 0x87: strcat(msg,"Lc inconsistent with P1-P2"); break;
	case 0x88: strcat(msg,"Referenced data not found"); break;
      }
      break;
    case 0x6B:
      if (sw==0x6B00)
        strcpy(msg,"Wrong parameter(s) P1-P2");
      else
        strcpy(msg,"Unknown 6BXX error code");
      break;
    case 0x6C:
      strcpy(msg,"Wrong length Le, see SW2");
      break;
    case 0x6D:
      if (sw==0x6D00)
        strcpy(msg,"Instruction code not supported or invalid");
      else
        strcpy(msg,"Unknown 6DXX error code");
      break;
    case 0x6E:
      if (sw==0x6E00)
        strcpy(msg,"Class not supported");
      else
        strcpy(msg,"Unknown 6EXX error code");
      break;
    case 0x6F:
      strcpy(msg,"No precise diagnosis");
      break;
    default:
      strcpy(msg,"** Unkown error code **");
  }
  return msg;
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

  status = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL, 
				 &hcontext);
  if (status!=SCARD_S_SUCCESS)
  {
    log_printf(LOG_INFO,"Failed to establish PCSC card manager context");
    return 0;
  }

  status = SCardListReaders(hcontext, NULL, NULL, &dwReaders);
  if (status!=SCARD_S_SUCCESS)
  {
    log_printf(LOG_WARNING,"No PCSC reader connected");
    return 0;
  }

  readers=(char *)malloc(dwReaders);
  status = SCardListReaders(hcontext, NULL, readers, &dwReaders);

  if (status!=SCARD_S_SUCCESS)
  {
    log_printf(LOG_WARNING,"PCSC Reader list failed");
    return 0;
  }

  p=readers;
  cm->readers_count=0;
  while (*p)
  {
    cm->readers_count++;
    p+=strlen(p)+1;
  }
  cm->readers=(char **)malloc(sizeof(char*)*cm->readers_count);

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
    log_printf(LOG_ERROR,"Failed to release PCSC context");

  return cm->readers_count;
}


