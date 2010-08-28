--
-- This file is part of Cardpeek, the smartcard reader utility.
--
-- Copyright 2009-2010 by 'L1L1'
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

-------------------------------------------------------------------------
-- *PLEASE NOTE*
-- This work is based on:
-- * public information about the calypso card specification, 
-- * partial information found on the web about the ticketing data 
--   format, as described in the French "intercode" documentation.
-- * experimentation and guesses, 
-- This information is incomplete. If you have further data, such 
-- as details of ISO 1545 or calypso card specs, please help send them
-- to L1L1@gmx.com
--------------------------------------------------------------------------

CARD = 0
card.CLA=0x94 -- Class for navigo cards

SEL_BY_PATH = 1
SEL_BY_LFI  = 2
sel_method  = SEL_BY_LFI

require('lib.strict')
require('lib.country_codes')

function bytes.is_all(bs,byte)
	local i
	if #bs==0 then return false end
	for i=0,#bs-1 do
		if bs[i]~=byte then return false end
	end
	return true
end


LFI_LIST = {
  { "ICC",              "/0002",      "file" },
  { "ID",               "/0003",      "file" },
  { "Ticketing",        "/2000",      "application" },
  { "Environment",      "/2000/2001", "file" },
  { "Holder",           "/2000/2002", "file" }, 
  { "Event logs",       "/2000/2010", "file" },
  { "Contracts",        "/2000/2020", "file" },
  { "Counters",         "/2000/202A", "file" },
  { "Counters",         "/2000/202B", "file" },
  { "Counters",         "/2000/202C", "file" },
  { "Counters",         "/2000/202D", "file" },
  { "Counters",         "/2000/202E", "file" },
  { "Counters",         "/2000/202F", "file" },
  { "Counters",         "/2000/2060", "file" },
  { "Counters",         "/2000/2061", "file" },
  { "Counters",         "/2000/2062", "file" },
--  { "Contracts", "/2000/2030" }, -- this is a copy of 0x2050
  { "Special events",   "/2000/2040", "file" },
  { "Contract list",    "/2000/2050", "file" },
  { "Counters",         "/2000/2069", "file" },
  { "Holder Extended",  "/3F1C",      "file" }
}

function calypso_select(ctx,desc,path,type)
	local path_parsed = card.make_file_path(path)
	local lfi = bytes.sub(path_parsed,-2)
	local resp, sw
	local r,item
	local PARENT_REF=ctx
	local FILE_REF=nil

	if sel_method==SEL_BY_LFI then
		sw,resp = card.select(bytes.format("#%D",lfi))
	else
		sw,resp = card.select(path)
	end

	if sw==0x9000 then
		for r=0,(#path_parsed/2)-1 do
			item = bytes.format("%D",bytes.sub(path_parsed,r*2,r*2+1))
			FILE_REF = ui.tree_find_node(PARENT_REF,nil,item)
			if FILE_REF==nil then
				FILE_REF = ui.tree_add_node(PARENT_REF,
							    desc,
							    item,
							    nil,
							    type)
			end
			PARENT_REF = FILE_REF
		end
		return FILE_REF
	end
	return nil
end

function calypso_guess_network(APP)
	local country_bin
	local network_bin
	local ENV_REF
	local RECORD_REF
	local DATA_REF
	local data

	ENV_REF    = ui.tree_find_node(APP,"Environment")

	if ENV_REF then
		RECORD_REF = ui.tree_find_node(ENV_REF,"record",1)
		DATA_REF   = ui.tree_find_node(RECORD_REF,"raw data")
		if DATA_REF then
			data = bytes.convert(1,ui.tree_get_value(DATA_REF))
			country_bin = bytes.sub(data,13,24)
			network_bin = bytes.sub(data,25,36)
			print(bytes.convert(4,country_bin),bytes.convert(4,network_bin))                        
			return tonumber(bytes.format("%D",bytes.convert(4,country_bin))),
			       tonumber(bytes.format("%D",bytes.convert(4,network_bin)))
		else
			log.print(log.WARNING,"Could not find data in 'Environement/record 1/'")
		end
	else
		log.print(log.WARNING,"Could not find data in 'Environement'")
	end
	return false
end


function calypso_process(cardenv)
	local lfi_index
	local lfi_desc
	local LFI
	local REC
	local sw, resp
	local NODE
	local country, network
	local filename, file

	for lfi_index,lfi_desc in ipairs(LFI_LIST) do
		LFI = calypso_select(cardenv,lfi_desc[1],lfi_desc[2], lfi_desc[3])
		if LFI then
	        	local r
			for r=1,255 do
				sw,resp=card.read_record(0,r,0x1D)
				if sw ~= 0x9000 then
					break
				end
				REC = ui.tree_add_node(LFI,"record",r,#resp,"record")
                        	NODE = ui.tree_add_node(REC,"raw data")
	                	ui.tree_set_value(NODE,resp)
	--[[
				print(lfi_desc[1].." record "..r)
				local i
				local d = bytes.convert(1,resp)
				for i=0,#d,40 do
				    print(string.format("%03i:  ",i)..
					  bytes.format(" %D",bytes.sub(d,i+0,i+9))..
					  bytes.format(" %D",bytes.sub(d,i+10,i+19))..
					  bytes.format(" %D",bytes.sub(d,i+20,i+29))..
					  bytes.format(" %D",bytes.sub(d,i+30,i+39)))
				end
	--]]
			end
		end
	end
	
	country, network = calypso_guess_network(cardenv)
	filename = "calypso/c"..country..".lua"
	file = io.open(filename);
	if file then 
		io.close(file)
		dofile(filename)
	else
		log.print(log.LOG_INFO,"Could not find "..filename)
	end

	filename = "calypso/c"..country.."n"..network..".lua"
	file = io.open(filename);
	if file then 
		io.close(file)
		dofile(filename)
	else
		log.print(log.LOG_INFO,"Could not find "..filename)
	end

	--print(country, network);
	--process_en1545(APP)
end

local atr, hex_card_num, card_num

card.connect()

CARD = card.tree_startup("CALYPSO")

atr = card.last_atr();
hex_card_num = bytes.sub(atr,-7,-4)
card_num     = (hex_card_num[0]*256*65536)+(hex_card_num[1]*65536)+(hex_card_num[2]*256)+hex_card_num[3]

local ref = ui.tree_add_node(CARD,"Card number",nil,4,"block")
ui.tree_set_value(ref,hex_card_num)
ui.tree_set_alt_value(ref,card_num)

sw = card.select("#2010")
if sw==0x9000 then
   sel_method = SEL_BY_LFI
else
   sw = card.select("/2000/2010")
   if sw == 0x9000 then
      sel_method = SEL_BY_PATH
   else
      sel_method = SEL_BY_LFI
      ui.question("This script may not work: this card doesn't seem to react to file selection commands.",{"OK"})
   end
end

calypso_process(CARD)

card.disconnect()
