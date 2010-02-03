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

/***********************************************
 * SERIAL STUFF
 */

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>

#define BAUDRATE B9600  /* from termios.h */
#define _POSIX_SOURCE 1 /* POSIX compliant source */

int serial_flush_input(int fd)
{
  return tcflush(fd,TCIFLUSH);
}

int serial_flush_output(int fd)
{
  return tcflush(fd,TCOFLUSH);
}

int serial_open(const char *dev_name, struct termios *oldtio)
{
  int fd;
  struct termios newtio;

  /*
     Open device for reading and writing and not as controlling tty
     because we don't want to get killed if linenoise sends CTRL-C.
   */

  fd = open(dev_name, O_RDWR | O_NOCTTY );
  if (fd <0) 
  {
    log_printf(LOG_ERROR,"Failed to open %s, %s",dev_name,strerror(errno));
    return fd; 
  }

  if (oldtio)
    tcgetattr(fd,oldtio); /* save current serial port settings */

  bzero(&newtio, sizeof(newtio)); /* clear struct for new port settings */

  /*
     BAUDRATE: Set bps rate. You could also use cfsetispeed and cfsetospeed.
     CS8     : 8n1 (8bit,no parity,1 stopbit)
     CLOCAL  : local connection, no modem contol
     CREAD   : enable receiving characters
   */

  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;

  /*
     IGNPAR  : ignore bytes with parity errors
     ICRNL   : map CR to NL (otherwise a CR input on the other computer
               will not terminate input)
               otherwise make device raw (no other input processing)
      FIXME: should be 0 probably
   */

  newtio.c_iflag = IGNPAR | ICRNL;

  /*
     Raw output.
   */

  newtio.c_oflag = 0;

  /*
     ICANON  : enable canonical input
               disable all echo functionality, and don't send signals to calling program
   */

  newtio.c_lflag = ICANON;


 /*
     initialize all control characters
     default values can be found in /usr/include/termios.h, and are given
     in the comments, but we don't need them here
   */
#ifdef DO_UNECESSARY_INITIALIZATION
  newtio.c_cc[VINTR]    = 0;     /* Ctrl-c */
  newtio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
  newtio.c_cc[VERASE]   = 0;     /* del */
  newtio.c_cc[VKILL]    = 0;     /* @ */
  newtio.c_cc[VTIME]    = 0;     /* inter-character timer unused */
  newtio.c_cc[VSWTC]    = 0;     /* '\0' */
  newtio.c_cc[VSTART]   = 0;     /* Ctrl-q */
  newtio.c_cc[VSTOP]    = 0;     /* Ctrl-s */
  newtio.c_cc[VSUSP]    = 0;     /* Ctrl-z */
  newtio.c_cc[VEOL]     = 0;     /* '\0' */
  newtio.c_cc[VREPRINT] = 0;     /* Ctrl-r */
  newtio.c_cc[VDISCARD] = 0;     /* Ctrl-u */
  newtio.c_cc[VWERASE]  = 0;     /* Ctrl-w */
  newtio.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
  newtio.c_cc[VEOL2]    = 0;     /* '\0' */
#endif

  newtio.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */
  newtio.c_cc[VEOF]     = 4;     /* Ctrl-d */
  /*
     now clean the line and activate the settings for the port
   */
  tcflush(fd, TCIFLUSH);
  tcflush(fd, TCOFLUSH);
  tcsetattr(fd,TCSANOW,&newtio);

  return fd;
}

void serial_close(int fd, struct termios *restore)
{
  tcflush(fd, TCIFLUSH);
  tcflush(fd, TCOFLUSH);
  if (restore) 
    tcsetattr(fd,TCSANOW,restore);
  close(fd);
}

int serial_send(int fd, char *s)
{
  int len = strlen(s);
  int wr;

  serial_flush_input(fd);
  serial_flush_output(fd);
  wr = write(fd,s,len);

  if (wr!=len)
    log_printf(LOG_ERROR,"Failed to write to serial line - %s",strerror(errno));

  return wr == len;
}

const char* serial_recv(int fd)
{
  static char buf[1024];
  int rd;
  struct timeval Timeout;
  fd_set readfs;
  int res;

  FD_ZERO(&readfs);
  FD_SET(fd,&readfs);
  Timeout.tv_sec  = 0;
  Timeout.tv_usec = 500000;

  res = select(fd+1,&readfs,NULL,NULL,&Timeout);
  if (res == 1)
  {
    rd = read(fd,buf,1023);
    if (rd<0) {
      log_printf(LOG_ERROR,"Failed to read from serial line - %s",strerror(errno));
      return NULL;
    }
    buf[rd--]=0;
    while (rd>=0 && buf[rd]<' ') buf[rd--]=0;
    return buf;
  }
  return NULL;
}


