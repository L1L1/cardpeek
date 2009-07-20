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

#ifndef SMARTCARD_H
#define SMARTCARD_H

#include "bytestring.h"

#define PROTOCOL_T0 1
#define PROTOCOL_T1 2

/********************************************************************
 * CARDMANAGER
 */ 

typedef struct {
  char **readers;
  unsigned readers_count;
} cardmanager_t;

cardmanager_t *cardmanager_new();

void cardmanager_free(cardmanager_t* cm);

unsigned cardmanager_count_readers(cardmanager_t* cm);

const char *cardmanager_reader_name(cardmanager_t* cm, unsigned index);

const char **cardmanager_reader_name_list(cardmanager_t* cm);

/********************************************************************
 * CARDREADER
 */

typedef struct _cardreader_t cardreader_t;

struct _cardreader_t {
  char           *name;
  unsigned       connected;
  unsigned long  protocol;
  unsigned short sw;
  void           *extra_data;

  int (*connect)(cardreader_t*, unsigned);
  int (*disconnect)(cardreader_t*);
  int (*reset)(cardreader_t*);
  unsigned short (*transmit)(cardreader_t*,const bytestring_t*, bytestring_t*);
  bytestring_t* (*last_atr)(cardreader_t*);
  int (*fail)(cardreader_t*);
  void (*finalize)(cardreader_t*);
};


cardreader_t* cardreader_new(const char *card_reader_name);

int cardreader_connect(cardreader_t *reader, unsigned protocol);
 
int cardreader_disconnect(cardreader_t *reader);

int cardreader_warm_reset(cardreader_t *reader);

unsigned short cardreader_transmit(cardreader_t *reader,
				   const bytestring_t* command, 
				   bytestring_t* result);

unsigned short cardreader_get_sw(cardreader_t *reader);

bytestring_t* cardreader_last_atr(cardreader_t *reader);

int cardreader_fail(cardreader_t *reader);

void cardreader_free(cardreader_t *reader);



#endif
