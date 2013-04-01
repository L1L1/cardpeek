/********************************************************************** 
*
* This file is part of Cardpeek, the smartcard reader utility.
*
* Copyright 2009-2011 by 'L1L1'
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

#ifdef _WIN32
  /* 
   * On linux pioRecvPci is expected to point somewhere in the 
   * SCardTransmit() call but on windows, we must make it NULL. 
   * So here is an ugly trick. 
   */
  #define SCARD_PCI_NULL NULL
#elif __APPLE__
  SCARD_IO_REQUEST pioRecvPci_dummy;
  #define SCARD_PCI_NULL (&pioRecvPci_dummy)
  #include <wintypes.h>
  #define SCARD_ATTR_MAXINPUT 0x7A007
#else /* Linux et al. */
  SCARD_IO_REQUEST pioRecvPci_dummy;
  #define SCARD_PCI_NULL (&pioRecvPci_dummy)
  #include <reader.h>
#endif

/* #ifndef _WIN32
 * #ifndef __APPLE__
 * #include <reader.h>
 * #endif
 *  SCARD_IO_REQUEST pioRecvPci_dummy;
 * #define SCARD_PCI_NULL (&pioRecvPci_dummy)
 * #else
 * * 
   * On linux pioRecvPci is expected to point somewhere in the 
   * SCardTransmit() call but on windows, we must make it NULL. 
   * So here is an ugly trick. 
   *
 * #define SCARD_PCI_NULL NULL
 * #endif
 * #ifdef __APPLE__
 * #include <wintypes.h>
 * #define SCARD_ATTR_MAXINPUT 0x7A007
 * #endif
 */

#define MAX_PCSC_READ_LENGTH 270 

typedef struct {
  LONG         hcontext;
  SCARDHANDLE  hcard;
  DWORD        protocol;
  long         status;
} pcsc_data_t;

static const char *pcsc_stringify_protocol(DWORD proto)
{
	static char proto_string[32];

        switch (proto) {
		case SCARD_PROTOCOL_T0:
			return "T=0";
		case SCARD_PROTOCOL_T1:
			return "T=1";
		case SCARD_PROTOCOL_RAW:
			return "Raw";
	}
	sprintf(proto_string,"UNKNOWN(0x%x)",(unsigned)proto);
	return proto_string;
}

static const char *pcsc_stringify_state(DWORD state)
{
	static char state_string[500];
	int state_string_len;

	*state_string = 0;			

	if (state & SCARD_STATE_CHANGED)
		strcat(state_string," Changed state,");

	if (state & SCARD_STATE_IGNORE)
		strcat(state_string," Ignore reader,");

	if (state & SCARD_STATE_UNKNOWN)
		strcat(state_string," Unknown reader,");

	if (state & SCARD_STATE_UNAVAILABLE)
		strcat(state_string," Status unavailable,");

	if (state & SCARD_STATE_EMPTY)
		strcat(state_string," Card removed,");

	if (state & SCARD_STATE_PRESENT)
		strcat(state_string," Card present,");

	if (state & SCARD_STATE_EXCLUSIVE)
		strcat(state_string," Exclusive access,");

	if (state & SCARD_STATE_INUSE)
		strcat(state_string," Shared access,");

	if (state & SCARD_STATE_MUTE)
		strcat(state_string," Silent card,");
	
	state_string_len=strlen(state_string);
	if (state_string[state_string_len-1]==',')
		state_string[state_string_len-1]=0;
	else
		strcat(state_string,"UNDEFINED");

	return state_string;
}

