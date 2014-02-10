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
-- @name Calypso
-- @description Calypso tranport cards: Navigo, MOBIB, Korrigo, RavKav, ...
-- @targets 0.8 
--
-------------------------------------------------------------------------
-- *PLEASE NOTE*
-- This work is based on:
-- * public information about the calypso card specification, 
-- * partial information found on the web about the ticketing data 
--   format, as described in the French "intercode" documentation.
-- * experimentation and guesses, 
-- This information is incomplete. If you have further data, such 
-- as details ofd specs, please help send them
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
	local i,v

	if #bs==0 then return false end
	for i,v in bs:ipairs() do
		if v~=byte then return false end
	end
	return true
end

--[[
LFI_LIST = {
  { "ICC",              "/0002",      "file" },
  { "ID",               "/0003",      "file" },
  { "Ticketing",        "/2000",      "folder" },
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
  { "Special events",   "/2000/2040", "file" },
  { "Contract list",    "/2000/2050", "file" },
  { "Counters",         "/2000/2069", "file" },
  { "Holder Extended",  "/3F1C",      "file" }
}
--]]
LFI_LIST = {

  { "AID",              "/3F04",      "file" },
  { "ICC",              "/0002",      "file" },
  { "ID",               "/0003",      "file" },
  { "Holder Extended",  "/3F1C",      "file" },
  { "Display / Free",   "/2F10",      "file" },
  
  { "Ticketing",        "/2000",    "folder" },
  { "AID",              "/2000/2004", "file" },
  { "Environment",      "/2000/2001", "file" },
  { "Holder",           "/2000/2002", "file" }, 
  { "Event logs",       "/2000/2010", "file" },
  { "Contracts",        "/2000/2020", "file" },
  { "Contracts",        "/2000/2030", "file" },
  { "Counters",         "/2000/202A", "file" },
  { "Counters",         "/2000/202B", "file" },
  { "Counters",         "/2000/202C", "file" },
  { "Counters",         "/2000/202D", "file" },
  { "Counters",         "/2000/202E", "file" },
  { "Counters",         "/2000/202F", "file" },
  { "Counters",         "/2000/2060", "file" },
  { "Counters",         "/2000/2061", "file" },
  { "Counters",         "/2000/2062", "file" },
  { "Counters",         "/2000/2069", "file" },
  { "Counters",         "/2000/206A", "file" },
  { "Special events",   "/2000/2040", "file" },
  { "Contract list",    "/2000/2050", "file" },
  { "Free",             "/2000/20F0", "file" },
 
  { "MPP",              "/3100",    "folder" },
    { "AID",            "/3100/3104", "file" },
    { "Public Param.",  "/3100/3102", "file" },
    { "Log",            "/3100/3115", "file" },
    { "Contracts",      "/3100/3120", "file" },
    { "Counters",       "/3100/3113", "file" },
    { "Counters",       "/3100/3123", "file" },
    { "Counters",       "/3100/3133", "file" },
    { "Counters",       "/3100/3169", "file" },
    { "Miscellaneous",  "/3100/3150", "file" },
    { "Free",           "/3100/31F0", "file" },
        
  { "RT2",              "/2100",    "folder" },
    { "AID",            "/2100/2104", "file" },
    { "Environment",    "/2100/2101", "file" },
    { "Event logs",     "/2100/2110", "file" },
    { "Contract list",  "/2100/2150", "file" },
    { "Contracts",      "/2100/2120", "file" },
    { "Counters",       "/2100/2169", "file" },
    { "Special events", "/2100/2140", "file" },
    { "Free",           "/2100/21F0", "file" },
      
  { "EP",               "/1000",    "folder" },
   { "AID",             "/1000/1004", "file" },
   { "Load Log",        "/1000/1014", "file" },
   { "Purchase Log",    "/1000/1015", "file" },
   
  { "eTicket",          "/8000",    "folder" },
   { "AID",             "/8000/8004", "file" },
   { "Preselection",    "/8000/8030", "file" },
   { "Event logs",      "/8000/8010", "file" }
}

function calypso_select(ctx,desc,path,klass)
	local path_parsed = card.make_file_path(path)
	local lfi = bytes.sub(path_parsed,-2)
	local resp, sw
	local r,item
	local parent_node=ctx
	local file_node=nil

	if sel_method==SEL_BY_LFI then
		sw,resp = card.select(bytes.format(lfi,"#%D"))
	else
		sw,resp = card.select(path)
	end

	if sw==0x9000 then
		for r=0,(#path_parsed/2)-1 do
			item = bytes.format(bytes.sub(path_parsed,r*2,r*2+1),"%D")
			
			file_node = parent_node:find_first({id=item})
			if file_node==nil then
				file_node = parent_node:append{ classname = klass,
                                                label = desc,
                                                id = item }
			end
			parent_node = file_node
		end

        if resp and #resp>0 then
            parent_node:append{ classname="header", label="answer to select", size=#resp, val=resp}
        end

		return file_node
	end
	return nil
end

function calypso_guess_network(cardenv)
	local env_node
	local record_node
	local data

	env_node = cardenv:find_first({label="Environment"})

	if env_node then
		record_node = env_node:find_first({label="record", id="1"})
		if record_node then
			data = record_node:get_attribute("val"):convert(1)
			if #data > 36 then
				local country_bin = data:sub(13,24)
				local network_bin = data:sub(25,36)
				local country_num = tonumber(country_bin:convert(4):format("%D"))
			    local network_num = tonumber(network_bin:convert(4):format("%D"))

				if country_num==250 or country_num==56 or country_num==131 then
					return country_num, network_num
				end
				
				country_bin = data:sub(3,14)
                network_bin = data:sub(15,22)
				country_num = tonumber(country_bin:convert(4):format("%D"))
			    network_num = tonumber(network_bin:convert(4):format("%D"))
				if country_num==376 then
					return 376,network_num
				end

				log.print(log.WARNING,"Unknown Calypso card.")
				
			else
				log.print(log.WARNING,"Could not find enough data in 'Environement/record#1'")
			end
		else
			log.print(log.WARNING,"Could not find data in 'Environement/record#1'")
		end
	else
		log.print(log.WARNING,"Could not find data in 'Environement'")
	end
	return false
end


function calypso_process(cardenv)
	local lfi_index
	local lfi_desc
	local lfi_node
	local rec_node
	local sw, resp
	local country, network
	local filename, file

	for lfi_index,lfi_desc in ipairs(LFI_LIST) do
		lfi_node = calypso_select(cardenv,lfi_desc[1],lfi_desc[2], lfi_desc[3])
		
		if lfi_node and lfi_desc[3]=="file" then
            local record
			for record=1,255 do
				sw,resp=card.read_record(0,record,0x1D)
				if sw ~= 0x9000 then
					break
				end
				rec_node = lfi_node:append{ classname = "record", 
							    label = "record", 
							    size = #resp, 
							    id = record,
							    val = resp }
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
end

if card.connect() then 

  CARD = card.tree_startup("CALYPSO")
 
  sw = card.select("#2000")
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

  if sw~=0x6E00 then
      calypso_process(CARD)
  end

  card.disconnect()
else
  ui.question("Connection to card failed. Are you sure a card is inserted in the reader or in proximity of a contactless reader?\n\nNote: French 'navigo' cards cannot be accessed through a contactless interface.",{"OK"});
end