/*********************************************
 * REAL CARD STUFF
 */

typedef struct {
  bytestring_t *serial;
  bytestring_t *atr;
  unsigned reader_version;
  char *reader_version_string;
  unsigned tag_type;
  const char *tag_type_string;
  int line;
  struct termios old_line_state;
  int status;
} acg_data_t;

enum {
  ACG_READER_TYPE_MULTI_ISO,
  ACG_READER_TYPE_LFX
};

int acg_end_continuous_read_mode(int line)
{
  const char *res;
  int i;

 for (i=1;i<=5;i++) {
   serial_send(line,".");
   res = serial_recv(line);
   if (res!=NULL && res[0]=='?') return 1;
 }
 return 0;
}

const char *acg_reset_device(int line)
{
  const char *res;
  static char name[1024];
  int i;

  if (line<0)
    return NULL;
  acg_end_continuous_read_mode(line);

  for (i=1;i<=3;i++) {         /* apply a reset now that we are out of continuous mode */
    log_printf(LOG_DEBUG,"Attempt %i to reset ACG device",i);
    serial_send(line,"x");
    res = serial_recv(line);
    if (res!=NULL && res[0]!='S' && res[0]>=' ')
    {
      strcpy(name,res);
      log_printf(LOG_DEBUG,"Found ACG device - %s",name);
      return name;
    }
  }
  return NULL;
}

const char *acg_detect(const char *dev_name)
{
  struct termios tio;
  int line=serial_open(dev_name,&tio);
  const char *res;

  if (line<0) 
    return NULL;
  res = acg_reset_device(line);
  serial_close(line,&tio);
  return res;
}

const char* AGS_ISO_TAG_ID[] = {
  "D:ICode UID",
  "E:ICode EPC",
  "I:ICode",
  "J:ISO 14443 A Jewel tag",
  "M:ISO 14443 A",
  "S:ISO 14443 A SR 176",
  "V:ISO 15693",
  "Z:ISO 14443 B",
  NULL
};

const char* AGS_LFX_TAG_ID[] = {
  "U:EM4x02",
  "Z:EM4x05 (ISO FXDB)",
  "T:EM4x50",
  "h:HITAG1 / HITAG S",
  "H:HITAG2",
  "Q:Q5",
  "R:TI-RFID Systems",
  NULL
};


enum {
  TAG_ICODE_UID,
  TAG_ICODE_EPC,
  TAG_ICODE,
  TAG_ISO_14443_A_JEWEL_TAG,
  TAG_ISO_14443_A,
  TAG_ISO_14443_A_SR176,
  TAG_ISO_15693,
  TAG_ISO_14443_B,
  TAG_LFX_EM4X02,
  TAG_LFX_EM4X05,
  TAG_LFX_EM4X50,
  TAG_LFX_HITAG1,
  TAG_LFX_HITAG2,
  TAG_LFX_Q5,
  TAG_LFX_TI_RFID,
  TAG_UNKNOWN
};

#define TAG_LFX_START TAG_LFX_EM4X02