static int pcsc_connect(cardreader_t *cr, unsigned prefered_protocol)
{
  DWORD attr_maxinput;
  DWORD attr_maxinput_len = sizeof(unsigned int);
  SCARD_READERSTATE reader_state;

  pcsc_data_t* pcsc = cr->extra_data;
  
  pcsc->status = SCardEstablishContext(SCARD_SCOPE_SYSTEM, NULL, NULL,
  				       &(pcsc->hcontext));
  if (pcsc->status!=SCARD_S_SUCCESS)
  {
    log_printf(LOG_ERROR,"Failed to establish PCSC card manager context: %s (error 0x%08x).",
		    pcsc_stringify_error(pcsc->status),
		    pcsc->status );
    return 0;
  }

  memset(&reader_state,0,sizeof(reader_state));
  reader_state.szReader = cr->name+7;
  reader_state.dwCurrentState = SCARD_STATE_UNAWARE;
  pcsc->status = SCardGetStatusChange(pcsc->hcontext,INFINITE,&reader_state,1);
  if (pcsc->status != SCARD_S_SUCCESS)
  {
     log_printf(LOG_ERROR,"Failed to query reader status before connecting: %s (error 0x%08x).",
			pcsc_stringify_error(pcsc->status),
			pcsc->status );
     return 0;
  }

  while ((reader_state.dwEventState & SCARD_STATE_PRESENT)==0)
  {
     reader_state.dwCurrentState = reader_state.dwEventState;
     log_printf(LOG_INFO,"Waiting for card to be present (current state: %s)...",
			  pcsc_stringify_state(reader_state.dwEventState));
     pcsc->status = SCardGetStatusChange(pcsc->hcontext,3000,&reader_state,1);
     if (pcsc->status != SCARD_S_SUCCESS && pcsc->status != SCARD_E_TIMEOUT)
     {
	log_printf(LOG_ERROR,"Failed to query reader status change before connecting: %s (error 0x%08x).",
			pcsc_stringify_error(pcsc->status),
			pcsc->status );
	return 0;
     }
  }

  log_printf(LOG_DEBUG,"Attempting to connect to '%s'",cr->name);
  pcsc->status = SCardConnect(pcsc->hcontext,
      			      cr->name+7,
      			      SCARD_SHARE_EXCLUSIVE,
      			      prefered_protocol,
      			      &(pcsc->hcard),
      			      &(cr->protocol));
  
  if (pcsc->status!=SCARD_S_SUCCESS)
  {
    log_printf(LOG_ERROR,"Connection failed: %s (error 0x%08x).",
		    pcsc_stringify_error(pcsc->status),
		    pcsc->status );
    return 0;
  }

  if (SCardGetAttrib(pcsc->hcard,SCARD_ATTR_MAXINPUT,(LPBYTE)&attr_maxinput,(LPDWORD)&attr_maxinput_len)==SCARD_S_SUCCESS)
    log_printf(LOG_INFO,"Reader maximum input length is %u bytes",attr_maxinput);
  else
    log_printf(LOG_DEBUG,"Could not determinate reader maximum input length");

  log_printf(LOG_INFO,"Connection successful, protocol is %s",pcsc_stringify_protocol(cr->protocol));
  cr->connected=1;
  return 1;
}

static int pcsc_disconnect(cardreader_t *cr)
{
  pcsc_data_t* pcsc = cr->extra_data;

  pcsc->status = SCardDisconnect(pcsc->hcard,SCARD_UNPOWER_CARD);
  if (pcsc->status==SCARD_S_SUCCESS)
  {
    cr->connected=0;
    log_printf(LOG_INFO,"Disconnected reader");
    return 1;
  }

  log_printf(LOG_ERROR,"Failed to disconnect reader: %s (error 0x%08x).",
		  pcsc_stringify_error(pcsc->status),
		  pcsc->status );	
  return 0;
}

static int pcsc_reset(cardreader_t *cr)
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

  log_printf(LOG_ERROR,"Failed to reconnect reader: %s (error 0x%08x).",
		  pcsc_stringify_error(pcsc->status),
		  pcsc->status );
  cr->connected=0;
  return 0;
}

