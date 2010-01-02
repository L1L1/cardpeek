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

#include <winscard.h>

typedef struct {
  LONG         hcontext;
  SCARDHANDLE  hcard;
  DWORD        protocol;
  long         status;
} pcsc_data_t;


int pcsc_connect(cardreader_t *cr, unsigned prefered_protocol)
{
  pcsc_data_t* pcsc = cr->extra_data;
  
  pcsc->status = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL,
				     &(pcsc->hcontext));
  if (pcsc->status!=SCARD_S_SUCCESS)
  {
    log_printf(LOG_INFO,"Failed to establish PCSC card manager context");
    return 0;
  }

  log_printf(LOG_INFO,"Attempting to connect to '%s'",cr->name);
  pcsc->status = SCardConnect(pcsc->hcontext,
      			      cr->name+7,
      			      SCARD_SHARE_EXCLUSIVE,
      			      prefered_protocol,
      			      &(pcsc->hcard),
      			      &(cr->protocol));
  
  if (pcsc->status!=SCARD_S_SUCCESS)
  {
    log_printf(LOG_ERROR,"Connection failed");
    return 0;
  }

  log_printf(LOG_INFO,"Connection successful");
  cr->connected=1;
  return 1;
}

int pcsc_disconnect(cardreader_t *cr)
{
  pcsc_data_t* pcsc = cr->extra_data;

  pcsc->status = SCardDisconnect(pcsc->hcard,SCARD_UNPOWER_CARD);
  if (pcsc->status==SCARD_S_SUCCESS)
  {
    cr->connected=0;
    log_printf(LOG_INFO,"Disconnected reader");
    return 1;
  }

  log_printf(LOG_ERROR,"Failed to disconnect reader");
  return 0;
}

int pcsc_reset(cardreader_t *cr)
{
  pcsc_data_t* pcsc = cr->extra_data;
  
  pcsc->status = SCardReconnect(pcsc->hcard,
				SCARD_SHARE_EXCLUSIVE,
				cr->protocol,
				SCARD_RESET_CARD,
				&(cr->protocol));
  if (pcsc->status==SCARD_S_SUCCESS)
  {
    log_printf(LOG_INFO,"Reconnected reader");
    cr->connected=1;
    return 1;
  }

  log_printf(LOG_ERROR,"Failed to reconnect reader");
  cr->connected=0;
  return 0;
}

unsigned short pcsc_transmit(cardreader_t* cr,
			     const bytestring_t* command, 
			     bytestring_t* result)
{
  pcsc_data_t* pcsc = cr->extra_data;
  BYTE REC_DAT[258]; /* 256 + 2 bytes SW MAX */
  DWORD REC_LEN=256;
  SCARD_IO_REQUEST pioRecvPci;
  /* char *tmp; */
  unsigned short SW;

/*
   tmp = bytestring_to_hex_string(command);
  log_printf(LOG_INFO,"send: %s", tmp);
  free(tmp);
*/

  if (cr->protocol==SCARD_PROTOCOL_T0)
  {
    pcsc->status = SCardTransmit(pcsc->hcard,SCARD_PCI_T0,
	     			 bytestring_get_data(command), 
				 bytestring_get_size(command),
	     			 &pioRecvPci, REC_DAT,&REC_LEN);
  }
  else
  {
    /* FIXME: incorrect T1 stuff */
    pcsc->status = SCardTransmit(pcsc->hcard,SCARD_PCI_T1,
				 bytestring_get_data(command), 
                                 bytestring_get_size(command),
	     			 &pioRecvPci, REC_DAT,&REC_LEN);

  }

  if (pcsc->status!=SCARD_S_SUCCESS)
  {
    log_printf(LOG_ERROR,"Failed to transmit command to card");
    return 0;
  }

  bytestring_assign_data(result,REC_LEN-2,REC_DAT);

  SW = (REC_DAT[REC_LEN-2]<<8)|REC_DAT[REC_LEN-1];

/*
  tmp = bytestring_to_hex_string(result);
  log_printf(LOG_INFO,"recv: %04X %s [%s]", SW, tmp, iso7816_error_code(SW));
  free(tmp);
*/
  return SW;
}

bytestring_t* pcsc_last_atr(cardreader_t* cr)
{
  pcsc_data_t* pcsc = cr->extra_data;
  DWORD state;
  DWORD protocol;
  BYTE pbAtr[MAX_ATR_SIZE];
  DWORD atrlen=MAX_ATR_SIZE;
  char readername[MAX_READERNAME];
  DWORD readernamelen=MAX_READERNAME;
  bytestring_t* res = bytestring_new();
  char *tmp;

  pcsc->status = SCardStatus(pcsc->hcard,
	      		     readername,&readernamelen,
		   	     &state,
			     &protocol,
			     pbAtr,&atrlen);

  if (pcsc->status==SCARD_S_SUCCESS)
  {
    bytestring_assign_data(res,atrlen,pbAtr);
    tmp = bytestring_to_hex_string(res);
    log_printf(LOG_INFO,"ATR=%s",tmp);
    free(tmp);
  }
  else
    log_printf(LOG_ERROR,"Failed to query card status");
  return res;
}

int pcsc_fail(cardreader_t* cr)
{
  pcsc_data_t* pcsc = cr->extra_data;
  return (pcsc->status!=SCARD_S_SUCCESS);
}

void pcsc_finalize(cardreader_t* cr)
{
  pcsc_data_t* pcsc = cr->extra_data;
  free(pcsc);
}

int pcsc_initialize(cardreader_t *reader)
{
  pcsc_data_t* pcsc = malloc(sizeof(pcsc_data_t));
  
  memset(pcsc,0,sizeof(pcsc_data_t));

  reader->extra_data   = pcsc;

  reader->connect      = pcsc_connect;
  reader->disconnect   = pcsc_disconnect;
  reader->reset        = pcsc_reset;
  reader->transmit     = pcsc_transmit;
  reader->last_atr     = pcsc_last_atr;
  reader->fail         = pcsc_fail;
  reader->finalize     = pcsc_finalize;
  return 1;
}

