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

#include "iso7816.h"
#include <string.h>

int iso7816_describe_apdu(apdu_descriptor_t *ad, const bytestring_t* apdu)
{
  unsigned apdu_len = bytestring_get_size(apdu);
  unsigned char c;

  memset(ad,0,sizeof(*ad));

  if (apdu_len<4)
    return ISO7816_ERROR;

  if (apdu_len==4) /* CASE 1 */
  {
    ad->apdu_class = APDU_CLASS_1;
    return ISO7816_OK;
  }
  
  bytestring_get_element(&c,apdu,4);

  if (apdu_len==5) /* CASE 2S */
  {
    ad->apdu_class = APDU_CLASS_2S;
    ad->le_len = 1;
    ad->le     = c;
    return ISO7816_OK;
  }

  if (c>0) /* apdu_len>5 */
  {
    ad->lc_len= 1;
    ad->lc    = c;

    if ((5+ad->lc)==apdu_len) /* CASE 3S */
    {
      ad->apdu_class = APDU_CLASS_3S;
      return ISO7816_OK;
    }
    bytestring_get_element(&c,apdu,ad->lc+5);
    if ((ad->lc+6)==apdu_len) /* CASE 4S */
    {
      ad->apdu_class = APDU_CLASS_4S;
      ad->le_len = 1;
      ad->le     = c;
      return ISO7816_OK;
    }
  }
  else /* c==0 */
  {
    if (apdu_len<7)
      return ISO7816_ERROR;
    if (apdu_len==7) /* CASE 2E */
    {
      ad->apdu_class = APDU_CLASS_2E;
      ad->le_len=3;
      bytestring_get_element(&c,apdu,5);
      ad->le = c;
      bytestring_get_element(&c,apdu,6);
      ad->le = ((ad->le)<<8)|c;
      return ISO7816_OK;
    }
   
    ad->lc_len=3;
    bytestring_get_element(&c,apdu,5);
    ad->lc = c;
    bytestring_get_element(&c,apdu,6);
    ad->lc = ((ad->lc)<<8)|c;
    if (apdu_len==ad->lc+7) /* case 3E */
    {
      ad->apdu_class = APDU_CLASS_3E;
      return ISO7816_OK;
    }
    if (apdu_len==ad->lc+10) /* case 4E */
    {
      bytestring_get_element(&c,apdu,ad->lc+7);
      if (c!=0)
	return ISO7816_ERROR;
      ad->apdu_class = APDU_CLASS_4E;
      ad->le_len = 3;
      bytestring_get_element(&c,apdu,ad->lc+8);
      ad->le = c;
      bytestring_get_element(&c,apdu,ad->lc+9);
      ad->le = ((ad->le)<<8)|c;
      return ISO7816_OK; 
    }
  }
  return ISO7816_ERROR;
}

#define is_hex(c) (((c)>='0' && (c)<='9') || \
		   ((c)>='A' && (c)<='F') || \
		   ((c)>='a' && (c)<='f'))

int is_hex4(const char* s)
{
  if (strlen(s)<4)
    return 0;
  return (is_hex(s[0]) && is_hex(s[1]) && is_hex(s[2]) && is_hex(s[3]));
}

int iso7816_make_file_path(bytestring_t* file_path, 
			   int *path_type, 
	   		   const char* path)
{ 
  unsigned path_len = strlen(path);

  *path_type=-1;
  bytestring_clear(file_path);

  if (path[0]=='.')
  {
    if (path[1]=='.' && path_len==2)
    {
      *path_type=3;
      return ISO7816_OK;
    }
    else if (path[1]=='/')
    {
      *path_type=9; /* 9 => SELECT FROM CURRENT DF */
      bytestring_assign_digit_string(file_path,path+2);
      return ISO7816_OK;
    }
    else if (is_hex4(path+1))
    {
      if (path[5]=='/' && path_len==6) 
      {
	*path_type=1;
	bytestring_assign_digit_string(file_path,path+1);
	return ISO7816_OK;
      }
      else if (path_len==5)
      {
	*path_type=2;
	bytestring_assign_digit_string(file_path,path+1);
        return ISO7816_OK;
      }
    }
  }
  else if (path[0]=='#')
  {
    if (path_len==5)
    {
      *path_type=0;
      bytestring_assign_digit_string(file_path,path+1);
      return ISO7816_OK;
    }
    else if (path_len==1)
    {
      *path_type=0;
      return ISO7816_OK;
    }
    else
    {
      *path_type=4;
      bytestring_assign_digit_string(file_path,path+1);
      return ISO7816_OK;
    }
  }
  else if (path[0]=='/' && is_hex4(path+1)) 
  {
    *path_type=8; /* 8 => SELECT FROM MF */
    bytestring_assign_digit_string(file_path,path+1);
    return ISO7816_OK;
  }
  return ISO7816_ERROR;
}

const char* iso7816_stringify_sw(unsigned short sw)
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
      if (sw==0x6FFF)
	strcpy(msg,"Cardpeek application-level error");
      else
	strcpy(msg,"No precise diagnosis");
      break;
    case 0x92:
      strcpy(msg,"GSM - Update successful after n reties (see SW2)");
      break;
    case 0x94:
      switch (sw&0xFF) {
	case 0x00: strcpy(msg,"GSM - No EF selected"); break;
	case 0x02: strcpy(msg,"GSM - Out of range (invalid address)"); break;
	case 0x04: strcpy(msg,"GSM - File ID or pattern not found"); break;
	case 0x08: strcpy(msg,"GSM - File inconsistent with the command"); break;
      }
      break;
    case 0x98:
      switch (sw&0xFF) {
	case 0x02: strcpy(msg,"GSM - No CHV initilaized"); break;
	case 0x04: strcpy(msg,"GSM - Authentication failed"); break;
	case 0x08: strcpy(msg,"GSM - In contradiction with CHV status"); break;
	case 0x10: strcpy(msg,"GSM - In contradiction invalidation status"); break;
	case 0x40: strcpy(msg,"GSM - Authentication failed, no attempts left"); break;
	case 0x50: strcpy(msg,"GSM - Increase failed, max. vaule reached"); break;
      }
      break;
     case 0x9F:
      strcpy(msg,"GSM - Length of response in SW2"); 
      break;

    default:
      strcpy(msg,"** Unkown error code **");
  }
  return msg;
}

const char *APDU_CLASS_NAMES[]={"None","1","2S","3S","4S","2E","3E","4E"};

const char *iso7816_stringify_apdu_class(unsigned apdu_class)
{
  if (apdu_class>7)
    apdu_class=0;
  return APDU_CLASS_NAMES[apdu_class];
}
