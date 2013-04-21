--
-- This file is part of Cardpeek, the smartcard reader utility.
--
-- Copyright 2009-2013 by 'L1L1'
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

require('lib.strict')
require('lib.apdu')

MIFARE_STD_KEYS= { [0xA]= "A0A1A2A3A4A5",
                   [0xB]= "B0B1B2B3B4B5",
                   [0xF]= "FFFFFFFFFFFF" }


function mifare_read_uid()
	return card.send(bytes.new(8,"FF CA 00 00 00"))
end

function mifare_load_key(keynum,keyval,options)
	local key = bytes.new(8,keyval)
	if options==nil then
		options=0x20 -- load keys in non-volatile memory
	end
	return card.send(bytes.new(8,"FF 82", options, keynum, #key, key)) 
end

function mifare_authenticate(addr,keynum,keytype)
	return card.send(bytes.new(8,"FF 86 00 00 05 01",bit.SHR(addr,8),bit.AND(addr,0xFF),keytype,keynum))
end

function mifare_read_binary(addr,len)
	return card.send(bytes.new(8,"FF B0",bit.SHR(addr,8),bit.AND(addr,0xFF),len))
end

function MIFARE_text(node,data)
	local i
	local r = ""

	for i=0,#data-1 do
		if data[i]>=32 and data[i]<127 then
			r = r .. string.char(data[i])
		else
			r = r .. string.format("\\%03o",data[i])
		end	
	end
	node:setAlt(r)
end

function MIFARE_trailer_sector(trailer,data)
	local key_a, ac, ac_full, key_b
	key_a   = bytes.sub(data,0,5)
	ac_full = bytes.sub(data,6,9)
	key_b   = bytes.sub(data,10,15)

	--trailer = trailer:append("item","interpreted data")	

	ac = bytes.sub(bytes.convert(1,ac_full),12,23)

	ac_split = tostring(bytes.sub(ac,0,3)) .. ", " ..
		   tostring(bytes.sub(ac,4,7)) .. ", " ..
		   tostring(bytes.sub(ac,8,11))

	trailer:append{ classname="item",
			label="Key A",
			size=#key_a,
			val=key_a }

	trailer:append{ classname="item",
			label="Access bits",
			size=#ac_full,
			val=ac_ful,
			alt=ac_split }

	trailer:append{ classname="item",
			label="Key B or data",
			size=#key_b,
			val=key_b }
end

function MIFARE_first_sector(init,data)
	local UID, cc, manu
	
	--init = init:append("item","interpreted data")	
	
	UID  = bytes.sub(data,0,3)
	cc   = bytes.sub(data,4,4)
	manu = bytes.sub(data,5,15)

	init:append{ classname="item",
		     label="UID",
		     size=#UID,
		     vale=UID }

	init:append{ classname="item",
		     label="Check byte",
		     size=#cc,
		     val=cc }

	init:append{ classname="item",
		     label="Manufacturer data",
		     size=#manu, 
		     vale=manu }
	
end

function MIFARE_scan(root)
	local sector,last_sector
	local block
	local key
	local sw, resp
	local SECTOR	

	sw, resp = mifare_read_uid()
	if sw == 0x9000 then
		root:append{ classname="block", label="UID", val=resp, alt=bytes.tonumber(resp) }
	end

	root:append("block","last sector")

	for sector=0,63 do
		sw, resp = mifare_authenticate(sector*4,0xA,0x60)
		key = MIFARE_STD_KEYS[0xA]
		if sw == 0x6982 then
			sw, resp = mifare_authenticate(sector*4,0xB,0x61)
			key = MIFARE_STD_KEYS[0xB]
			if sw == 0x6982 then
				sw, resp = mifare_authenticate(sector*4,0xF,0x60)
				key = MIFARE_STD_KEYS[0xF]
			end
		end

		if sw ~= 0x9000 and sw ~= 0x6982 then
			break
		end
		
		if sw == 0x9000 then
			last_sector = sector
			SECTOR = root:append{ classname="record", label="sector", id=sector }
			SECTOR:append{ classname="item", label="access key", val=bytes.new(8,key) }
			for block = 0,3 do
				sw, resp = mifare_read_binary(sector*4+block,16)
				if sw == 0x9000 then
					SECTOR	:append{ classname="record", 
						         label="block", 
							 id=block, 
							 size=#resp,
							 val=resp }
					if block == 3 then
						MIFARE_trailer_sector(SECTOR:children():last(),resp)
					elseif block+sector == 0 then
						MIFARE_first_sector(SECTOR:children():last(),resp)
					else
						MIFARE_text(SECTOR:children():last(),resp)
					end
				end
			end

		end
	end
	if last_sector then
		_n("last sector"):val(bytes.new(8,last_sector))
	end
end

if card.connect() then
   CARD = card.tree_startup("Mifare")

   mifare_load_key(0xA,MIFARE_STD_KEYS[0xA])
   mifare_load_key(0xB,MIFARE_STD_KEYS[0xB])
   mifare_load_key(0xF,MIFARE_STD_KEYS[0xF])
   MIFARE_scan(CARD)
   card.disconnect()
end
