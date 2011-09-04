--
-- This file is part of Cardpeek, the smartcard reader utility.
--
-- Copyright 2009-2011 by 'L1L1'
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
require('lib.treeflex')

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

BCD_EXTENDED = { "0", "1", "2", "3", 
		 "4", "5", "6", "7", 
		 "8", "9", "*", "#",
		 "-", "?", "!", "F" }

function GSM_bcd_swap(data)
	local i 
	local msb, lsb
	local r = ""
	for i=0,#data-1 do
		lsb = bit.AND(data[i],0xF)
		msb = bit.SHR(data[i],4)
		if lsb == 0xF then break end
		r = r .. BCD_EXTENDED[1+lsb]
		if msb == 0xF then break end 
		r = r .. BCD_EXTENDED[1+msb]
	end
	return r
end

function GSM_tostring(data)
	local r = ""
	local i
	for i=0,#data-1 do
		if data[i]==0xFF then
			return r
		end
		r = r .. string.char(data[i])	
	end	
	return r
end

-------------------------------------------------------------------------
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

function GSM_access_conditions(node,data)
	local text
	text = 	   AC_GSM[1+bit.SHR(data[0],  4)] .. ","
		.. AC_GSM[1+bit.AND(data[0],0xF)] .. ","
		.. AC_GSM[1+bit.SHR(data[1],  4)] .. ","
		-- RFU: .. AC_GSM[1+bit.AND(data[1],0xF)] .. ","
		.. AC_GSM[1+bit.SHR(data[2],  4)] .. ","
		.. AC_GSM[1+bit.AND(data[2],0xF)]
	return node:setAlt(text)
end

function GSM_byte_map(node,data,map)
	return node:setAlt(map[bytes.tonumber(data)])
end


function GSM_ICCID(node,data)
	return node:setAlt(GSM_bcd_swap(data))
end

function GSM_SPN(node,data)
	return node:setAlt(GSM_tostring(bytes.sub(data,1)))
end

function GSM_ADN(node,data)
	local alpha_len = #data-14
	local r = ""
	if data[0]==0xFF then
		return false
	end
	if alpha_len then
		r = GSM_tostring(bytes.sub(data,0,alpha_len-1))
	end
	r = r .. ": " .. GSM_bcd_swap(bytes.sub(data,alpha_len+2,alpha_len+12)) 
	return node:setAlt(r)
end

GSM_MAP = 
{ "application", "3F00", "MF", {
		{ "file", "2FE2", "ICCID", GSM_ICCID },
		{ "application", "7F10", "TELECOM", {
				{ "file", "6F3A", "ADN", GSM_ADN },
				{ "file", "6F3B", "FDN", nil },
				{ "file", "6F3V", "SMS", nil },
				{ "file", "6F3D", "CCP", nil },
				{ "file", "6F40", "MSISDN", nil },
				{ "file", "6F42", "SMSP", nil },
				{ "file", "6F43", "SMSS", nil },
				{ "file", "6F44", "LND", nil },
				{ "file", "6F4A", "EXT1", nil },
				{ "file", "6F4B", "EXT2", nil },
			}
		},
		{ "application", "7F20", "GSM", {
				{ "file", "6F05", "LP", nil },
				{ "file", "6F07", "IMSI", nil },
				{ "file", "6F20", "Kc", nil },
				{ "file", "6F30", "PLMNsel", nil },
				{ "file", "6F31", "HPLMN", nil },
				{ "file", "6F37", "ACMmax", nil },
				{ "file", "6F38", "SST", nil },
				{ "file", "6F39", "ACM", nil },
				{ "file", "6F3E", "GID1", nil },
				{ "file", "6F3F", "GID2", nil },
				{ "file", "6F41", "PUCT", nil },
				{ "file", "6F45", "CBMI", nil },
				{ "file", "6F46", "SPN", GSM_SPN },
				{ "file", "6F74", "BCCH", nil },
				{ "file", "6F78", "ACC", nil },
				{ "file", "6F7B", "FPLMN", nil },
				{ "file", "6F7E", "LOCI", nil },
				{ "file", "6FAD", "AD", nil },
				{ "file", "6FAE", "PHASE", nil },
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
	{ 1, "File structure", GSM_byte_map, MAP1_FILE_STRUCT },
	{ 1, "Length of a record" },
}
function gsm_map_descriptor(node,data,map)
	local pos = 0
	local i,v 
	local item

	node = node:append("record","header",nil,#data)

	for i,v in ipairs(map) do
		item = bytes.sub(data,pos,pos+v[1]-1)
		child = node:append("item",v[2],nil,v[1]):setVal(item)
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
		bytes.append(data,resp)
		pos = pos + try_read
		fsize = fsize - try_read
	end
	
	node = node:append("record","data",nil,#data):setVal(data)
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
		record = node:append("record","record",rec_num,rec_len):setVal(resp)
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
		child = root:append(amap[1],amap[3],amap[2])
		if amap[1]=="file" then
			gsm_map_descriptor(child,resp,EF_MAP)
			file_type = resp[13]
			file_size = resp[2]*256+resp[3]
			if file_type == 0 then
				gsm_read_content_binary(child,file_size,amap[4])
			else
				gsm_read_content_record(child,file_size,resp[14],amap[4])
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
		bytes.append(r,string.byte(pin,i))
	end
	for i=#pin+1,8 do
		bytes.append(r,0xFF)
	end
	return r
end

local PIN
local sw,resp

if card.connect() then
   CARD = card.tree_startup("GSM")

   sw,resp = card.send(bytes.new(8,"A0 A4 00 00 02 7F 20")) -- select DF GSM
   log.print(log.INFO,"Testing if response code starts with 9F")
   if bit.AND(sw,0xFF00) == 0x9F00 then
	PIN = ui.readline("Enter PIN for verification (or keep empty to avoid PIN verification)",8,"0000")
	if PIN~="" then
		PIN= pin_wrap(PIN)
   		card.send(bytes.new(8,"A0 20 00 01 08",PIN)) -- unblock pin = 0000
	end
   	gsm_map(_(CARD),GSM_MAP)
   else
	print(sw,resp)
	ui.question("This does not seem to be a GSM SIM card, halting.",{"OK"})
   end
   card.disconnect()
end

log.print(log.WARNING,"NOTE: This GSM script is still incomplete.")

