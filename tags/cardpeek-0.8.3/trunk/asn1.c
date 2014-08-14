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

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "misc.h"
#include "asn1.h" /* includes <bytestring> */

#define ASN1_VERIFY(bs) if (bs->width!=8) { log_printf(LOG_ERROR,"ASN1 operation requires 8 bit-width bytestring"); return BYTESTRING_ERROR; } 

int FORCE_SINGLE_BYTE_LENGTH = 0;

int asn1_force_single_byte_length_parsing(int enable)
{
  int retval = FORCE_SINGLE_BYTE_LENGTH;

  FORCE_SINGLE_BYTE_LENGTH = enable;
  log_printf(LOG_INFO,
	     "Setting ASN1 single byte length parsing to %s",
	     enable?"true":"false");
  return retval;
}

int asn1_skip_tlv(unsigned *pos, const bytestring_t *tlvlist)
{
  unsigned tag;
  if (asn1_decode_tag(pos,tlvlist,&tag)!=BYTESTRING_OK)
    return BYTESTRING_ERROR;
  return asn1_skip_value(pos,tlvlist);
}

int asn1_decode_tag(unsigned *pos, const bytestring_t *tlv, unsigned *tag)
{
  unsigned char element;
  unsigned room = bytestring_get_size(tlv);

  *tag = 0;

  ASN1_VERIFY(tlv);

  if (room<*pos+1)
  {
    log_printf(LOG_WARNING,"Error decoding tag in tlv");
    return BYTESTRING_ERROR;
  }
  room -=*pos;

  bytestring_get_element(&element,tlv,(*pos)++);
  *tag=element;
  room--;

  if ((*tag&0x1F)!=0x1F)
    return BYTESTRING_OK;
 
  do {
    *tag<<=8;
    bytestring_get_element(&element,tlv,(*pos)++);
    *tag|=element;
    room--;
  } while (((*tag&0xFF)>0x80) && room>0);

  if (room==0 && (*tag&0xFF)>0x80)
  {
    log_printf(LOG_WARNING,"Tag length error in tlv");
    return BYTESTRING_ERROR;
  }    
  return BYTESTRING_OK;
}

int asn1_decode_length(unsigned *pos, const bytestring_t *lv, unsigned* len)
{
  unsigned char element;
  unsigned room = bytestring_get_size(lv);
  unsigned i;
  
  *len=0;

  ASN1_VERIFY(lv);

  if (room<*pos+1)
  {
    log_printf(LOG_WARNING,"Missing tlv length");
    return BYTESTRING_ERROR;
  }
  room -=*pos;

  bytestring_get_element(&element,lv,(*pos)++);

  if ((element&0x80)==0x80 && !FORCE_SINGLE_BYTE_LENGTH)
  {
    unsigned len_of_len=element&0x7F;
 
    if (room<=len_of_len || len_of_len>4)
    {
      log_printf(LOG_WARNING,"Length of length error in tlv");
      return BYTESTRING_ERROR;
    }
    for (i=0;i<len_of_len;i++)
    {
      bytestring_get_element(&element,lv,(*pos)++);
      *len=(*len<<8)+element;
    }
  }
  else
  {
    *len=element;
  }
  return BYTESTRING_OK;
}

int asn1_skip_value(unsigned *pos, const bytestring_t *lv)
{
  unsigned len;

  if (asn1_decode_length(pos,lv,&len)!=BYTESTRING_OK)
    return BYTESTRING_ERROR;
  if (*pos+len>bytestring_get_size(lv))
    return BYTESTRING_ERROR;
  *pos+=len;
  return BYTESTRING_OK;
}

int asn1_decode_value(unsigned *pos, const bytestring_t *lv, bytestring_t *val)
{
  unsigned len;

  bytestring_clear(val);
  if (asn1_decode_length(pos,lv,&len)!=BYTESTRING_OK)
    return BYTESTRING_ERROR;

  if (*pos+len>bytestring_get_size(lv))
  {
    log_printf(LOG_ERROR,"Value length error in tlv at position %i: expected %i, have only %i",*pos,*pos+len,bytestring_get_size(lv));
    *pos+=bytestring_get_size(lv);
    return BYTESTRING_ERROR;
  }
/*** 
  {
    log_printf(LOG_WARNING,"Value length error in tlv at position %i: expected %i, have only %i",*pos,*pos+len,bytestring_get_size(lv));
    len = bytestring_get_size(lv)-*pos;
    log_printf(LOG_WARNING,"Attempting to shrink tlv length to %i, with unpredictable results",len);
  }
***/
  bytestring_substr(val,*pos,len,lv);
  *pos+=len;
  return BYTESTRING_OK;
}

int asn1_decode_tlv(unsigned *pos, const bytestring_t *tlv, unsigned *tag, bytestring_t *val)
{
  if (asn1_decode_tag(pos,tlv,tag)!=BYTESTRING_OK)
    return BYTESTRING_ERROR;
  if (asn1_decode_value(pos,tlv,val)!=BYTESTRING_OK)
  {
    log_printf(LOG_WARNING,"... after parsing tag `%X'",*tag);
    return BYTESTRING_ERROR;
  }
  return BYTESTRING_OK;
}

int asn1_encode_tag(unsigned tag, bytestring_t* bertag)
{
  ASN1_VERIFY(bertag);

  bytestring_clear(bertag);

  if (tag>0xFFFFFF)
    bytestring_pushback(bertag,tag>>24);
  if (tag>0xFFFF)
    bytestring_pushback(bertag,(tag>>16)&0xFF);
  if (tag>0xFF)
    bytestring_pushback(bertag,(tag>>8)&0xFF);
  bytestring_pushback(bertag,tag&0xFF);

  return BYTESTRING_OK;
}

