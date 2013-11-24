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


require("lib.tlv")

local SSC
local KM_ENC
local KM_MAC
local KS_ENC
local KS_MAC
local TEST=false

function epass_inc_SSC()
	local i=7
	local e
	while i>=0 do
	   SSC:set(i,SSC:get(i)+1)
	   if SSC:get(i)~=0 then break end
	   i=i-1
	end
	--print("SSC: ",#SSC,SSC)
	return true
end

function epass_key_derivation(seed)
	local SHA1
	local D
	local Kenc
	local Kmac
	
	SHA1 = crypto.create_context(0x0030)
	D = bytes.concat(seed, bytes.new(8,"00 00 00 01"))
        Kenc = crypto.digest(SHA1,D)
        D = bytes.concat(seed, bytes.new(8,"00 00 00 02"))
        Kmac = crypto.digest(SHA1,D)
        return bytes.sub(Kenc,0,15),bytes.sub(Kmac,0,15)
end

function epass_create_master_keys(mrz)
	local pass_no = string.sub(mrz,1,10)
	local pass_bd = string.sub(mrz,14,20)
	local pass_ed = string.sub(mrz,22,28)
	local mrz_info = pass_no..pass_bd..pass_ed;
	local hash
	local SHA1

	SHA1 = crypto.create_context(0x0030);
	hash = crypto.digest(SHA1,bytes.new_from_chars(mrz_info))
	KM_ENC, KM_MAC = epass_key_derivation(bytes.sub(hash,0,15))
	return true
end

function epass_create_session_keys()
	local sw, resp 
	local rnd_icc
	local rnd_ifd
	local k_ifd =   bytes.new(8,"0B 79 52 40 CB 70 49 B0 1C 19 B3 3E 32 80 4F 0B")
	local S
	local iv = bytes.new(8,"00 00 00 00 00 00 00 00")
	local TDES_CBC = crypto.create_context(crypto.ALG_DES2_EDE_CBC+crypto.PAD_ZERO,KM_ENC);
	local CBC_MAC = crypto.create_context(crypto.ALG_ISO9797_M3+crypto.PAD_ISO9797_P2,KM_MAC);
	local e_ifd
	local m_ifd

	rnd_ifd = bytes.new(8,"01 23 45 67 89 AB DC FE") 
	sw, rnd_icc = card.send(bytes.new(8,"00 84 00 00 08"))
   
	if (sw~=0x9000) then return false end
	S = rnd_ifd .. rnd_icc .. k_ifd
	e_ifd = crypto.encrypt(TDES_CBC,S,iv)
	m_ifd = crypto.mac(CBC_MAC,e_ifd)
	cmd_data = e_ifd..m_ifd
	   
	sw, resp = card.send(bytes.new(8,"00 82 00 00 28",cmd_data,0x28))
	
	if (sw==0x6300) then
	   	log.print(log.WARNING,"Perhaps you didn't enter the right MRZ data for this passport")
        end
	if (sw~=0x9000) then 
		return false 
	end
	R = crypto.decrypt(TDES_CBC,bytes.sub(resp,0,31),iv)
	k_icc = bytes.sub(R,16,31)
	k_seed = bytes.new(8)
	for i=0,15 do
	    k_seed = bytes.concat(k_seed,bit.XOR(k_ifd:get(i),k_icc:get(i)))
	end
	SSC = bytes.sub(rnd_icc,4,7)..bytes.sub(rnd_ifd,4,7)
	KS_ENC, KS_MAC = epass_key_derivation(k_seed)
	return true
end

function epass_select(EF)
	local cmdheader = bytes.new(8,"0C A4 02 0C")
	local cmdbody   = bytes.new(8,bit.SHR(EF,8),bit.AND(EF,0xFF))
	local TDES_CBC = crypto.create_context(crypto.ALG_DES2_EDE_CBC+crypto.PAD_OPT_80_ZERO,KS_ENC)
	local CBC_MAC  = crypto.create_context(crypto.ALG_ISO9797_M3+crypto.PAD_ISO9797_P2,KS_MAC)
	local iv = bytes.new(8,"00 00 00 00 00 00 00 00")
	local DO87, DO8E
	local command
	local DO99

	M = bytes.concat("01",crypto.encrypt(TDES_CBC,cmdbody,iv))
	DO87 = asn1.join(0x87,M)
	epass_inc_SSC()
	N = bytes.concat(SSC,cmdheader,"80 00 00 00",DO87)
	CC = crypto.mac(CBC_MAC,N)
	DO8E = asn1.join(0x8E,crypto.mac(CBC_MAC,N))
	command=bytes.concat(cmdheader,(#DO87+#DO8E),DO87,DO8E,0)
	sw, resp = card.send(command)
	epass_inc_SSC()
	if sw~=0x9000 then
		return sw, resp
	end 
	tag_99, value_99, rest = asn1.split(resp)
	tag_8E, value_8E, rest = asn1.split(rest)
	if tag_99~=0x99 then
	   log.print(log.WARNING,"Expected tag 99 in card response")
	   return 0x6FFF
	end
	if tag_8E~=0x8E then
           log.print(log.WARNING,"Expected tag 8E in card response")
           return 0x6FFF
        end
	sw = 0+bytes.tonumber(value_99)

	N = SSC..asn1.join(tag_99,value_99)

	CC = crypto.mac(CBC_MAC,N)
	if value_8E~=CC then
	   log.print(log.WARNING,"Failed to verify MAC in response")
        end
	return sw, resp
end

function epass_read_binary(start,len)
	local cmdheader = bytes.new(8,"0C B0", bit.SHR(start,8),bit.AND(start,0xFF))
	local CBC_MAC  = crypto.create_context(crypto.ALG_ISO9797_M3+crypto.PAD_ISO9797_P2,KS_MAC)
	local TDES_CBC = crypto.create_context(crypto.ALG_DES2_EDE_CBC+crypto.PAD_OPT_80_ZERO,KS_ENC)
	local DO97,D08E
	local value_87
	local iv = bytes.new(8,"00 00 00 00 00 00 00 00")

	DO97 = bytes.new(8,0x97,01,len)
	epass_inc_SSC()
	DO8E = asn1.join(0x8E,crypto.mac(CBC_MAC,bytes.concat(SSC,cmdheader,"80 00 00 00",DO97)))
	command = bytes.concat(cmdheader,(#DO97+#DO8E),DO97,DO8E,0)
	sw, resp = card.send(command)
	epass_inc_SSC()
	if (sw~=0x9000) and (sw~=0x6282) then return false end
	tag_87, value_87 = asn1.split(resp)
	decrypted_data = crypto.decrypt(TDES_CBC,bytes.sub(value_87,1,-1),iv)
	return sw, bytes.sub(decrypted_data,0,len-1)
end

function epass_read_file()
	sw,resp = epass_read_binary(0,4)
	if sw~=0x9000 then return nil end
	log.print(log.INFO,"Heading is " .. tostring(resp))
	tag, rest = asn1.split_tag(resp)
	len, rest = asn1.split_length(rest)

	--io.write("read_file: [")
	
	if len>127 then
           to_read = len + 2 + resp:get(1)-128
	else
           to_read = len + 2 
	end

	log.print(log.INFO,"Expecting " .. to_read)
	result = bytes.new(8)
	already_read = 0
	while to_read>0 do
	     --io.write(".");io.flush()    
	     if to_read>0x80 then
	        le = 0x80
             else
	        le = to_read
	     end
	     sw, resp = epass_read_binary(already_read,le)
	     if sw~=0x9000 then return result end
	     result = bytes.concat(result,resp)
	     already_read = already_read + #resp
	     to_read = to_read - #resp
	end
	--io.write("] "..#result.."bytes\n")
	return result
end

function ui_parse_version(node,data)
	local ver = bytes.format(bytes.sub(data,0,1),"%P")
	local i
	for i=1,(#data/2)-1 do
		ver = ver .. "." .. bytes.format(bytes.sub(data,i*2,i*2+1),"%P")
	end
	nodes.set_attribute(node,"val",data)
	nodes.set_attribute(node,"alt",ver)
	return true
end



function ui_parse_cstring(node,data)
	nodes.set_attribute(node,"val",data)
	if #data>1 then
        	nodes.set_attribute(node,"alt",bytes.format(bytes.sub(data,0,#data-2),"%P"))
	end
end

BL_FAC_RECORD_HEADER = { 
	{ "Format Identifier", 4, ui_parse_cstring },
	{ "Format Version", 4, ui_parse_cstring }, 
	{ "Length of Record", 4, ui_parse_number }, 
	{ "Number of Faces", 2, ui_parse_number }
}

BL_FAC_INFORMATION = {
	{ "Face Image Block Length", 4, ui_parse_number },
	{ "Number of Feature Points", 2, ui_parse_number },
	{ "Gender", 1 , { 
		[0]="Unspecified", 
		[1]="Male", 
		[2]="Female", 
		[3]="Unknown" }},
	{ "Eye Color", 1, {
		[0]="Unspecified", 
                [1]="Black",
                [2]="Blue",
                [3]="Brown",
                [4]="Gray",
                [5]="Green",
                [6]="Multi-Coloured",
                [7]="Pink",
                [8]="Other or Unknown" }},
	{ "Hair Color", 1 , {
		[0]="Unspecified", 
                [1]="Bald",
                [2]="Black",
                [3]="Blonde",
                [4]="Brown",
                [5]="Gray",
                [6]="White",
                [7]="Red",
                [8]="Green",
                [9]="Blue",
                [255]="Other or Unknown" }},
	{ "Feature Mask", 3 },
	{ "Expression", 2, {
		[0]="Unspecified",
                [1]="Neutral",
                [2]="Smile with closed mouth",
                [3]="Smile with open mouth",
                [4]="Raised eyebrows",
                [5]="Eyes looking away",
                [6]="Squinting",
                [7]="Frowning" }},
	{ "Pose Angle", 3 }, 
	{ "Pose Angle Uncertainty", 3 }
}

BL_FAC_FEATURE_POINT = {
	{ "Feature Type", 1 },
	{ "Feature Point", 1 },
	{ "Horizontal Position (x)", 2 },
	{ "Vertical Position (y)", 2 },
	{ "Reserved", 2 }
}

BL_FAC_IMAGE_INFORMATION = { 
	{ "Face Image Type", 1 },
	{ "Image Data Type", 1 },
	{ "Width", 2, ui_parse_number },
	{ "Height", 2, ui_parse_number },
	{ "Image Color Space", 1 },
	{ "Source Type", 1 },
	{ "Device Type", 2 },
	{ "Quality", 2 }
}
	
BL_FIR_GENERAL_RECORD_HEADER = {
	{ "Format Identifier", 4, ui_parse_cstring },
	{ "Version Number", 4, ui_parse_cstring },
	{ "Record Length", 6, ui_parse_number },
	{ "Vendor ID", 2 },
	{ "Scanner ID", 2 },
	{ "Number of fingers/palms", 1, ui_parse_number },
	{ "Scale Units", 1 },
	{ "Horizontal Scan Resolution", 2, ui_parse_number },
	{ "Vertical Scan Resolution", 2, ui_parse_number },
	{ "Horizontal Image Resolution", 2, ui_parse_number },
	{ "Vertical Image Resolution", 2, ui_parse_number },
	{ "Pixel Depth", 1 },
	{ "Image Compression Algorithm", 1, {
		[1]="Uncompressed – bit packed",
		[2]="Compressed – WSQ",
		[3]="Compressed – JPEG",
		[4]="Compressed – JPEG2000",
		[5]="PNG" }},
	{ "Reserved", 2 }
}

BL_FIR_IMAGE_RECORD_HEADER = {
	{ "Length of finger data block", 4, ui_parse_number },
	{ "Finger/palm position", 1, {
		[0]="Unknown",
		[1]="Right thumb",
		[2]="Right index finger",
		[3]="Right middle finger",
		[4]="Right ring finger",
		[5]="Right little finger",
		[6]="Left thumb",
		[7]="Left index finger",
		[8]="Left middle finger",
		[9]="Left ring finger",
		[10]="Left little finger",
		[11]="Plain right thumb",
		[12]="Plain left thumb",
		[13]="Plain right four fingers",
		[14]="Plain left four fingers",
		[15]="Plain thumbs (2)"	}},
	{ "Count of views", 1, ui_parse_number },
	{ "View number", 1, ui_parse_number },
	{ "Finger/palm image quality", 1 },
	{ "Impression type", 1 },
	{ "Horizontal line length", 2, ui_parse_number },
	{ "Vertical line length", 2, ui_parse_number },
	{ "Reserved", 1 }
}


function ui_parse_block(node,format,data,pos)
	local ITEM
	local block
	local i,v
	local index

	for i,v in ipairs(format) do
		ITEM = node:append({ classname="item", label=v[1], id=i-1, size=v[2] })
		block = bytes.sub(data,pos,pos+v[2]-1)
		if v[3]~=nil then
			if type(v[3])=="function" then
			   if v[3](ITEM,block)==false then
			      return nil
			   end
			elseif type(v[3])=="table" then
			   index = bytes.tonumber(block)
			   nodes.set_attribute(ITEM,"val",block)
			   if v[3][index] then
				nodes.set_attribute(ITEM,"alt",v[3][index])
			   else
				nodes.set_attribute(ITEM,"alt",index)
			   end
			else
			   return nil
			end 
		else
			nodes.set_attribute(ITEM,"val",block)
		end
		pos = pos + v[2]
	end
	return pos
end

FAC_FORMAT_IDENTIFIER = bytes.new(8,"46414300")
FIR_FORMAT_IDENTIFIER = bytes.new(8,"46495200")

function ui_parse_biometry_fac(node,data)
	local BLOCK
	local SUBBLOCK
	local SUBSUBBLOCK
	local SUBSUBSUBBLOCK
	local index = 0
	local num_faces
	local num_features
	local block_length
	local block_start
	local image_len
	local i,j

	num_faces = bytes.tonumber(bytes.sub(data,12,13))

	BLOCK = node:append({ classname="item", label="Facial Record Header" })
	index = ui_parse_block(BLOCK,BL_FAC_RECORD_HEADER,data,index)
	if index==nil then
		log.print(log.ERROR,"Failed to parse facial header information") 
		return false 
	end 

	BLOCK = node:append({ classname="item", label="Facial Data Records" })

	for i = 1,num_faces do
		block_start = index
		block_length = bytes.tonumber(bytes.sub(data,index,index+3))
		num_features = bytes.tonumber(bytes.sub(data,index+4,index+5))
		
		SUBBLOCK = BLOCK:append({ classname="item", label="Facial Record Data", id=i, size=block_length })
		
		SUBSUBBLOCK = SUBBLOCK:append({ classname="item", label="Facial Information" })
		index = ui_parse_block(SUBSUBBLOCK,BL_FAC_INFORMATION,data,index)
		if index==nil then 
			log.print(log.ERROR,"Failed to parse facial information")
			return false 
		end 	

		if num_features>0 then
			SUBSUBBLOCK = SUBBLOCK:append({ classname="item", label="Feature points" })
			for j=1,num_features do
				SUBSUBSUBBLOCK =  node:append({ classname="item", label="Feature Point", id=j })
				index = ui_parse_block(SUBSUBSUBBLOCK,BL_FEATURE_POINT,data,index)				
				if index==nil then 
					log.print(log.ERROR,"Failed to parse facial feature information")
					return false 
				end 	
			end
		end

		SUBSUBBLOCK = SUBBLOCK:append({ classname="item", label="Image Information" })
		index = ui_parse_block(SUBSUBBLOCK,BL_FAC_IMAGE_INFORMATION,data,index)
		if index==nil then
			log.print(log.ERROR,"Failed to parse facial image information") 
			return false 
		end
		
		image_len = block_length-(index-block_start)
		SUBSUBBLOCK = SUBBLOCK:append({ classname="item", label="Image Data", size=image_len })
		nodes.set_attribute(SUBSUBBLOCK,"val",bytes.sub(data,index,index+image_len-1))
        
        if image_len>2 and bytes.sub(data,index,index+1) == bytes.new(8,"FF D8") then
		    nodes.set_attribute(SUBSUBBLOCK,"mime-type","image/jpeg")
	    elseif image_len>6 and bytes.sub(data,index,index+5) == bytes.new(8,"00 00 00 0C 6A 50") then
		    nodes.set_attribute(SUBSUBBLOCK,"mime-type","image/jp2")
        else
            log.print(log.WARNING,"Could not identify facial image format.")
        end

        index = index + image_len
	end
	return true

end

function ui_parse_biometry_fir(node,data)
        local BLOCK
        local SUBBLOCK
        local SUBSUBBLOCK
        local index = 0
	local num_images
	local record_size
	local record_start
	local image_size
	local i

	BLOCK = node:append({ classname="item", label="Finger General Record Header" })
	index = ui_parse_block(BLOCK,BL_FIR_GENERAL_RECORD_HEADER,data,index)
	if index==nil then 
		log.print(log.ERROR,"Failed to parse finger general record header information")
		return false 
	end 
	num_images = bytes.tonumber(bytes.sub(data,18,18))

	BLOCK = node:append({ classname="item", label="Finger Images" })
	for i=1,num_images do
		record_size = bytes.tonumber(bytes.sub(data,index,index+3))
		record_start = index

		SUBBLOCK = BLOCK:append({ classname="item", label="Finger Image", id=i, size=record_size })
		SUBSUBBLOCK = SUBBLOCK:append({ classname="item", label="Finger Image Record Header" })
		index = ui_parse_block(SUBSUBBLOCK,BL_FIR_IMAGE_RECORD_HEADER,data,index)
		if index==nil then 
			log.print(log.ERROR,"Failed to parse finger image record header information")
			return false 
		end

		image_size = record_size-(index-record_start)
		SUBSUBBLOCK = SUBBLOCK:append({ classname="item", label="Finger Image Data" })
		nodes.set_attribute(SUBSUBBLOCK,"val",bytes.sub(data,index,index+image_size-1))
		index = index + image_size
	end
 
	return true
end

function ui_parse_biometry(node,data)
	local tag = bytes.sub(data,0,3)

	if tag==FAC_FORMAT_IDENTIFIER then
		ui_parse_biometry_fac(node,data)
	elseif tag==FIR_FORMAT_IDENTIFIER then
		ui_parse_biometry_fir(node,data)
	else
		nodes.set_attribute(node,"val",data)
	end
	return true
end


AID_MRTD = "#A0000002471001"

MRP_REFERENCE = {
   ['5F01']   = { "LDS Version Number", ui_parse_version },
   ['5F08']   = { "Date of birth (truncated)" },
   ['5F1F']   = { "MRZ data elements", ui_parse_printable },
   ['5F2E']   = { "Biometric data block", ui_parse_biometry },
   ['5F36']   = { "Unicode Version number", ui_parse_version },
   ['60']     = { "Common Data Elements" },
   ['61']     = { "Template for MRZ Data Group" },
   ['63']     = { "Template for finger biometric Data Group" },
   ['65']     = { "Template for digitized facial image" },
   ['67']     = { "Template for digitized signature or usual mark" },
   ['68']     = { "Template for machine assisted security -- Encoded data" },
   ['69']     = { "Template for machine assisted security -- Structure" },
   ['6A']     = { "Template for machine assisted security -- Substance" },
   ['6B']     = { "Template for additional personal details" },
   ['6C']     = { "Template for additional document details" },
   ['6D']     = { "Optional details" },
   ['6E']     = { "Reserved for future use" },
   ['70']     = { "Person to notify" },
   ['75']     = { "Template for facial biometric Data Group" },
   ['76']     = { "Template for iris (eye) biometric template" },
   ['77']     = { "Security data (CMD/RFC 3369)" },
   ['7F2E']   = { "Biometric data block (enciphered)" },
   ['7F60']   = { "Biometric Information Template" },
   ['7F61']   = { "Biometric Information Group Template" },
   ['7F61/2'] = { "Number of instances" },
   ['A1']     = { "Biometric Header Template" },
   ['A1/80']  = { "ICAO header version" },
   ['A1/81']  = { "Biometric type" },
   ['A1/82']  = { "Biometric feature" },
   ['A1/83']  = { "Creation date and time" },
   ['A1/84']  = { "Validity period" },
   ['A1/86']  = { "Creator of the biometric reference data (PID)" },
   ['A1/87']  = { "Format owner" },
   ['A1/88']  = { "Format type" }
}



FILES = { 
  { ['FID']=0x011E, ['name']='EF.COM' },
  { ['FID']=0x0101, ['name']='EF.DG1' },
  { ['FID']=0x0102, ['name']='EF.DG2' },
  { ['FID']=0x0103, ['name']='EF.DG3' },
  { ['FID']=0x0104, ['name']='EF.DG4' },
  { ['FID']=0x0105, ['name']='EF.DG5' },
  { ['FID']=0x0106, ['name']='EF.DG6' },
  { ['FID']=0x0107, ['name']='EF.DG7' },
  { ['FID']=0x0108, ['name']='EF.DG8' },
  { ['FID']=0x0109, ['name']='EF.DG9' },
  { ['FID']=0x010A, ['name']='EF.DG10' },
  { ['FID']=0x010B, ['name']='EF.DG11' },
  { ['FID']=0x010C, ['name']='EF.DG12' },
  { ['FID']=0x010D, ['name']='EF.DG13' },
  { ['FID']=0x010E, ['name']='EF.DG14' },
  { ['FID']=0x010F, ['name']='EF.DG15' },
  { ['FID']=0x0110, ['name']='EF.DG16' },
  { ['FID']=0x011D, ['name']='EF.SOD' } 
}

local MRZ_DATA=""

repeat
	MRZ_DATA = ui.readline("Enter the code from the lower MRZ data (printed inside the passport)",44,MRZ_DATA)
	if MRZ_DATA==nil then
		break
	end
	if #MRZ_DATA<28 then
		ui.question("You must enter at least 28 characters",{"OK"})
	end
until #MRZ_DATA>=28

if MRZ_DATA then
  if card.connect() then
    local ke,km,i,v,r
  
    CARD = card.tree_startup("e-passport")
    sw, resp = card.select(AID_MRTD,0)
    APP = CARD:append({classname="application", label="application", id=AID_MRTD})
    if #resp>0 then
  	  tlv_parse(APP,resp)
    end
    epass_create_master_keys(MRZ_DATA)
    if epass_create_session_keys(ke,km)
       then
         for i,v in ipairs(FILES) do
      	   log.print(log.INFO,"Attempt to select " .. v['name'])
           sw, resp = epass_select(v['FID'])
  
           FID = APP:append({ classname="file", label=v['name'], id=string.format(".%04X",v['FID']) })
	   FID:append({classname="header", label="header", size=#resp, val=resp })
	   CONTENT = FID:append({classname="body", label="content" })
	 
	  if sw==0x9000 then
   	      log.print(log.INFO,"Attempt to read from " .. v['name'])

	      r = epass_read_file()
	         
	      if r then
	  	nodes.set_attribute(CONTENT,"size",#r)
                if r:get(0)==0x6D then
	       		tag_6D, value_6D = asn1.split(r)
	       		TAG6D = CONTENT:append({ classname="item", label="National specific data", size=#value_6D })
	       		nodes.set_attribute(TAG6D,"val",value_6D)
	    	else
               		tlv_parse(CONTENT,r,MRP_REFERENCE)
	    	end
	      else
		    nodes.set_attribute(CONTENT,"alt","Selected file, but data is not accessible")
              end
	   
           else
	      nodes.set_attribute(CONTENT,"alt",string.format("Content not accessible (code %04X)",sw))
           end
         end
       else
  	     ui.question("Could not create session keys. Perhaps you didn't enter the correct MRZ data.",{"OK"})
       end
    card.disconnect()
  else
    ui.question("No passport detected in proximity of the reader",{"OK"})
  end
else
  log.print(log.ERROR,"Aborted by the user.")
end

