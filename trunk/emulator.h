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

#ifndef EMULATOR_H
#define EMULATOR_H

#include "bytestring.h"

enum {
  CARDEMUL_COMMAND,
  CARDEMUL_RESET
};

enum {
  CARDEMUL_ERROR,
  CARDEMUL_OK
};

typedef struct _anyemul_t {
  struct _anyemul_t *next;
  unsigned type;
} anyemul_t;

typedef struct _comemul_t {
  anyemul_t *next;
  unsigned type;
  bytestring_t *query;
  unsigned sw;
  bytestring_t *response;
} comemul_t;

typedef struct _resemul_t {
  anyemul_t *next;
  unsigned type;
  bytestring_t *atr;
} resemul_t;

typedef union {
  anyemul_t *any;
  comemul_t *com;
  resemul_t *res;
} emul_t;

typedef struct _cardemul_t {
  int count;
  emul_t start;
  emul_t pos;
  emul_t atr;
} cardemul_t;

int cardemul_add_command(cardemul_t* ce, 
			 const bytestring_t* command, 
			 unsigned sw, const bytestring_t* result);

int cardemul_add_reset(cardemul_t* ce, const bytestring_t* atr);

cardemul_t* cardemul_new(void);

void cardemul_free(cardemul_t* ce);

anyemul_t* cardemul_after_atr(cardemul_t* ce);

int cardemul_run_command(cardemul_t* ce, 
			 const bytestring_t* command, 
			 unsigned short *sw, 
			 bytestring_t *response);

int cardemul_run_cold_reset(cardemul_t* ce);

int cardemul_run_warm_reset(cardemul_t* ce);

int cardemul_run_last_atr(const cardemul_t* ce, bytestring_t *atr);

int cardemul_save_to_file(const cardemul_t* ce, const char *filename);

cardemul_t* cardemul_new_from_file(const char *filename);

int cardemul_count_records(const cardemul_t* ce);

#endif
