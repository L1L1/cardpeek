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

--------------------------------------------------------------------------
-- GLOBAL EMV CARD COMMANDS extending general.lib 
--------------------------------------------------------------------------

function card.get_processing_options(pdol)
	local data = card.bytes_unpack('83'..pdol)
	local command = card.bytes_pack({0x80,0xA8,0,0,#data,unpack(data)})
	return card.send(command)
end

function card.get_data(data)
	local command = card.bytes_pack({0x80,0xCA,bit_and(bit_shr(data,8),0xFF),bit_and(data,0xFF),0})
	return card.send(command)
end

-- ------------------------------------------------------------------------
-- EMV 
-- ------------------------------------------------------------------------

EMV_REFERENCE = {
   ['5D'] = "Directory Definition File (DDF) Name", 
   ['70'] = "Application Data File (ADF)", 
   ['80'] = "Response Message Template Format 1", 
   ['82'] = "Application Interchange Profile (AIP)", 
   ['87'] = "Application Priority Indicator", 
   ['88'] = "Short File Identifier (SFI)", 
   ['8C'] = "Card Risk Management Data Object List 1 (CDOL1)", 
   ['8D'] = "Card Risk Management Data Object List 2 (CDOL2)", 
   ['8E'] = "Cardholder Verification Method (CVM) List", 
   ['8F'] = "Certificate Authority Public Key Index (PKI)", 
   ['90'] = "Issuer PK Certificate", 
   ['91'] = "Issuer Authentication Data", 
   ['92'] = "Issuer PK Remainder", 
   ['93'] = "Signed Static Application Data", 
   ['94'] = "Application File Locator (AFL)", 
   ['97'] = "Transaction Certificate Data Object List (TDOL)", 
   ['A5'] = "FCI Proprietary Template", 
   ['9F05'] = "Application Discretionary Data", 
   ['9F07'] = "Application Usage Control", 
   ['9F08'] = "Application Version Number", 
   ['9F0B'] = "Cardholder Name - Extended", 
   ['9F0D'] = "Issuer Action Code - Default", 
   ['9F0E'] = "Issuer Action Code - Denial", 
   ['9F0F'] = "Issuer Action Code - Online", 
   ['9F10'] = "Issuer Application Data", 
   ['9F11'] = "Issuer Code Table Index", 
   ['9F12'] = "Application Preferred Name", 
   ['9F13'] = "Last Online ATC Register", 
   ['9F14'] = "Lower Consecutive Offline Limit (Terminal Check)", 
   ['9F17'] = "PIN Try Counter", 
   ['9F19'] = "Dynamic Data Authentication Data Object List (DDOL)", 
   ['9F1F'] = "Track 1 Discretionary Data", 
   ['9F23'] = "Upper Consecutive Offline Limit (Terminal Check)", 
   ['9F26'] = "Application Cryptogram (AC)", 
   ['9F27'] = "Cryptogram Information Data", 
   ['9F2D'] = "ICC PIN Encipherment Public Key Certificate", 
   ['9F2E'] = "ICC PIN Encipherment Public Key Exponent", 
   ['9F2F'] = "ICC PIN Encipherment Public Key Remainder",
   ['9F32'] = "Issuer PK Exponent", 
   ['9F36'] = "Application Transaction Counter (ATC)", 
   ['9F38'] = "Processing Options Data Object List (PDOL)", 
   ['9F42'] = "Application Currency Code", 
   ['9F44'] = "Application Currency Exponent", 
   ['9F45'] = "Data Authentication Code", 
   ['9F46'] = "ICC Public Key Certificate", 
   ['9F47'] = "ICC Public Key Exponent", 
   ['9F48'] = "ICC Public Key Remainder", 
   ['9F4A'] = "Static Data Authentication Tag List", 
   ['9F4B'] = "Signed Dynamic Application Data", 
   ['9F4C'] = "Integrated Circuit Card (ICC) Dynamic Number",
   ["9F4F"] = "Log Fromat",
   ['9F51'] = "Application Currency Code", 
   ['9F52'] = "Application Default Action (ADA)", 
   ['9F53'] = "Consecutive Transaction Limit (International)", 
   ['9F54'] = "Cumulative Total Transaction Amount Limit", 
   ['9F55'] = "Geographic Indicator", 
   ['9F56'] = "Issuer Authentication Indicator",
   ['9F57'] = "Issuer Country Code", 
   ['9F58'] = "Lower Consecutive Offline Limit (Card Check)",
   ['9F59'] = "Upper Consecutive Offline Limit (Card Check)", 
   ['9F5A'] = "Issuer URL2", 
   ['9F5C'] = "Cumulative Total Transaction Amount Upper Limit", 
   ['9F72'] = "Consecutive Transaction Limit (International - Country)", 
   ['9F73'] = "Currency Conversion Factor", 
   ['9F74'] = "VLP Issuer Authorization Code", 
   ['9F75'] = "Cumulative Total Transaction Amount Limit - Dual Currency", 
   ['9F76'] = "Secondary Application Currency Code", 
   ['9F77'] = "VLP Funds Limit", 
   ['9F78'] = "VLP Single Transaction Limit", 
   ['9F79'] = "VLP Available Funds", 
   ['9F7F'] = "Card Production Life Cycle (CPLC) History File Identifiers", 
   ['BF0C'] = "FCI Issuer Discretionary Data"
}

function emv_parse(cardenv,tlv)
	tlv_parse(cardenv,tlv,EMV_REFERENCE)
end

PSE = "315041592E5359532E4444463031"

AID_LIST = { 
  "A0000000421010",
  "A0000000422010",
  "A0000000031010",
  "A0000000032010",
  "A0000000041010",
  "A0000000042010",
  "A00000006900"
}

EXTRA_DATA = { 0x9F36, 0x9F13, 0x9F17, 0x9F4F }


function emv_process_pse(cardenv)
	local APP
	local ref
	local sfi
	local REC
	local FILE
	local RECORD
	local aid

	sw, resp = card.select_file(PSE)

	-- Could it be a french card ?
	if sw == 0x6E00 then 
	   local ATR
	   card.warm_reset()
	   ATR = ui.tree_append(cardenv,false,"ATR","warm",nil,nil)
	   ui.tree_append(ATR,true,card.last_atr(),nil,nil,nil);
           sw, resp = card.select_file(PSE)
	end
	if sw ~= 0x9000 then
     	   log.print(log.INFO,"No PSE")
	   return false
	end

	-- Construct tree
	APP = ui.tree_append(cardenv,false,"PSE",PSE,nil,nil)
	emv_parse(APP,resp)

	ref = ui.tree_find(APP,nil,"88")
	if (ref) then
	   ref = ui.tree_get(ref..":0")
	   sfi = card.bytes_unpack(ref['node'])
	   FILE = ui.tree_append(APP,false,"file",sfi[1],nil,nil)

	   rec = 1
	   AID_LIST = {}
	   repeat 
	     sw,resp = card.read_record(sfi[1],rec)
	     if sw == 0x9000 then
	        RECORD = ui.tree_append(FILE,false,"record",rec,nil,nil)
	        emv_parse(RECORD,resp)
	        aid = ui.tree_get(ui.tree_find(RECORD,nil,"4F")..":0")
	        table.insert(AID_LIST,aid['node'])
		rec = rec + 1
             end
	   until sw ~= 0x9000
	end
	return true
end

function emv_process_application(cardenv,aid)
	local app
	local ref
	local gpo
	local PDOL
	local AFL
	local LOG
	local extra
	local j -- counter


	-- Select AID
	sw,resp = card.select_file(aid)
	if sw ~=0x9000 then
	   return false
	end


	-- Process 'File Control Infomation' and get PDOL
	app = ui.tree_append(cardenv,false,"application",aid,nil,nil)
	emv_parse(app,resp)
	ref = ui.tree_find(app,nil,"9F38")
	if (ref) then
	   ref = ui.tree_get(ref..":0")
	   PDOL = ref['node']
	else
	   PDOL = '00';
	end


	-- find LOG INFO
	ref = ui.tree_find(app,nil,"9F4D")
	if (ref==nil) then
	   -- proprietary style ?
	   ref = ui.tree_find(app,nil,"DF60")
	end
	if (ref) then
	   ref = ui.tree_get(ref..":0")
	   LOG = card.bytes_unpack(ref['node'])
	   log.print(log.INFO,"Found transaction log indicator")
	else
	   LOG = nil
	   log.print(log.WARNING,"No transaction log indicator")
	end


	-- Get processing options
	log.print(log.INFO,"Attempting GPO")
	sw,resp = card.get_processing_options(PDOL)
	if sw ~=0x9000 then
	   log.print(log.ERROR,"GPO Failed")
	   return false
	end
	gpo = ui.tree_append(app,false,"processing_options",nil,nil,nil);
	emv_parse(gpo,resp)


	-- find AFL
	ref = ui.tree_find(gpo,nil,"80")
	ref = ui.tree_get(ref..":0")
	AFL = card.bytes_unpack(ref['node'])



	-- Read all the application data
	for i=3,#AFL,4 do
	    local sfi
	    local rec
	    log.print(log.INFO,string.format("Reading SFI %i",bit_shr(AFL[i],3)))
	    sfi = ui.tree_append(app,false,"file",bit_shr(AFL[i],3),nil,nil)
	    for j=AFL[i+1],AFL[i+2] do
	        local rec
		log.print(log.INFO,string.format("Reading record %i",j))
		sw,resp = card.read_record(bit_shr(AFL[i],3),j)
	        if sw ~= 0x9000 then
		   log.print(log.ERROR,"Read record failed")
		else
		   rec = ui.tree_append(sfi,false,"record",j,nil,nil)
		   emv_parse(rec,resp)
		end
	    end
	end


	-- Read logs if they exist
	if LOG then
     	   local logfile
	   local rec
	   log.print(log.INFO,string.format("Reading LOG SFI %i",LOG[1]))
	   logfile = ui.tree_append(app,false,"file",LOG[1],nil,nil)
	   for j =1,LOG[2] do
	       sw,resp = card.read_record(LOG[1],j)
               if sw ~= 0x9000 then
                  log.print(log.ERROR,"Read log record failed")
	       else
	          rec = ui.tree_append(logfile,false,"record",j,card.bytes_size(resp),nil)
		  ui.tree_append(rec,true,resp,nil,nil,nil)
	       end
	   end 
	end


	-- Read extra data 
	extra = ui.tree_append(app,false,"extra emv data",nil,nil,nil)
	for j=1,#EXTRA_DATA do
	    sw,resp = card.get_data(EXTRA_DATA[j])
	    if sw == 0x9000 then
	       emv_parse(extra,resp)
	    end
	end

	return true
end


CPLC_DATA =
{
  { "IC Fabricator", 2, 0 } ,
  { "IC Type", 2, 0 },
  { "Operating System Provider Identifier", 2 },
  { "Operating System Release Date", 2 },
  { "Operating System Release Level", 2 },
  { "IC Fabrication Date", 2 },
  { "IC Serial Number", 4 },
  { "IC Batch Identifier", 2 },
  { "IC ModuleFabricator", 2 },
  { "IC ModulePackaging Date", 2 },
  { "ICC Manufacturer", 2 },
  { "IC Embedding Date", 2 },
  { "Prepersonalizer Identifier", 2 },
  { "Prepersonalization Date", 2 },
  { "Prepersonalization Equipment", 4 },
  { "Personalizer Identifier", 2 },
  { "Personalization Date", 2 },
  { "Personalization Equipment", 4 },
}


function emv_process_cplc(cardenv)
	local cplc
	local cplc_data
	local i
	local pos
        local ref

	log.print(log.INFO,"Processing CPLC data")
	sw,resp = card.get_data(0x9F7F)
	if sw == 0x9000 then
	   cplc      = ui.tree_append(cardenv,false,"cpcl",nil,nil,nil)
	   cplc_data = card.tlv_split(resp)
	   ref = ui.tree_append(cplc,false,"cplc data","9F7F",card.bytes_size(cplc_data[1][2]),nil)
	   cplc_data = cplc_data[1][2]
	   ui.tree_append(ref,true,cplc_data,nil,nil,nil) 
	   pos = 0
	   for i=1,#CPLC_DATA do
	       ui.tree_append(cplc,
			      false,
			      CPLC_DATA[i][1],
			      card.bytes_substr(cplc_data,pos,CPLC_DATA[i][2]),
			      nil,
			      string.format("pos=%i",pos))
	       pos = pos + CPLC_DATA[i][2]
	   end
	end
end


-- PROCESSING

card.connect()

mycard = card.tree_startup("EMV")

emv_process_pse(mycard)
card.warm_reset()

for i=1,#AID_LIST
do
        print(AID_LIST[i])
	emv_process_application(mycard,AID_LIST[i])
	card.warm_reset()
end

emv_process_cplc(mycard)

card.disconnect()



