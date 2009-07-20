--
-- This file is part of Cardpeek, the smartcard reader utility.
--
-- Copyright 2009 by 'L1L1'
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

-- HELPER LIBRARY : implements some frequent iso7816 APDUs

-- tree_startup : helper startup function
-- connect the card and start the card information tree

function card.tree_startup(title)
	local mycard
	local ref

	mycard = ui.tree_append(nil,false,"card",title,nil,nil)
	ref = ui.tree_append(mycard,false,"ATR","cold",nil,nil)
	ref = ui.tree_append(ref,true,card.last_atr(),nil,nil,nil)
	return mycard
end

-- card_select : standard file select apdu

function card.select_file(name,ftype)
        local fname = card.bytes_unpack(name)
	if ftype==nil
	then
	   ftype=4
	end
        local command = card.bytes_pack({card.CLA,0xA4,ftype,0,#fname,unpack(fname)})
        return card.send(command)
end

-- card_record : standard read_record apdu, with optionnal length in "extra"

function card.read_record(sfi,r,extra)
        local command;
	if extra==nil then
		extra = 0
	end
	command = card.bytes_pack({card.CLA,0xB2,r,bit_or(bit_shl(sfi,3),4),extra})
        return card.send(command)
end


