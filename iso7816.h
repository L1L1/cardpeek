/********************************************************************** 
*
* This file is part of Cardpeek, the smartcard reader utility.
*
* Copyright 2009-2013 by 'L1L1'
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

#ifndef ISO7816_H
#define ISO7816_H

#include "bytestring.h"


typedef struct {
  unsigned apdu_class;
  unsigned lc_len;
  unsigned lc;
  unsigned le_len;
  unsigned le;
} apdu_descriptor_t;

enum {
  APDU_CLASS_NONE,
  APDU_CLASS_1,
  APDU_CLASS_2S,
  APDU_CLASS_3S,
  APDU_CLASS_4S,
  APDU_CLASS_2E,
  APDU_CLASS_3E,
  APDU_CLASS_4E
};

#define ISO7816_OK 1
#define ISO7816_ERROR 0

const char* iso7816_stringify_sw(unsigned short sw);

const char* iso7816_stringify_apdu_class(unsigned apdu_class);

int iso7816_describe_apdu(apdu_descriptor_t *ad, 
			  const bytestring_t* apdu);

int iso7816_make_file_path(bytestring_t* file_path, 
			   int *path_type, 
	   		   const char* path);

/**
   iso7816_make_file_path() converts a string 'path' into a couple 
   { path_file, path_type } according to the following rules :

   path              | path_type |
   ------------------+-----------+-----------------------------------------
   '#'               | 0         | Select the MF (equiv. #3F00)
   '#HHHH'           | 0         | Select file with ID=HHHH
   '#HHHHHH...'      | 4         | Select by AID HHHHHH... (5 to 16 bytes)
   '.HHHH'           | 1         | select child *EF* with file ID=HHHH
   '.HHHH/'          | 2         | select child *DF* with file ID=HHHH
   '..'              | 3         | select parent of current EF or DF
   '/HHHH/HHHH/...'  | 8         | select absolute path /3F00/HHHH/HHHH/...
                     |           |  without specifying the MF (3F00)
   './HHHH/HHHH/...' | 9         | select by relative path ./HHHH/HHHH/...
                     |           |  without specifiying the current DF.

   The values "HH" reperesents hexadecimal pairs of digits (i.e 1 byte).
   
   'path_file' is a bytestring representing 'path' in a binary form that
   is compatible with SELECT_FILE is ISO 7816-4.

   This approach is designed to cover all the cases presented in the 
   specification of SELECT_FILE in ISO 7816-4 : path_type maps to the
   parameter P1 in the SELECT_FILE command.
**/

#endif