int acg_connect(cardreader_t *cr, unsigned prefered_protocol)
{
  bytestring_t *tmp;
  unsigned char checksum,element;
  int count;
  const char *response;
  acg_data_t *extra = cr->extra_data;
  char dev_name[32];
  char* pos;

  pos=strrchr(cr->name,'/');
  if (pos==NULL) {
    log_printf(LOG_ERROR,"Could not parse device name in %s",cr->name);
    return SMARTCARD_ERROR;
  }
  memcpy(dev_name,cr->name+6,pos-cr->name);
  dev_name[(pos-cr->name)-6]=0;

  extra->line = serial_open(dev_name,&(extra->old_line_state));
  if (extra->line<0)
  {
    log_printf(LOG_ERROR,"Failed to open %s",dev_name);
    return SMARTCARD_ERROR;
  }
  
  response = acg_reset_device(extra->line);
  if (response)
    log_printf(LOG_DEBUG,"ACG software reset successful on %s",response);
  else
  {
    log_printf(LOG_ERROR,"ACG Failed to reset reader %s",cr->name);
    return SMARTCARD_ERROR;
  }
  if (response[0]=='M') 
    extra->reader_version=ACG_READER_TYPE_MULTI_ISO;
  else
    extra->reader_version=ACG_READER_TYPE_LFX;
  extra->reader_version_string = strdup(response);

  log_printf(LOG_DEBUG,"ACG Begin waiting for tag");
  for (count=1;count<360;count++)
  {
    response = serial_recv(extra->line);
    if (response!=NULL && response[0]>0) break;
    if ((count%10)==0) 
      log_printf(LOG_DEBUG,"ACG Waiting for tag (%i seconds)",count/2);
  }
  if (response==NULL)
  {
    log_printf(LOG_ERROR,"ACG No tag found near %s",cr->name);
    return SMARTCARD_ERROR;
  }
  else
    log_printf(LOG_DEBUG,"ACG Found tag <%s>",response);

  acg_end_continuous_read_mode(extra->line);


  if (extra->reader_version==ACG_READER_TYPE_MULTI_ISO)
  {
    serial_send(extra->line,"of0501"); /* enable extended protocol */
    response = serial_recv(extra->line);
    if (response)
      log_printf(LOG_DEBUG,"ACG Setting extended ID");
    else
    {
      log_printf(LOG_ERROR,"ACG Failed to set extended ID for %s",cr->name);
      return SMARTCARD_ERROR;
    }

    serial_send(extra->line,"of0101"); /* enable new serial mode */
    response = serial_recv(extra->line);
    if (response)
      log_printf(LOG_DEBUG,"ACG Setting serial mode");
    else
    {
      log_printf(LOG_ERROR,"ACG Failed to set new serial mode for %s",cr->name);
      return SMARTCARD_ERROR;
    }
  }

  serial_send(extra->line,"s");
  response = serial_recv(extra->line);
  if (!response || response[0]=='N')
  {
    log_printf(LOG_ERROR,"ACG Failed to select the tag on %s",cr->name);
    return 0;
  }
  
  if (extra->reader_version==ACG_READER_TYPE_MULTI_ISO)
  {
    for (count=0;AGS_ISO_TAG_ID[count]!=NULL;count++) 
    {
      if (AGS_ISO_TAG_ID[count][0]==response[0])
      {
	extra->tag_type=count;
	extra->tag_type_string=AGS_ISO_TAG_ID[count]+2;
	log_printf(LOG_DEBUG,"ACG Selected the tag %s with type %s",response,extra->tag_type_string);
	break;
      }
    }
    if (AGS_ISO_TAG_ID[count]==NULL)
    {
      extra->tag_type=TAG_UNKNOWN;
      log_printf(LOG_WARNING,"ACG Failed to detect the tag type on %s",cr->name);
    }
  }
  else
  {
    for (count=0;AGS_LFX_TAG_ID[count]!=NULL;count++) 
    {
      if (AGS_LFX_TAG_ID[count][0]==response[0])
      {
	extra->tag_type=count+TAG_LFX_START;
	extra->tag_type_string=AGS_LFX_TAG_ID[count]+2;
	log_printf(LOG_DEBUG,"ACG Selected the tag %s with type %s",response,extra->tag_type_string);
	break;
      }
    }
    if (AGS_LFX_TAG_ID[count]==NULL)
    {
      extra->tag_type=TAG_UNKNOWN;
      log_printf(LOG_WARNING,"ACG Failed to detect the tag type on %s",cr->name);
    }
  }

  extra->serial = bytestring_new_from_string(8,response+1);
  extra->atr = bytestring_new_from_string(8,"3B 88 80 01");
  
  tmp = bytestring_new(8);
  if (bytestring_get_size(extra->serial)>10)
    bytestring_substr(tmp,4,BYTESTRING_NPOS,extra->serial);
  else /* this is an incorrect ATR */
    bytestring_substr(tmp,1,BYTESTRING_NPOS,extra->serial);
  bytestring_append(extra->atr,tmp);
  bytestring_free(tmp);

  checksum=0;
  for (count=1;count<bytestring_get_size(extra->atr);count++)
  {
    bytestring_get_element(&element,extra->atr,count);
    checksum ^= element;
  }
  bytestring_pushback(extra->atr,checksum);
 
  extra->status = SMARTCARD_OK;
  cr->connected=1;
  return SMARTCARD_OK;
}


int acg_disconnect(cardreader_t *cr)
{
  acg_data_t* extra = cr->extra_data;

  serial_close(extra->line,&(extra->old_line_state));
  bytestring_clear(extra->atr);
  bytestring_clear(extra->serial);
  if (extra->reader_version_string)
    free(extra->reader_version_string);
  log_printf(LOG_DEBUG,"Disconnected reader %s",cr->name);
  cr->connected=0;
  return SMARTCARD_OK;
}

int acg_reset(cardreader_t *cr)
{
  acg_disconnect(cr);
  return acg_connect(cr,cr->protocol);
}

unsigned short acg_transmit_pcsc_emulation(cardreader_t* cr,
					   const bytestring_t* command,
					   bytestring_t* result)
{
  /* FF CA 00 00 : Get Data -> UID */
  /* FF CA 01 00 : Get Data -> Historical bytes */
  /* FF 86 00 00 05 01 MSB LSB KeyType KeyNum -> Login */
  /* FF B0 MSB LSB -> read */ 
  return CARDPEEK_ERROR_SW;
}

