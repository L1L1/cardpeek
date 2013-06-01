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
-- @name Belgian eID
-- @description Belgian electronic ID card.
-- @targets 0.8

require('lib.apdu')
require('lib.tlv')
require('lib.strict')

function tlv_parse_utf8(node,data)
	node:set_attribute("val",data)
	node:set_attribute("alt",data:format("%C"))
end

EID_IDO = {
   ['0'] = { "File structure version" },
   ['1'] = { "Card number", ui_parse_printable },
   ['2'] = { "Chip number" },
   ['3'] = { "Card validity start date", ui_parse_printable },
   ['4'] = { "Card validity end date", ui_parse_printable },
   ['5'] = { "Card delivery municipality", ui_parse_printable },
   ['6'] = { "National number", ui_parse_printable },
   ['7'] = { "Name", ui_parse_printable },
   ['8'] = { "2 first given names", ui_parse_printable },
   ['9'] = { "First letter of third given name", ui_parse_printable },
   ['A'] = { "Nationality", ui_parse_printable },
   ['B'] = { "Birth location", ui_parse_printable },
   ['C'] = { "Birth date", ui_parse_printable },
   ['D'] = { "Sex", ui_parse_printable },
   ['E'] = { "Noble condition", ui_parse_printable },
   ['F'] = { "Document type", ui_parse_printable },
   ['10'] = { "Special status", ui_parse_printable },
   ['11'] = { "Hash of photo" }
}

ADDRESS_IDO = {
   ['0'] = { "File structure version" },
   ['1'] = { "Street and number", ui_parse_printable },
   ['2'] = { "ZIP code", ui_parse_printable },
   ['3'] = { "municipality", ui_parse_printable }
}

--[[
-- If we follow the specs, this is the type of TLV parsing we need to do.
-- in practice tlv_parse() in lib.tlv seems to work just as well
function simpletlv_parse(node,data,ido)
	local tag 
	local len = 0
	local head
	local tail
	local pos = 1
	local child

	if data==nil or #data==0 then
		return
	end

	tag = string.format("%X",data:get(0))
	
	while data:get(pos)==0xFF do
		len = len + 255
		pos = pos + 1
	end

	len = len + data:get(pos)
	pos = pos + 1

	head = data:sub(pos,pos+len-1)
	tail = data:sub(pos+len)

	if ido[tag] then
		child = node:append({ classname="item", label=ido[tag][1], id=tag, size=#head })
		if ido[tag][2] then
			ido[tag][2](child,head)
		else
			child:set_attribute("val",head)
		end
	else
		node:append({ classname="file", label="(unknown)", id=tag, size=#head, val=head })
	end

	return simpletlv_parse(node,tail,ido)
end
--]]

-- weird parameters for belgian EID card select
local BEID_SELECT = card.SELECT_RETURN_FMD + card.SELECT_RETURN_FCP

function eid_process_photo(node)
	node:set_attribute("mime-type","image/jpeg")
end

function eid_process_tlv(node)
	local data = node:get_attribute("val")
	tlv_parse(node,data)
end

function eid_process_tlv_id(node)
	local data = node:get_attribute("val")
	tlv_parse(node,data,EID_IDO)
end

function eid_process_tlv_address(node)
	local data = node:get_attribute("val")
	tlv_parse(node,data,ADDRESS_IDO)
end



local eid_structure = 
{
	{ "folder",	"MF",	".3F00",	{
		{ "file",	"EF_DIR",	".2F00",	eid_process_tlv },
		{ "folder",	"DF_BELPIC",	".DF00",	{
			{ "file",	"EF_ODF",	".5031",	eid_process_tlv },
			{ "file",	"EF_TokenInfo",	".5032",	eid_process_tlv },	
			{ "file",	"EF_AODF",	".5034",	eid_process_tlv },	
			{ "file",	"EF_PrKDF",	".5035",	eid_process_tlv },	
			{ "file",	"EF_CDF",	".5037",	eid_process_tlv },	
			{ "file",	"EF_Cert#2",	".5038",	eid_process_tlv },	
			{ "file",	"EF_Cert#3",	".5039",	eid_process_tlv },	
			{ "file",	"EF_Cert#4",	".503A",	eid_process_tlv },	
			{ "file",	"EF_Cert#6",	".503B",	eid_process_tlv },	
			{ "file",	"EF_Cert#8",	".503C",	eid_process_tlv },	
			}},
		{ "folder",	"DF_ID",	".DF01",	{
			{ "file",	"EF_ID#RN",	".4031",	eid_process_tlv_id },	
			{ "file",	"EF_SGN#RN",	".4032",	nil },	
			{ "file",	"EF_ID#Address",".4033",	eid_process_tlv_address },	
			{ "file",	"EF_SGN#Address",".4034",	nil },	
			{ "file",	"EF_ID#Photo",	".4035",	eid_process_photo },	
			{ "file",	"EF_PuK#7",	".4038",	nil },	
			{ "file",	"EF_Preferences",".4039",	nil },	
			}}
		}
	}
}

function eid_load_files(parent, struct, path)
	local k,v
	local sw, resp
	local node

	if struct==nil then
		log.print(log.ERROR,"missing parameter #2 in eid_load_files()")
		return
	end
	if path==nil then
		path = {}
	end

	for k,v in ipairs(struct) do
		sw, resp = card.select(v[3],BEID_SELECT)

		if sw == 0x9000 then
			node = parent:append({ classname=v[1], label=v[2], id=v[3] })	
			if type(v[4])=="table" then
				table.insert(path,v[3])
				eid_load_files(node,v[4],path)
				table.remove(path)
				
				local k2,v2 
				-- this is done to move up to parent dir
				-- We start again from the MF and go down
				for k2,v2 in ipairs(path) do
					card.select(v2,BEID_SELECT)
				end
			else
				local pos = 0
				local data = bytes.new(8)
	
				repeat 
					sw, resp = card.read_binary('.',pos)
					if resp then
						pos = pos + #resp
						data = data .. resp
					end
				until sw~=0x9000 or resp==nil or #resp<256

				if #data then
					node:set_attribute("val",data)
					node:set_attribute("size",#data)

					if type(v[4])=="function" then
						v[4](node)
					end
				else
					node:set_attribute("alt",string.format("No content (code %04x)",sw))
				end
			end
		end	
	end
end

if card.connect() then
   local CARD
   
   CARD = card.tree_startup("Belgian eID")

   eid_load_files(CARD,eid_structure)

   card.disconnect()
end
