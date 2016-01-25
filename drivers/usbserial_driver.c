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


static int usbserial_error(cardreader_t *cr)
{
  UNUSED(cr);

  log_printf(LOG_ERROR,"Operation failed: no connected reader selected");
  return 0;
}

static int usbserial_connect(cardreader_t *cr, unsigned prefered_protocol)
{
  UNUSED(prefered_protocol);

  return usbserial_error(cr);
}

static unsigned short usbserial_transmit(cardreader_t* cr,
			     const bytestring_t* command, 
			     bytestring_t* result)
{
  UNUSED(command);
  UNUSED(result);

  usbserial_error(cr);
  return CARDPEEK_ERROR_SW;
}

static const bytestring_t* usbserial_last_atr(cardreader_t* cr)
{
  return cr->atr;
}

static int usbserial_fail(cardreader_t* cr)
{
  UNUSED(cr);

  return 1;
}

static void usbserial_finalize(cardreader_t* cr)
{
  /* usbserial_error(cr); */
}

static int usbserial_initialize(cardreader_t *reader)
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

