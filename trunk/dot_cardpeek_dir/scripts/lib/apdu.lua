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

	mycard = ui.tree_add_node(nil,"card",title)
	ref = ui.tree_add_node(mycard,"ATR","cold")
	ui.tree_set_value(ref,tostring(card.last_atr()))
	return mycard
end

-- card.select_file : standard file select apdu

function card.select_file(name,ftype,mode)
	local command
	local bname = bytes.new(8,name)
	if ftype==nil
	then
	   ftype=4
	end
	if mode==nil
	then
	   mode=0
	end
        local command = bytes.new(8,card.CLA,0xA4,ftype,mode,#bname,bname,0)
        return card.send(command)
end

-- card.read_record : standard read_record apdu, with optionnal length in "extra"

function card.read_record(sfi,r,extra)
	if extra==nil 
	then
	   extra = 0
	end
	local command = bytes.new(8,card.CLA,0xB2,r,bit.OR(bit.SHL(sfi,3),4),extra)
        return card.send(command)
end


