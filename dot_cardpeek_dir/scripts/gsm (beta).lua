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

card.CLA = 0xA0

function card.get_response(len)
	return card.send(bytes.new(8,card.CLA,0xC0,0x00,0x00,len))
end

function card.gsm_select(file_path,return_what,length)
	local sw,resp = card.select(file_path,return_what,length)
	if bit.AND(sw,0xFF00)==0x9F00 then
		log.print(log.INFO,"GSM specific response code 9Fxx")
 		sw,resp = card.get_response(bit.AND(sw,0xFF))
	end
	return sw,resp
end

GSM_DEFAULT_ALPHABET = { 
	"@", "£", "$",  "¥", "è", "é", "ù", "ì", "ò", "Ç", "\\n", "Ø",   "ø", "\\r", "Å", "å",
	"Δ", "_", "Φ",  "Γ", "Λ", "Ω", "Π", "Ψ", "Σ", "Θ", "Ξ",   "\\e", "Æ", "æ",   "ß", "É",
	" ", "!", "\"", "#", "¤", "%", "&", "'", "(", ")", "*",   "+",   ",", "-",   ".", "/",
	"0", "1", "2",  "3", "4", "5", "6", "7", "8", "9", ":",   ";",   "<", "=",   ">", "?",
	"¡", "A", "B",  "C", "D", "E", "F", "G", "H", "I", "J",   "K",   "L", "M",   "N", "O",
	"P", "Q", "R",  "S", "T", "U", "V", "W", "X", "Y", "Z",   "Ä",   "Ö", "Ñ",   "Ü", "§",
	"¿", "a", "b",  "c", "d", "e", "f", "g", "h", "i", "j",   "k",   "l", "m",   "n", "o",
	"p", "q", "r",  "s", "t", "u", "v", "w", "x", "y", "z",   "ä",   "ö", "ñ",   "ü", "à" }
	
BCD_EXTENDED = { "0", "1", "2", "3", 
		 "4", "5", "6", "7", 
		 "8", "9", "*", "#",
		 "-", "?", "!", "F" }

AC_GSM = { "Always", "CHV1", "CHV2", "RFU", 
	   "ADM", "ADM", "ADM", "ADM",
	   "ADM", "ADM", "ADM", "ADM",
	   "ADM", "ADM", "ADM", "ADM",
	   "ADM", "ADM", "ADM", "ADM",
	   "ADM", "ADM", "ADM", "ADM",
	   "ADM", "ADM", "ADM", "ADM",
	   "ADM", "ADM", "ADM", "Never" }

MAP1_FILE_STRUCT 	= { [0]="transparent", [1]="linear fixed", [3]="cyclic" }
MAP1_FILE_TYPE 		= { [1]="MF", [2]="DF", [4]="EF" }
MAP1_FILE_STATUS 	= { [0]="invalidated", [1]="not invalidated" }

function GSM_bcd_swap(data)
	local i,v 
	local r = ""

	for i,v in data:ipairs() do
		local lsb = bit.AND(v,0xF)
		local msb = bit.SHR(v,4)

		if lsb == 0xF then break end
		r = r .. BCD_EXTENDED[1+lsb]
		if msb == 0xF then break end 
		r = r .. BCD_EXTENDED[1+msb]
	end
	return r
end

function GSM_tostring(data)
	local r = ""
	local i,v

	for i,v in data:ipairs() do
		if v==0xFF then
			return r
		end
		r = r .. GSM_DEFAULT_ALPHABET[v+1]	
	end	
	return r
end

-------------------------------------------------------------------------
function GSM_access_conditions(node,data)
	local text
	text = 	   AC_GSM[1+bit.SHR(data:get(0),  4)] .. ","
		.. AC_GSM[1+bit.AND(data:get(0),0xF)] .. ","
		.. AC_GSM[1+bit.SHR(data:get(1),  4)] .. ","
		.. AC_GSM[1+bit.SHR(data:get(2),  4)] .. ","
		.. AC_GSM[1+bit.AND(data:get(2),0xF)]
	return node:set_attribute("alt",text)
end

function GSM_byte_map(node,data,map)
	
	local ret = node:set_attribute("alt",map[bytes.tonumber(data)])
	return ret
end


function GSM_ICCID(node,data)
	return node:set_attribute("alt",GSM_bcd_swap(data))
end

function GSM_SPN(node,data)
	return node:set_attribute("alt",GSM_tostring(bytes.sub(data,1)))
