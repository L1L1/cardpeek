--
-- This file is part of Cardpeek, the smart card reader utility.
--
-- Copyright 2009-2013 by Alain Pannetrat <L1L1@gmx.com>
--
-- Cardpeek is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- Cardpeek is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with Cardpeek.  If not, see <http://www.gnu.org/licenses/>.
--
--********************************************************************--
--
-- 
-- 
--********************************************************************--

-----------------------------------------------------------
-- calypso_card_num : Reads card number from ATR and
-- writes it in the tree in decimal format.
-----------------------------------------------------------
function calypso_card_num()
	local atr
	local hex_card_num
	local card_num

	atr = card.last_atr();
	hex_card_num = bytes.sub(atr,-7,-4)
	card_num     = (hex_card_num:get(0)*256*65536)+(hex_card_num:get(1)*65536)+(hex_card_num:get(2)*256)+hex_card_num:get(3)

	CARD:find_first({ label = "cold ATR" } )
		:append{ classname = "item", 
				 label = "card number", 
				 size = 4,
				 val = hex_card_num,
				 alt = card_num }
end
