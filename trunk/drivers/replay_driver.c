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
#include "../replay.h"

static int replay_connect(cardreader_t *cr, unsigned prefered_protocol)
{
  cardreplay_t* emul = cr->extra_data;
  UNUSED(prefered_protocol);

  cr->connected = 1;
  return cardreplay_run_cold_reset(emul);
}

static int replay_disconnect(cardreader_t *cr)
{
  cr->connected = 0;
  log_printf(LOG_INFO,"Disconnected reader");
  return 1;
}

static int replay_reset(cardreader_t *cr)
{
  cardreplay_t* emul = cr->extra_data;
  log_printf(LOG_INFO,"Reset reader");
  return cardreplay_run_warm_reset(emul);
}

static unsigned short replay_transmit(cardreader_t* cr,
			     const bytestring_t* command, 
			     bytestring_t* result)
{
  cardreplay_t* emul = cr->extra_data;
  unsigned short SW = CARDPEEK_ERROR_SW;

  if (cr->connected) 
    cardreplay_run_command(emul,command,&SW,result);
  return SW;
}

static const bytestring_t* replay_last_atr(cardreader_t* cr)
{
  cardreplay_t* emul = cr->extra_data;

  if (cr->connected)
    cardreplay_run_last_atr(emul,cr->atr);
  return cr->atr;
}

static char** replay_get_info(cardreader_t* cr)
{
  UNUSED(cr);
  return NULL; /* nothing to add */
}

static int replay_fail(cardreader_t* cr)
{
  return (cr->connected==0);
}

static void replay_finalize(cardreader_t* cr)
{
  cardreplay_t* emul = cr->extra_data;
  free(emul);
}

static int replay_initialize(cardreader_t *reader)
{
  char fname[PATH_MAX];
  cardreplay_t* emul = cardreplay_new_from_file(fname);

  sprintf(fname,"%s/%s",config_get_string(CONFIG_FOLDER_LOGS), reader->name+9);
  
  emul = cardreplay_new_from_file(fname);

  if (emul==NULL)
  {
    log_printf(LOG_ERROR,"Could not load %s",fname);
    return 0;
  }

  reader->extra_data   = emul;

  reader->connect      = replay_connect;
  reader->disconnect   = replay_disconnect;
  reader->reset        = replay_reset;
  reader->transmit     = replay_transmit;
  reader->last_atr     = replay_last_atr;
  reader->get_info     = replay_get_info;
  reader->fail         = replay_fail;
  reader->finalize     = replay_finalize;
  return 1;
}