unsigned short acg_transmit(cardreader_t* cr,
			     const bytestring_t* command, 
			     bytestring_t* result)
{
  acg_data_t* extra = cr->extra_data;
  unsigned short SW;
  unsigned char SW1,SW2;
  bytestring_t *extended_command;
  char* extended_command_string;
  bytestring_t *extended_result;
  const char* extended_result_string;
  unsigned res_len;
  unsigned char CLA;

  if (extra->status!=SMARTCARD_OK && !cr->connected)
    return CARDPEEK_ERROR_SW;
  bytestring_clear(result);

  bytestring_get_element(&CLA,command,0);
  if (CLA==0xFF)
    return acg_transmit_pcsc_emulation(cr,command,result);
 
  extended_command = bytestring_new_from_string(8,"00 00 1F 02");
  bytestring_append(extended_command,command);
  bytestring_set_element(extended_command,1,
			 bytestring_get_size(extended_command)-3);

  extended_command_string = bytestring_to_alloc_string(extended_command);
  extended_command_string[1]='t'; /* little lazy hack */
  bytestring_free(extended_command);

  log_printf(LOG_DEBUG,"Sending %s to card",extended_command_string+1);
  if (!serial_send(extra->line,extended_command_string+1))
  {
    free(extended_command_string);
    log_printf(LOG_ERROR,"Failed to send command to %s",cr->name);
    extra->status = SMARTCARD_ERROR;
    return CARDPEEK_ERROR_SW;
  }
  free(extended_command_string);
 
  extended_result_string = serial_recv(extra->line);
  
  if (!extended_result_string)
    extended_result_string = serial_recv(extra->line);

  if (!extended_result_string)
  {
    log_printf(LOG_ERROR,"Did not receive an answer from %s",cr->name);
    extra->status = SMARTCARD_ERROR;
    return CARDPEEK_ERROR_SW;
  }
    
  log_printf(LOG_DEBUG,"Receiving %s from card",extended_result_string);
  extended_result = bytestring_new_from_string(8,extended_result_string);
  bytestring_substr(result,4,BYTESTRING_NPOS,extended_result);
  bytestring_free(extended_result);
  res_len = bytestring_get_size(result);
  bytestring_get_element(&SW1,result,res_len-2);
  bytestring_get_element(&SW2,result,res_len-1);
  bytestring_resize(result,res_len-2);
  SW=(SW1<<8)|SW2;
  
  extra->status = SMARTCARD_OK;
  return SW;
}

bytestring_t* acg_last_atr(cardreader_t* cr)
{
  acg_data_t* extra = cr->extra_data;
  if (extra->status!=SMARTCARD_OK)
    return NULL;
  return bytestring_duplicate(extra->atr);
}

char **acg_get_info(cardreader_t* cr, char** parent)
{
  acg_data_t* extra = cr->extra_data;
  char **res;
  unsigned parent_size=0;

  while (parent[parent_size]) parent_size++;
  res = realloc(parent,(parent_size+7)*sizeof(char*));
  res[parent_size]  =strdup("TagSerial");
  res[parent_size+1]=bytestring_to_alloc_string(extra->serial);
  res[parent_size+2]=strdup("ReaderVersion");
  res[parent_size+3]=strdup(extra->reader_version_string);
  res[parent_size+4]=strdup("TagType");
  res[parent_size+5]=strdup(extra->tag_type_string);
  res[parent_size+6]=NULL;
  return parent;
}

int acg_fail(cardreader_t* cr)
{
  acg_data_t* extra = cr->extra_data;
  return (extra->status!=SMARTCARD_OK);
}

void acg_finalize(cardreader_t* cr)
{
  acg_data_t* extra = cr->extra_data;
  bytestring_free(extra->atr);
  bytestring_free(extra->serial);
  free(extra);
}

int acg_initialize(cardreader_t *reader)
{
  acg_data_t* extra = malloc(sizeof(acg_data_t));

  if (extra==NULL)
    return 0;
  
  memset(extra,0,sizeof(acg_data_t));

  extra->atr = bytestring_new(8);
  extra->serial = bytestring_new(8);
  reader->extra_data   = extra;
 
  reader->connect      = acg_connect;
  reader->disconnect   = acg_disconnect;
  reader->reset        = acg_reset;
  reader->transmit     = acg_transmit;
  reader->last_atr     = acg_last_atr;
  reader->get_info     = acg_get_info;
  reader->fail         = acg_fail;
  reader->finalize     = acg_finalize;
  return SMARTCARD_OK;
}