int asn1_encode_tlv(unsigned tag, const bytestring_t *val, bytestring_t *bertlv)
{
  unsigned len = bytestring_get_size(val);

  if (asn1_encode_tag(tag,bertlv)!=BYTESTRING_OK)
    return BYTESTRING_ERROR;

  if (len>0xFFFFFF)
  {
    bytestring_pushback(bertlv,0x84);
    bytestring_pushback(bertlv,(len>>24)&0xFF);
    bytestring_pushback(bertlv,(len>>16)&0xFF);
    bytestring_pushback(bertlv,(len>>8)&0xFF);
    bytestring_pushback(bertlv,(len)&0xFF);
  }
  else if (len>0xFFFF)
  {
    bytestring_pushback(bertlv,0x83);
    bytestring_pushback(bertlv,(len>>16)&0xFF);
    bytestring_pushback(bertlv,(len>>8)&0xFF);
    bytestring_pushback(bertlv,(len)&0xFF);
  }
  else if (len>0xFF)
  {
    bytestring_pushback(bertlv,0x82);
    bytestring_pushback(bertlv,(len>>8)&0xFF);
    bytestring_pushback(bertlv,(len)&0xFF);
  }
  else if (len>0x80)
  {
    bytestring_pushback(bertlv,0x81);
    bytestring_pushback(bertlv,(len)&0xFF);
  }
  else
    bytestring_pushback(bertlv,len&0xFF);
  bytestring_append(bertlv,val);
  return BYTESTRING_OK;
}

/************************************************************/

const unsigned ANY_TAG = 0xFFFFFFFF;

static int seek_index(unsigned *pos, unsigned max, const bytestring_t *data, unsigned t_index, unsigned tag)
{
  unsigned cindex = 0;
  unsigned ctag=0;
  
  while (cindex<=t_index && *pos<max)
  {
    if (asn1_decode_tag(pos,data,&ctag)!=BYTESTRING_OK)
      return BYTESTRING_ERROR;

    if (tag==ANY_TAG || tag==ctag)
    {
      if (t_index==cindex)
	return BYTESTRING_OK;
      else
	cindex++;
    }
    if (!asn1_skip_value(pos,data))
      return BYTESTRING_ERROR;
  }
  return BYTESTRING_ERROR;
}

static const char *parse_tag(unsigned *tag, const char *s)
{
  char s_pos[15];
  int  s_pos_i;

  *tag=ANY_TAG;
  if (!isxdigit(*s))
    return NULL;
  s_pos_i=0;
  while (isxdigit(*s) && s_pos_i<14)
  {
    s_pos[s_pos_i++]=*s++;
  }
  s_pos[s_pos_i]=0;
  if (!isxdigit(*s))
  {
    *tag=strtol(s_pos,NULL,16);
    return s;
  }
  return NULL;
}

static const char *parse_index(unsigned *t_index, const char *s)
{
  char s_index[15];
  int  s_index_i;

  *t_index=0;
  if (*s!='[')
    return NULL;
  s++;
  s_index_i=0;
  while (isdigit(*s) && s_index_i<15)
  {
    s_index[s_index_i++]=*s++;
  }
  s_index[s_index_i]=0;
  if (*s==']')
  {
    *t_index=atoi(s_index);
    return s+1;
  }
  return NULL;
}

static int parse_path_internal(const char* path, unsigned *pos, unsigned max, const bytestring_t *src, bytestring_t *val)
{
  unsigned tag=ANY_TAG;
  unsigned t_index=0;
  unsigned length;

  while (*path=='/') path++;
  if (*path==0)
    goto parse_path_fail;

  if (*path=='[')
    path=parse_index(&t_index,path);
  else
  {
    path=parse_tag(&tag,path);
    if (path!=NULL && *path=='[')
      path=parse_index(&t_index,path);
  }
  if (path==NULL) /* parse error */
    goto parse_path_fail;
  
  if (seek_index(pos,max,src,t_index,tag)==BYTESTRING_OK)
  {
    if (*path)
    {
      asn1_decode_length(pos,src,&length);
      return parse_path_internal(path,pos,*pos+length,src,val);
    }
    return asn1_decode_value(pos,src,val);
  }
parse_path_fail:
  bytestring_clear(val);
  return BYTESTRING_ERROR;
}

int asn1_parse_path(const char* path, const bytestring_t *src, bytestring_t *val)
{
  unsigned pos=0;
  return parse_path_internal(path,&pos,bytestring_get_size(src),src,val);
}

/***************************************************************/
#ifdef TEST_ASN1
#include <stdio.h>
int main(int argc, char **argv)
{
  const char *s = "6F 25 84 07 A0 00 00 00 03 10 10 A5 1A 50 0A 56 49 53 41 43 52 45 44 49 54 87 01 01 5F 2D 08 65 6E 7A 68 6A 61 6B 6F";
  bytestring_t* t = bytestring_new_from_hex(s);
  unsigned tag,len;
  bytestring_t* value = bytestring_new();
  unsigned pos=0;
  char *tmp;

  asn1_force_single_byte_length_parsing(1);


  tmp = bytestring_to_hex_string(t);
  fprintf(stderr,"%s\n",tmp);
  free(tmp);
  fprintf(stderr,"retcode=%i\n",asn1_decode_tag(&pos,t,&tag));
  fprintf(stderr,"pos=%i\n",pos);
  fprintf(stderr,"tag=%x\n",tag);

  fprintf(stderr,"retcode=%i\n",asn1_decode_value(&pos,t,value));
  fprintf(stderr,"pos=%i\n",pos);
  tmp = bytestring_to_hex_string(value);
  fprintf(stderr,"val=%s\n",tmp);
  free(tmp);
  
  bytestring_free(t);
  bytestring_free(value);
}
#endif
