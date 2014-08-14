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

#ifndef REPLAY_H
#define REPLAY_H

#include "bytestring.h"

enum {
  CARDREPLAY_COMMAND,
  CARDREPLAY_RESET
};

enum {
  CARDREPLAY_ERROR,
  CARDREPLAY_OK
};

typedef struct _anyreplay_t {
  struct _anyreplay_t *next;
  unsigned type;
} anyreplay_t;

typedef struct _comreplay_t {
  anyreplay_t *next;
  unsigned type;
  bytestring_t *query;
  unsigned sw;
  bytestring_t *response;
} comreplay_t;

typedef struct _resreplay_t {
  anyreplay_t *next;
  unsigned type;
  bytestring_t *atr;
} resreplay_t;

typedef union {
  anyreplay_t *any;
  comreplay_t *com;
  resreplay_t *res;
} replay_t;

typedef struct _cardreplay_t {
  int count;
  replay_t start;
  replay_t pos;
  replay_t atr;
} cardreplay_t;

int cardreplay_add_command(cardreplay_t* ce, 
			 const bytestring_t* command, 
			 unsigned sw, const bytestring_t* result);

int cardreplay_add_reset(cardreplay_t* ce, const bytestring_t* atr);

cardreplay_t* cardreplay_new(void);

void cardreplay_free(cardreplay_t* ce);

anyreplay_t* cardreplay_after_atr(cardreplay_t* ce);

int cardreplay_run_command(cardreplay_t* ce, 
			 const bytestring_t* command, 
			 unsigned short *sw, 
			 bytestring_t *response);

int cardreplay_run_cold_reset(cardreplay_t* ce);

int cardreplay_run_warm_reset(cardreplay_t* ce);

int cardreplay_run_last_atr(const cardreplay_t* ce, bytestring_t *atr);

int cardreplay_save_to_file(const cardreplay_t* ce, const char *filename);

cardreplay_t* cardreplay_new_from_file(const char *filename);

int cardreplay_count_records(const cardreplay_t* ce);

#endif
