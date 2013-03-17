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
#include <limits.h>
#include "../emulator.h"

static int emul_connect(cardreader_t *cr, unsigned prefered_protocol)
{
  cardemul_t* emul = cr->extra_data;

  cr->connected = 1;
  return cardemul_run_cold_reset(emul);
}

static int emul_disconnect(cardreader_t *cr)
{
  cr->connected = 0;
  log_printf(LOG_INFO,"Disconnected reader");
  return 1;
}

static int emul_reset(cardreader_t *cr)
{
  cardemul_t* emul = cr->extra_data;
  log_printf(LOG_INFO,"Reset reader");
  return cardemul_run_warm_reset(emul);
}

static unsigned short emul_transmit(cardreader_t* cr,
			     const bytestring_t* command, 
			     bytestring_t* result)
{
  cardemul_t* emul = cr->extra_data;
  unsigned short SW = CARDPEEK_ERROR_SW;

  if (cr->connected) 
    cardemul_run_command(emul,command,&SW,result);
  return SW;
}

static const bytestring_t* emul_last_atr(cardreader_t* cr)
{
  cardemul_t* emul = cr->extra_data;

  if (cr->connected)
    cardemul_run_last_atr(emul,cr->atr);
  return cr->atr;
}

static char** emul_get_info(cardreader_t* cr,char** parent)
{
  return parent; /* nothing to add */
}

static int emul_fail(cardreader_t* cr)
{
  return (cr->connected==0);
}

static void emul_finalize(cardreader_t* cr)
{
  cardemul_t* emul = cr->extra_data;
  free(emul);
}

static int emul_initialize(cardreader_t *reader)
{
  char fname[PATH_MAX];
  cardemul_t* emul = cardemul_new_from_file(fname);

  sprintf(fname,"%s/%s",config_get_string(CONFIG_FOLDER_LOGS), reader->name+11);
  
  emul = cardemul_new_from_file(fname);

  if (emul==NULL)
  {
    log_printf(LOG_ERROR,"Could not load %s",fname);
    return 0;
  }

  reader->extra_data   = emul;

  reader->connect      = emul_connect;
  reader->disconnect   = emul_disconnect;
  reader->reset        = emul_reset;
  reader->transmit     = emul_transmit;
  reader->last_atr     = emul_last_atr;
  reader->get_info     = emul_get_info;
  reader->fail         = emul_fail;
  reader->finalize     = emul_finalize;
  return 1;
}

