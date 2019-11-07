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

typedef struct
{
  char *dev_path;
  ssize_t usbdev_fd;
  LONG status;

} serial_connection_t;


#define MAX_SERIAL_READ_LENGTH 270


static int usbserial_error(cardreader_t *cr)
{
  serial_connection_t *serial = cr->extra_data;
  close(serial->usbdev_fd);
  cr->connected=0;

  log_printf(LOG_ERROR,"Reader disconnected");
  return 0;
}

static int usbserial_connect(cardreader_t *cr, unsigned prefered_protocol)
{
  UNUSED(prefered_protocol);
  serial_connection_t *serial = cr->extra_data;

  serial->usbdev_fd = open(serial->dev_path, O_RDWR | O_NOCTTY); //O_NDELAY
  if(serial->usbdev_fd==-1) {
    log_printf(LOG_ERROR,"Operation failed: can't open serial port");
    return 0;
  }
  else
    cr->connected=1;


  return 1;
  return usbserial_error(cr);
}

static BYTE read_APDU_byte(int fd, BYTE *buf, int blocking) {
    int n;
    if(blocking)
        fcntl(fd, F_SETFL, 0);
    else
        fcntl(fd, F_SETFL, FNDELAY);
    
    n = read(fd, &buf, 1);
    switch(n) {
       case -1:
           log_printf(LOG_ERROR,"Read byte: %x", buf);
           break;
       case 0:
           log_printf(LOG_INFO,"EOF reached");
           break;
       default:
           log_printf(LOG_INFO,"Read byte: %x", buf);
    }   
    return *buf;
}
static long send_APDU(int fd, const SCARD_IO_REQUEST *t0_t1, const unsigned char *data, DWORD size, BYTE *resp, DWORD* resp_len ) {

    int n;
    n = write(fd, &data, size);
    if (n < 0)
       return CARDPEEK_ERROR_SW;
   
    
    resp[0] = read_APDU_byte(fd, resp, 1);
    resp[1] = read_APDU_byte(fd, resp, 0);
    
    return 0;
}

static unsigned short usbserial_transmit(cardreader_t* cr,
			     const bytestring_t* command, 
			     bytestring_t* result)
{
    UNUSED(command);
    UNUSED(result);

    BYTE REC_DAT[MAX_SERIAL_READ_LENGTH];
    DWORD REC_LEN=MAX_SERIAL_READ_LENGTH;

    serial_connection_t *serial = cr->extra_data;

    //usbserial_error(cr);
 
    unsigned short SW;

/*

T=0 protocol
An asynchronous, character-oriented half-duplex transmission protocol.
T=1 protocol
An asynchronous, block-oriented half-duplex transmission protocol.

*/

//#define SCARD_PCI_T0	(&g_rgSCardT0Pci)
//#define SCARD_PCI_T1	(&g_rgSCardT1Pci)
//extern const SCARD_IO_REQUEST g_rgSCardT0Pci, g_rgSCardT1Pci, g_rgSCardRawPci;

    if (cr->protocol==SCARD_PROTOCOL_T0)
    {
        serial->status = send_APDU(serial->usbdev_fd,SCARD_PCI_T0,
                bytestring_get_data(command),
                bytestring_get_size(command),
                REC_DAT,&REC_LEN);
    }
    else if (cr->protocol==SCARD_PROTOCOL_T1)
    {
        serial->status = send_APDU(serial->usbdev_fd,SCARD_PCI_T1,
                bytestring_get_data(command),
                bytestring_get_size(command),
                REC_DAT,&REC_LEN);
    }
    else
    {
        log_printf(LOG_ERROR,"Unknown smartcard protocol: %i",cr->protocol);
        return CARDPEEK_ERROR_SW;
    }

    if (serial->status!=SCARD_S_SUCCESS)
    {
        log_printf(LOG_ERROR,"Failed to transmit command to card: %s (error 0x%08x).",
                pcsc_stringify_error(serial->status),
                serial->status );
        return CARDPEEK_ERROR_SW;
    }

    if (REC_LEN>=2)
    {
        bytestring_assign_data(result,REC_LEN-2,REC_DAT);
        SW = (REC_DAT[REC_LEN-2]<<8)|REC_DAT[REC_LEN-1];
    }
    else if (REC_LEN==1)
    {
        bytestring_clear(result);
        SW = REC_DAT[0];
    }
    else
    {
        log_printf(LOG_ERROR,"Transmited %i bytes to the card (%s), but recieved a response of length %i, without any status word included.",
                bytestring_get_size(command),
                pcsc_stringify_protocol(cr->protocol),
                REC_LEN);
        return CARDPEEK_ERROR_SW;
    }

    return SW;


}

static const bytestring_t* usbserial_last_atr(cardreader_t* cr)
{
  return cr->atr;
}

static int usbserial_fail(cardreader_t* cr)
{
  UNUSED(cr);

  return 1;
}

static void usbserial_finalize(cardreader_t* cr)
{
  free(((serial_connection_t*)cr->extra_data)->dev_path);
  free(cr->extra_data);

  /* usbserial_error(cr); */
}

static int usbserial_initialize(cardreader_t *reader)
{
  char *path;
  int pre_len=strlen("usbserial://");
  path = calloc(18+strlen(reader->name)+1-pre_len, sizeof(char));
  path = strcat(path, "/dev/serial/by-id/");
  path = strcat(path, reader->name + pre_len);

  serial_connection_t* serial = malloc(sizeof(serial_connection_t));
  memset(serial,0,sizeof(serial_connection_t));
  reader->extra_data = serial;
  
  log_printf(LOG_INFO,"Device path: %s", path);
  
  serial->dev_path = path;

  //system("stty -F /dev/ttyUSB0 115200 cs8 -cstopb -parity -icanon min 1 time 1"); //depends on hardware, by default not needed and needs gui changes


  reader->connect      = usbserial_connect;
  reader->disconnect   = usbserial_error;
  reader->reset        = usbserial_error;
  reader->transmit     = usbserial_transmit;
  reader->last_atr     = usbserial_last_atr;
  reader->fail         = usbserial_fail;
  reader->finalize     = usbserial_finalize;
  return 1;
}

