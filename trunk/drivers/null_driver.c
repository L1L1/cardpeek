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


int null_error(cardreader_t *cr)
{
  log_printf(LOG_ERROR,"Operation failed: no connected reader selected");
  return 0;
}

int null_connect(cardreader_t *cr, unsigned prefered_protocol)
{
  return null_error(cr);
}

unsigned short null_transmit(cardreader_t* cr,
			     const bytestring_t* command, 
			     bytestring_t* result)
{
  null_error(cr);
  return 0x6D00;
}

bytestring_t* null_last_atr(cardreader_t* cr)
{
  null_last_atr(cr);
  return bytestring_new();
}

int null_fail(cardreader_t* cr)
{
  return 1;
}

void null_finalize(cardreader_t* cr)
{
  null_error(cr);
}

int null_initialize(cardreader_t *reader)
{
  reader->connect      = null_connect;
  reader->disconnect   = null_error;
  reader->reset        = null_error;
  reader->transmit     = null_transmit;
  reader->last_atr     = null_last_atr;
  reader->fail         = null_fail;
  reader->finalize     = null_finalize;
  return 1;
}

