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

-- HELPER LIBRARY : implements some frequent iso7816 APDUs

-----------------------------------------------------------
-- load_smartcard_list
--
----------------------------------------------------------

TABLE_BASE={}
TABLE_REGEX={}
TABLE_LOADED=false

function card.load_smartcard_list()
	local current=nil

	local f = io.open('etc/smartcard_list.txt','r')

	if f==nil then
		f = io.open('/usr/share/pcsc/smartcard_list.txt','r')
	end

	if f==nil then
		log.print(log.WARNING,"Could not find smartcard_list.txt either in .cardpeek/scripts/etc/ or in /usr/share/pcsc/.")
		return false 
	end

	for line in f:lines() do
	
		cfirst = line:sub(1,1)

		-- quick'n'dirty
		if cfirst=="\t" then
			res = { string.find(line,"\t(.+)") }
			table.insert(current, res[3])
		elseif cfirst=="3" then
			res = line:gsub(" ", "")
			if res:find("%.") then
				TABLE_REGEX[res]={}
				current=TABLE_REGEX[res]
			else	
				TABLE_BASE[res]={}
				current=TABLE_BASE[res]
			end	
		else
			-- nothing
		end
	end

	io.close(f)

	TABLE_LOADED = true

	return true
end

function card.identify_atr(atr)
	local k,v
	local target=tostring(atr)

	if TABLE_LOADED==false then
		card.load_smartcard_list()
	end

	for k,v in pairs(TABLE_BASE) do
		if k==target then
			return v
		end
	end
	for k,v in pairs(TABLE_REGEX) do
		if string.match(target,k) then
			return v
		end
	end
	if TABLE_LOADED==true then
		log.print(log.INFO,"Could not find ATR in smartcard database.")
	end
	return nil
end

-----------------------------------------------------------
-- tree_startup : helper startup function
-- connect the card and start the card information tree
-----------------------------------------------------------


function card.tree_startup(title)
	local mycard
	local ref
	local atr = card.last_atr()
	local candidates = card.identify_atr(atr)
	local atrnode

	mycard = nodes.root():append({ classname="card",
				       label=title });

	atrnode = mycard:append({ classname="atr",
			          label="cold ATR",
			  	  size=#atr,
				  val=atr })

	if candidates then
		local cardtypes = atrnode:append({ classname="item",
						   label="Possibly identified card",
						   val=table.concat(candidates,"\n  ")})
	end

	return mycard
end

-----------------------------------------------------------
-- card.select : standard general select command 
-- (superseeds card.select_file)
--
-----------------------------------------------------------

-- constants for return_what

card.SELECT_RETURN_FIRST      = 0
card.SELECT_RETURN_LAST       = 1
card.SELECT_RETURN_NEXT       = 2
card.SELECT_RETURN_PREVIOUS   = 3
card.SELECT_RETURN_FCI        = 0
card.SELECT_RETURN_FCP        = 4
card.SELECT_RETURN_FMD        = 8

function card.select(file_path,return_what,length)
	local command
	local sel_type, sel_file
	local fileid

	sel_file, sel_type = card.make_file_path(file_path)

	if sel_type==nil then
	   return 0x6F00, nil
	end
	
	if return_what==nil then
	   return_what=card.SELECT_RETURN_FIRST + card.SELECT_RETURN_FCI
	end
	
	if sel_file==nil or #sel_file==0 then
	   command = bytes.new(8,card.CLA,0xA4,sel_type,return_what)
	else
	   fileid = bytes.new(8,sel_file)
	   command = bytes.new(8,card.CLA,0xA4,sel_type,return_what,#fileid,fileid)
	end

	if length~=nil then
	   command = bytes.concat(command,length)
	else
	   command = bytes.concat(command,"00")
	end

	return card.send(command)
end

-----------------------------------------------------------
-- card.read_record : standard read_record apdu, 
-- with optionnal length in "extra"
--
-- IN sfi : a number representing a short file identifier
--          or '.' for the current selected file.
-- IN r : record number to read
-- IN length_expected : number of bytes to read
-- RETURNS : sw, data
-----------------------------------------------------------

function card.read_record(sfi,r,length_expected)
	local command

	if sfi==nil then
	   log.print(log.ERROR,"You must specify a SFI number or '.' in card.read_record()")
	   return 0x6F00, nil
	end

	if sfi=='.' then
	   sfi = 0
	end

	if length_expected==nil then
	   length_expected = 0
	end

	local command = bytes.new(8,card.CLA,0xB2,r,bit.OR(bit.SHL(sfi,3),4),length_expected)
        return card.send(command)
end

-----------------------------------------------------------
-- card.read_binary : standard read_binary apdu
--
-- IN sfi : a number representing a short file identifier
--          or '.' for the current selected file.
-- IN addr : start address of read (or nil for 0)
-- IN length_expected : number of bytes to read (or 0 for 256)
-- RETURNS : sw, data
-----------------------------------------------------------

function card.read_binary(sfi,addr,length_expected)
	local command

	if sfi==nil then
	   log.print(log.ERROR,"You must specify a SFI number or '.' in card.read_binary()")
	   return 0x6F00, nil
	end

	if addr==nil then
	   addr=0
	end

	if length_expected==nil then
	   length_expected=0
	end

	if sfi=='.' then
	   command = bytes.new(8,card.CLA,0xB0,bit.AND(bit.SHR(addr,8),0x7F),bit.AND(addr,0xFF),length_expected)
	else
	   command = bytes.new(8,card.CLA,0xB0,bit.OR(0x80,sfi),addr,length_expected)
	end
	return card.send(command)
end

-----------------------------------------------------------
-- card.read_data : read data command
--
-- RETURNS : sw, data
-----------------------------------------------------------

function card.get_data(id,length_expected)
	local command

	if length_expected==nil then
           length_expected=0
        end
	command = bytes.new(8,card.CLA,0xCA,bit.SHR(id,8),bit.AND(id,0xFF),length_expected)
	return card.send(command)
end

-----------------------------------------------------------
-- card.verify : verify apdu
--
-- IN pin : ASCII string with secret pin
-- IN p1 : predicate 1, 0x00 according to ISO/IEC 7816-4,
--			here as input because it assumes other values
--			in Calypso card's usage.
-- In p2 : Qualifier of the reference data according to
--          ISO/IEC 7816-4  
-- RETURNS : sw, data
-----------------------------------------------------------

function card.verify(pin, p1, p2)
    local command, length

    p1 = p1 or 0x00
    p2 = p2 or 0x00
    pin = bytes.new_from_chars(pin)
    length = #pin
    command = bytes.new(8,card.CLA,0x20,p1,p2,length,pin)
    return card.send(command)
end