end

function GSM_ADN(node,data)
	local alpha_len = #data-14
	local r = ""
	if data:get(0)==0xFF then
		return node:set_attribute("alt","(empty)")
	end
	if alpha_len then
		r = GSM_tostring(bytes.sub(data,0,alpha_len-1))
	end
	r = r .. ": " .. GSM_bcd_swap(bytes.sub(data,alpha_len+2,alpha_len+12)) 
	return node:set_attribute("alt",r)
end



function GSM_SMS_decode_default_alphabet(node,data)
	local text = ""
	local char
	local back_char = 0
	local pos

	local dmy
	
	for pos=0,#data-1 do
		shifted = (pos%7)
		dmy = bit.AND(data:get(pos),bit.SHR(0xFF,shifted+1))
		
		char = bit.AND(bit.SHL(data:get(pos),shifted),0x7F)+back_char
		back_char = bit.SHR(data:get(pos),7-shifted)
	
		text = text..GSM_DEFAULT_ALPHABET[char+1]
		if shifted==6 then
			text = text..GSM_DEFAULT_ALPHABET[back_char+1]
			back_char = 0
		end
	end
	return node:set_attribute("alt",text)
end

function GSM_number(node,data)
	local bcd=GSM_bcd_swap(data)
	
	if (bcd[#bcd]=='F') then 
		node:set_attribute("alt",string.sub(bcd,1,-1))
	else
		node:set_attribute("alt",bcd)
	end
end

function GSM_byte(node,data)
	node:set_attribute("alt",data:get(0))
end

GSM_MONTHS = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
	       "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" }
GSM_QUARTER = { "00","15","30","45" }

function GSM_timestamp(node,data)
	local bcd=data:convert(4) 
	local tz = bcd:get(13)*10+bcd:get(12)
	local year = bcd:get(1)*10+bcd:get(0)
	
	if year>90 then
		year=1900+year
	else
		year=2000+year
	end

	node:set_attribute("alt",string.format("%i%i:%i%i:%i%i, %i%i %s %i [+%i:%s]",
				bcd:get(7), bcd:get(6), 
				bcd:get(9), bcd:get(8), 
				bcd:get(11), bcd:get(10), 
				bcd:get(5), bcd:get(4),
				GSM_MONTHS[bcd:get(3)*10+bcd:get(2)],
				year,
				math.floor(tz/4), GSM_QUARTER[1+tz%4]));
 
end

function create_sub_node(node,data,label,pos,len,func)
	local subnode
	local edata = bytes.sub(data,pos,pos+len-1)

	subnode = node:append{classname="item",label=label,size=#edata, val=edata}
	if func then
		func(subnode,edata)
	end
end

function GSM_SMS(node,data)
	local subnode
	local pos
	local encoding
	
	create_sub_node(node,data,"status",0,1)
	pos = 1
	if data:get(0)~=0 then
		create_sub_node(node,data,"Length of SMSC information",pos,1,GSM_byte)
		create_sub_node(node,data,"Type of address",pos+1,1)
		create_sub_node(node,data,"Service center number",pos+2,data:get(pos)-1,GSM_number)
		pos = pos+data:get(pos) + 1
		create_sub_node(node,data,"First octet SMS deliver message",pos,1,GSM_byte)
		pos = pos + 1
		create_sub_node(node,data,"Length of address",pos,1,GSM_byte)
		create_sub_node(node,data,"Type of address",pos+1,1)
		create_sub_node(node,data,"Sender number".." "..tostring(data:get(pos)),pos+2,math.floor((data:get(pos)+1)/2),GSM_number)
		pos = pos+math.floor((data:get(pos)+1)/2) + 2
		create_sub_node(node,data,"TP-PID",pos,1)
		encoding = data:get(pos+1)
		create_sub_node(node,data,"TP-DCS",pos+1,1)
		create_sub_node(node,data,"TP-SCTS",pos+2,7,GSM_timestamp)
		pos = pos + 9
		create_sub_node(node,data,"Length of SMS",pos,1,GSM_byte)
		if encoding==0 then
			create_sub_node(node,data,"Text of SMS",pos+1,math.floor(((data:get(pos))*7+6)/8),GSM_SMS_decode_default_alphabet)
		else
			create_sub_node(node,data,"Text of SMS",pos+1,math.floor(((data:get(pos))*7+6)/8))
		end
	end
end


--[[
	GSM_MAP allows to map out the content of a GSM SIM card into a set of nodes diplayed in the cardpeek
	interface.

	Each entry in GSM_MAP reperesents a node and is composed of 4 parts:
		1. a "classname" (the icon used in cardpeek)
		2. an "id" (the id of the node)
		3. a "label" for the node.
		4. an action to undertake, which itself can take 3 types of values:
			(a) a function name representing a function that will be called to futher process the data.
			(b) an array of GSM_MAP entries, mapping out a sub-directory.
			(c) nil, meaning that we do nothing except show raw data.

	The function described in 4(a) takes two parameters: the (tQuery) node in the cardpeek interface, and the 
	data itself, represented as a bytestring. The function can therefore create sub-nodes and/or create 
	interpretation of the data (by calling the alt() function on the node). 

--]]

GSM_MAP = 
{ "folder", "3F00", "MF", {
		{ "file", "2F00", "Application directory", nil },
		{ "file", "2F05", "Preferred languages", nil },
		{ "file", "2F06", "Access rule reference", nil },
		{ "file", "2FE2", "ICCID", GSM_ICCID },
		{ "folder", "7F10", "TELECOM", {
				{ "file", "6F06", "Access rule reference", nil },
				{ "file", "6F3A", "Abbreviated dialling numbers", GSM_ADN },
				{ "file", "6F3B", "Fixed dialing numbers", GSM_ADN },
				{ "file", "6F3C", "Short messages", GSM_SMS },
				{ "file", "6F3D", "Capability configuration parameters", nil },
				{ "file", "6F40", "MSISDN", GSM_ADN },
				{ "file", "6F42", "SMS parameters", nil },
				{ "file", "6F43", "SMS status", nil },
				{ "file", "6F44", "LND", GSM_ADN },
				{ "file", "6F47", "Short message status report", nil },
				{ "file", "6F49", "Service dialing numbers", GSM_ADN },
				{ "file", "6F4A", "Extension 1", nil },
				{ "file", "6F4B", "Extension 2", nil },
				{ "file", "6F4C", "Extenstion 3", nil },
				{ "file", "6F4D", "Barred dialing numbers", nil },
				{ "file", "6F4E", "Extension 5", nil },
				{ "file", "6F4F", "ECCP", nil },
				{ "file", "6F53", "GPRS location", nil },
				{ "file", "6F54", "SetUp menu elements", nil },
				{ "file", "6FE0", "In case of emergency - dialing number", nil },
				{ "file", "6FE1", "In case of emergency - free format", nil },
				{ "file", "6FE5", "Public service identity of the SM-SC", nil },

			}
		},
		{ "folder", "7F20", "GSM", {
				{ "file", "6F05", "Language indication", nil },
				{ "file", "6F07", "IMSI", nil },
				{ "file", "6F20", "Ciphering key Kc", nil },
				{ "file", "6F30", "PLMN selector", nil },
				{ "file", "6F31", "Higher priority PLMN search", nil },
				{ "file", "6F37", "ACM maximum value", nil },
				{ "file", "6F38", "Sim service table", nil },
				{ "file", "6F39", "Accumulated call meter", nil },
				{ "file", "6F3E", "Group identifier 1", nil },
				{ "file", "6F3F", "Groupe identifier 2", nil },
				{ "file", "6F41", "PUCT", nil },
				{ "file", "6F45", "CBMI", nil },
				{ "file", "6F46", "Service provider name", GSM_SPN },
				{ "file", "6F74", "BCCH", nil },
				{ "file", "6F78", "Access control class", nil },
				{ "file", "6F7B", "Forbidden PLMNs", nil },
				{ "file", "6F7E", "Location information", nil },
				{ "file", "6FAD", "Administrative data", nil },
				{ "file", "6FAE", "Phase identification", nil },
			}
		},
	}
}

DF_MAP =
{
  	{ 2, "RFU" },
	{ 2, "Total memory"},
	{ 2, "File ID" },
	{ 1, "Type of file" },
	{ 5, "RFU" },
	{ 1, "Length of extra GSM data" },
	{ 1, "File characteristics" },
	{ 1, "Number of DFs in this DF" },
	{ 1, "Number of EFs in this DF" },
	{ 1, "Number of CHVs" },
	{ 1, "RFU" },
	{ 1, "CHV1 status" },
	{ 1, "UNBLOCK CHV1 status" },
	{ 1, "CHV2 status" },
	{ 1, "UNBLOCK CHV2 status" },
}

EF_MAP =
{
	{ 2, "RFU" },
	{ 2, "File size" },
	{ 2, "File ID" },
	{ 1, "File type", GSM_byte_map, MAP1_FILE_TYPE },
	{ 1, "Command flags" },
	{ 3, "Access conditions", GSM_access_conditions },
	{ 1, "File status", GSM_byte_map, MAP1_FILE_STATUS },
	{ 1, "Length of extra GSM data" },
	{ 1, "File structure" }, --GSM_byte_map, MAP1_FILE_STRUCT },
	{ 1, "Length of a record" },
}
function gsm_map_descriptor(node,data,map)
	local pos = 0
	local i,v 
	local item

	node = node:append{ classname="header", label="answer to select", size=#data }

	
	for i,v in ipairs(map) do
		item = bytes.sub(data,pos,pos+v[1]-1)
		child = node:append{ classname="item", label=v[2], size=v[1], val=item }
		if v[3] then
			v[3](child,item,v[4])
		end	
		pos = pos + v[1]
	end
end

function gsm_read_content_binary(node,fsize,alt)
	local pos = 0
	local try_read
	local sw,resp
	local data = bytes.new(8)

	while fsize>0 do
		if fsize>128 then
			try_read = 128
		else
			try_read = fsize
		end
		sw, resp = card.read_binary('.',pos,try_read)
		if sw~=0x9000 then
			return false
		end
		data = bytes.concat(data,resp)
		pos = pos + try_read
		fsize = fsize - try_read
	end
	
	node = node:append{classname="block", label="data", size=#data, val=data}
	if alt then
		alt(node,data)
	end
	return true
end

function gsm_read_content_record(node,fsize,rec_len,alt)
	local rec_count
	local rec_num
	local sw,resp
	local record

	if rec_len==nil or rec_len==0 then
		return false
	end
	rec_count = fsize/rec_len

	for rec_num=1,rec_count do
		sw, resp = card.read_record('.',rec_num,rec_len)
		if sw~=0x9000 then
			return false
		end
		record = node:append{ classname="record", label="record", id=rec_num, size=rec_len, val=resp }
		if alt then
			alt(record,resp)
		end
	end
	return true
end

function gsm_map(root,amap)
	local i,v
	local sw,resp
	local child
	local file_type
	local file_size
	
	sw, resp = card.gsm_select("#"..amap[2])

	if sw == 0x9000 then
		child = root:append{classname=amap[1], label=amap[3], id=amap[2]}
		if amap[1]=="file" then
			gsm_map_descriptor(child,resp,EF_MAP)
			file_type = resp:get(13)
			file_size = resp:get(2)*256+resp:get(3)
			if file_type == 0 then
				gsm_read_content_binary(child,file_size,amap[4])
			else
				gsm_read_content_record(child,file_size,resp:get(14),amap[4])
			end
		else
			gsm_map_descriptor(child,resp,DF_MAP)
			if amap[4] then
				for i,v in ipairs(amap[4]) do
					gsm_map(child,v)
				end
			end
		end
	end
end

function pin_wrap(pin)
	local i
	local r = bytes.new(8)
	for i=1,#pin do
		r = bytes.concat(r,string.byte(pin,i))
	end
	for i=#pin+1,8 do
		r = bytes.concat(r,0xFF)
	end
	return r
end

local PIN
local sw,resp

if card.connect() then
   CARD = card.tree_startup("GSM")

   PIN = ui.readline("Enter PIN for verification (or keep empty to avoid PIN verification)",8,"0000")
   if PIN~="" then
   	PIN=pin_wrap(PIN)
   	sw, resp = card.send(bytes.new(8,"A0 20 00 01 08",PIN)) -- unblock pin = XXXX
	if sw == 0x9000 then
		gsm_map(CARD,GSM_MAP)
	elseif bit.AND(sw,0xFF00) == 0x9800 then 
		log.print(log.ERROR,"PIN Verification failed")
		ui.question("PIN Verfication failed, halting.",{"OK"})
	else
		ui.question("This does not seem to be a GSM SIM card, halting.",{"OK"})
   	end
   else
	gsm_map(CARD,GSM_MAP)
   end

   card.disconnect()
   log.print(log.WARNING,"NOTE: This GSM script is still incomplete. Several data items are not analyzed and UMTS (3G) card data is not processed.")
end


