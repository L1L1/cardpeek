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

require('lib.apdu')
require('lib.tlv')

MONEO_SFI = {
  ['EF_QLOAD']   = 0x08,
  ['EF_RSEQ']    = 0x0A,
  ['EF_ICP']     = 0x0C,
  ['EF_ID']      = 0x17,
  ['EF_BETRAG']  = 0x18,
  ['EF_BOERSE']  = 0x19,
  ['EF_LSEQ']    = 0x1A,
  ['EF_BSEQ']    = 0x1B,
  ['EF_LLOG']    = 0x1C,
  ['EF_BLOG']    = 0x1D

}

function process_moneo(card_ctx)
	local sw, resp
	local APP
	local AID = "A00000006900"
	local r
	local SFI
	local REC

	sw, resp = card.select_file(AID)

	if (sw ~= 0x9000) then
	   return FALSE
	end
	APP = ui.tree_append(card_ctx,false,"application",AID,nil,nil)
	tlv_parse(APP,resp,DUMMY)
	for name,sfi in pairs(MONEO_SFI) do
	    SFI = ui.tree_append(APP,false,"file",name,nil,string.format("SFI=%02X",sfi))
	    for r=1,255 do
	        sw,resp = card.read_record(sfi,r)
	        if (sw ~= 0x9000) then
                   break
		end
		REC = ui.tree_append(SFI,false,"record",r,card.bytes_size(resp),nil)
		ui.tree_append(REC,true,resp,nil,nil,nil)
	    end
	end
end



card.connect()
CARD = card.tree_startup("MONEO")
process_moneo(CARD)
card.disconnect()
