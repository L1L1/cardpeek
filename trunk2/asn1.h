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

#ifndef ASN1_H
#define ASN1_H

#include "bytestring.h"


int asn1_force_single_byte_length_parsing(int enable);

int asn1_skip_tlv(unsigned* pos, const bytestring_t* tlvlist);

int asn1_decode_tag(unsigned* pos, const bytestring_t* tlv, unsigned* tag);

int asn1_decode_length(unsigned* pos, const bytestring_t* tlv, unsigned* len);

int asn1_skip_value(unsigned* pos, const bytestring_t* lv);

int asn1_decode_value(unsigned* pos, const bytestring_t* lv, bytestring_t* val);

int asn1_decode_tlv(unsigned* pos, const bytestring_t* tlv, unsigned *tag, bytestring_t* val);

int asn1_encode_tag(unsigned tag, bytestring_t* bertag);

int asn1_encode_tlv(unsigned tag, const bytestring_t* val, bytestring_t* bertlv);

int asn1_parse_path(const char* path, const bytestring_t* src, bytestring_t* val);

#endif
