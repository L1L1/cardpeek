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

card.CLA=0x94 -- Class for navigo cards

require('lib.apdu')

LFI_LIST = {
  ["0002"] = "Unknown",
  ["2001"] = "Environement",
  ["2010"] = "Events logs",
  ["2020"] = "Contracts",
  ["2040"] = "Special events",
  ["2050"] = "Contract list",
  ["2069"] = "Counters"
}

function en1543_parse(ctx,resp,context)
	ui.tree_append(ctx,true,resp,nil,nil,nil)
end

function process_calypso(cardenv)
	local APP
	local DF_NAME = "1TIC.ICA" 
	local lfi
	local lfi_desc
	local LFI
	local REC


	APP = ui.tree_append(cardenv,false,"application",DF_NAME,nil,nil)
	for lfi,lfi_desc in pairs(LFI_LIST) do
	        sw,resp = card.select_file("2000",8)
		if sw~=0x9000 then 
		        break
		end
		sw,resp = card.select_file(lfi,8)
		if sw==0x9000 then
	                local r
			LFI = ui.tree_append(APP,false,lfi_desc,lfi,nil,nil)
			for r=1,255 do
				sw,resp=card.read_record(0,r,0x1D)
				if sw ~= 0x9000 then
					break
				end
				REC = ui.tree_append(LFI,false,"record",r,card.bytes_size(resp),nil)	
				en1543_parse(REC,resp,lfi_desc)		
			end
		end
	end
end

card.connect()

CARD = card.tree_startup("CALYPSO")

atr = card.last_atr();
hex_card_num = card.bytes_substr(atr,card.bytes_size(atr)-7,4)
hex_card     = card.bytes_unpack(hex_card_num)
card_num     = (hex_card[1]*256*65536)+(hex_card[2]*65536)+(hex_card[3]*256)+hex_card[4]

ui.tree_append(CARD,false,"Card number",card_num,4,"hex: "..hex_card_num)

process_calypso(CARD)

card.disconnect()