static unsigned short pcsc_transmit(cardreader_t* cr,
			     const bytestring_t* command, 
			     bytestring_t* result)
{
  pcsc_data_t* pcsc = cr->extra_data;
  BYTE REC_DAT[MAX_PCSC_READ_LENGTH];
  DWORD REC_LEN=MAX_PCSC_READ_LENGTH;
  unsigned short SW;

  if (cr->protocol==SCARD_PROTOCOL_T0)
  {
    pcsc->status = SCardTransmit(pcsc->hcard,SCARD_PCI_T0,
	     			 bytestring_get_data(command), 
				 bytestring_get_size(command),
	     			 SCARD_PCI_NULL, 
				 REC_DAT,&REC_LEN);
  }
  else if (cr->protocol==SCARD_PROTOCOL_T1)
  {
    pcsc->status = SCardTransmit(pcsc->hcard,SCARD_PCI_T1,
				 bytestring_get_data(command), 
                                 bytestring_get_size(command),
	     			 SCARD_PCI_NULL, 
				 REC_DAT,&REC_LEN);

  }
  else
  {
    log_printf(LOG_ERROR,"Unknown smartcard protocol: %i",cr->protocol);
    return CARDPEEK_ERROR_SW;
  }

  if (pcsc->status!=SCARD_S_SUCCESS)
  {
    log_printf(LOG_ERROR,"Failed to transmit command to card: %s (error 0x%08x).",
		    pcsc_stringify_error(pcsc->status),
		    pcsc->status );
    return CARDPEEK_ERROR_SW;
  }

  bytestring_assign_data(result,REC_LEN-2,REC_DAT);

  SW = (REC_DAT[REC_LEN-2]<<8)|REC_DAT[REC_LEN-1];

  return SW;
}

static const bytestring_t* pcsc_last_atr(cardreader_t* cr)
{
  pcsc_data_t* pcsc = cr->extra_data;
  DWORD state;
  DWORD protocol;
  BYTE pbAtr[MAX_ATR_SIZE];
  DWORD atrlen=MAX_ATR_SIZE;
  char readername[MAX_READERNAME];
  DWORD readernamelen=MAX_READERNAME;
  char *tmp;

  pcsc->status = SCardStatus(pcsc->hcard,
	      		     readername,&readernamelen,
		   	     &state,
			     &protocol,
			     pbAtr,&atrlen);

  if (pcsc->status==SCARD_S_SUCCESS)
  {
    bytestring_assign_data(cr->atr,atrlen,pbAtr);
    tmp = bytestring_to_format("%D",cr->atr);
    log_printf(LOG_INFO,"ATR is %i bytes: %s",atrlen,tmp);
    free(tmp);
  }
  else
  {
    bytestring_clear(cr->atr);
    log_printf(LOG_ERROR,"Failed to query card status: %s (error 0x%08x).",
		    pcsc_stringify_error(pcsc->status),
		    pcsc->status );
  }
  return cr->atr;
}

static char **pcsc_get_info(cardreader_t* cr, char** parent)
{
  return parent;
}

static int pcsc_fail(cardreader_t* cr)
{
  pcsc_data_t* pcsc = cr->extra_data;
  return (pcsc->status!=SCARD_S_SUCCESS);
}

static void pcsc_finalize(cardreader_t* cr)
{
  pcsc_data_t* pcsc = cr->extra_data;
  free(pcsc);
}

static int pcsc_initialize(cardreader_t *reader)
{
  pcsc_data_t* pcsc = malloc(sizeof(pcsc_data_t));
  
  memset(pcsc,0,sizeof(pcsc_data_t));

  reader->extra_data   = pcsc;

  reader->connect      = pcsc_connect;
  reader->disconnect   = pcsc_disconnect;
  reader->reset        = pcsc_reset;
  reader->transmit     = pcsc_transmit;
  reader->last_atr     = pcsc_last_atr;
  reader->get_info     = pcsc_get_info;
  reader->fail         = pcsc_fail;
  reader->finalize     = pcsc_finalize;
  return 1;
}

