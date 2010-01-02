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
-- GLOBAL EMV CARD COMMANDS extending general lib.apdu 
--------------------------------------------------------------------------

function card.get_processing_options(pdol)
	local command = bytes.new(8,"80 A8 00 00",#pdol+1,0x83,pdol)
	return card.send(command)
end

function card.get_data(data)
	local command = bytes.new(8,"80 CA", bit.AND(bit.SHR(data,8),0xFF), bit.AND(data,0xFF), 0)
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

EXTRA_DATA = { 0x9F36, 0x9F13, 0x9F17, 0x9F4D, 0x9F4F }


function emv_process_pse(cardenv)
	local APP
	local ref
	local sfi
	local REC
	local RECORD
	local aid

        sw, resp = card.select_file(PSE)
	
	-- Could it be a french card ?
	if sw == 0x6E00 then 
	   local ATR
	   card.warm_reset()
	   ATR = ui.tree_add_node(cardenv,"ATR","warm")
	   ui.tree_set_value(ATR,tostring(card.last_atr()))
           sw, resp = card.select_file(PSE)
	end

	if sw ~= 0x9000 then
     	   log.print(log.INFO,"No PSE")
	   return false
	end

	-- Construct tree
	APP = ui.tree_add_node(cardenv,"application",PSE,nil,"PSE")
	emv_parse(APP,resp)

	ref = ui.tree_find_node(APP,nil,"88")
	if (ref) then
	   sfi = bytes.new(8,ui.tree_get_value(ref))
	   FILE = ui.tree_add_node(APP,"file",sfi[0])

	   rec = 1
	   AID_LIST = {}
	   repeat 
	     sw,resp = card.read_record(sfi[0],rec)
	     if sw == 0x9000 then
	        RECORD = ui.tree_add_node(FILE,"record",tostring(rec))
	        emv_parse(RECORD,resp)
	        aid = ui.tree_get_value(ui.tree_find_node(RECORD,nil,"4F"))
	        table.insert(AID_LIST,aid)
		rec = rec + 1
             end
	   until sw ~= 0x9000
	else
	   log.print(log.WARNING,"SFI indicator (tag 88) not found in PSE")
	end
	return true
end

function visa_process_application_logs(application, log_sfi, log_max)
	local sw, resp
	local LOG_FILE
	local LOG_DATA
	local LOG_FORMAT
	local REC
	local log_recno

	LOG_FILE   = ui.tree_add_node(application,"file",tostring(log_sfi))
	LOG_DATA   = ui.tree_add_node(LOG_FILE,"log data")
 	
	log.print(log.INFO,string.format("Reading LOG SFI %i",log_sfi))
	for log_recno =1,log_max do
           sw,resp = card.read_record(log_sfi, log_recno)
	   if sw ~= 0x9000 then
                  log.print(log.WARNING,"Read log record failed")
		  break
	   else
	          REC = ui.tree_add_node(LOG_DATA,"record",log_recno,#resp)
		  ui.tree_set_value(REC,tostring(resp))
	   end
	end 

end


function emv_process_application_logs(application, log_sfi, log_max)
	local log_format
	local log_tag
	local log_items = { }
	local sw, resp
	local tag
	local tag_name
	local len
	local i
	local item
	local item_pos
	local LOG_FILE
	local LOG_DATA
	local LOG_FORMAT
	local REC
	local log_recno

	LOG_FILE   = ui.tree_add_node(application,"file",log_sfi)
	LOG_FORMAT = ui.tree_add_node(LOG_FILE,"log format")
	LOG_DATA   = ui.tree_add_node(LOG_FILE,"log data")
 	
	sw, log_format = card.get_data(0x9F4F)
	if sw ~=0x9000 then
	   log.print(log.ERROR,"Failed to get log format information, transaction logs won't be analyzed")
	   return false
	end

	log_tag, log_format = asn1.split(log_format)

	i = 1
	while log_format 
	do
	    tag, log_format = asn1.split_tag(log_format)
	    len, log_format = asn1.split_length(log_format)
	    log_items[i]= { tag, len }
	    i = i+1
	    tag_name = tlv_tag_name(tag,EMV_REFERENCE,0);
	    if tag_name then
	        ui.tree_add_node(LOG_FORMAT,tag_name,string.format("%X",tag),len)
	    else
	        ui.tree_add_node(LOG_FORMAT,"tlv",string.format("%X",tag),len)
	    end
	end

	log.print(log.INFO,string.format("Reading LOG SFI %i",log_sfi))
	for log_recno =1,log_max do
           sw,resp = card.read_record(log_sfi,log_recno)
	   if sw ~= 0x9000 then
                  log.print(log.WARNING,"Read log record failed")
		  break
	   else
	          REC = ui.tree_add_node(LOG_DATA,"record",log_recno,#resp)
		  item = ""
		  item_pos = 0
		  for i=1,#log_items do
		     item = item .. tostring(bytes.sub(resp,item_pos,item_pos+log_items[i][2]-1)) .. "  "
	             item_pos = item_pos + log_items[i][2]
	          end
		  ui.tree_set_value(REC,item)
	   end
	end 

end

function emv_process_application(cardenv,aid)
	local APP
	local ref
	local GPO
	local pdol
	local AFL
	local LOG
	local extra
	local j -- counter
	local logformat

	log.print(log.INFO,"Processing application "..aid)

	-- Select AID
	sw,resp = card.select_file(aid)
	if sw ~=0x9000 then
	   return false
	end


	-- Process 'File Control Infomation' and get PDOL
	APP = ui.tree_add_node(cardenv,"application",aid)
	emv_parse(APP,resp)
	ref = ui.tree_find_node(APP,nil,"9F38")
	if (ref) then
	   pdol = bytes.new(8,ui.tree_get_value(ref))
	else
	   pdol = bytes.new(8,0);
	end


	-- find LOG INFO
	logformat=nil
	ref = ui.tree_find_node(APP,nil,"9F4D")
	-- I've seen this on some cards :
	if ref==nil then
	   -- proprietary style ?
	   ref = ui.tree_find_node(APP,nil,"DF60")
	   logformat = "VISA"
	else
	   logformat = "EMV"
	end

	if ref then
           LOG = bytes.new(8,ui.tree_get_value(ref))
	else
	   sw, resp = card.get_data(0x9F4D)
      	   if sw==0x9000 then
              LOG = tostring(resp)
	      logformat = "EMV"
	   else 
	      sw, resp = card.get_data(0xDF60)
	      if sw==0x9000 then
                 LOG = tostring(resp) 
	         logformat = "VISA"
	      else
	         logformat = nil
                 LOG = nil
	      end
	   end
	end

	if logformat then
	   log.print(log.INFO,"Found "..logformat.." transaction log indicator")
	else
	   log.print(log.INFO,"No transaction log indicator")
	end


	if ui.question("Issue a GET PROCESSING OPTIONS command?",{"Yes","No"})==1 then
	-- Get processing options
	log.print(log.INFO,"Attempting GPO")
	sw,resp = card.get_processing_options(pdol)
	if sw ~=0x9000 then
	   log.print(log.ERROR,"GPO Failed")
	   return false
	end
	GPO = ui.tree_add_node(APP,"processing_options");
	emv_parse(GPO,resp)


	-- find AFL
	ref = ui.tree_find_node(GPO,nil,"80")
	AFL = bytes.new(8,ui.tree_get_value(ref))


	-- Read all the application data
	for i=2,#AFL-1,4 do
	    local sfi
	    local rec
	    log.print(log.INFO,string.format("Reading SFI %i",bit.SHR(AFL[i],3)))
	    sfi = ui.tree_add_node(APP,"file",bit.SHR(AFL[i],3))
	    for j=AFL[i+1],AFL[i+2] do
		log.print(log.INFO,string.format("Reading record %i",j))
		sw,resp = card.read_record(bit.SHR(AFL[i],3),j)
	        if sw ~= 0x9000 then
		   log.print(log.ERROR,"Read record failed")
		else
		   rec = ui.tree_add_node(sfi,"record",j)
		   emv_parse(rec,resp)
		end
	    end
	end
	end -- GPO

	-- Read logs if they exist
	if logformat=="EMV" then
           emv_process_application_logs(APP,LOG[0],LOG[1])
	elseif logformat=="VISA" then
	   visa_process_application_logs(APP,LOG[0],LOG[1])
	end

	-- Read extra data 
	extra = ui.tree_add_node(APP,"extra emv data")
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
	local CPLC
	local cplc_data
	local cplc_tag
	local i
	local pos
        local ref
	local ref2

	log.print(log.INFO,"Processing CPLC data")
	sw,resp = card.get_data(0x9F7F)
	if sw == 0x9000 then
	   CPLC      = ui.tree_add_node(cardenv,"cpcl")
	   cplc_tag, cplc_data = asn1.split(resp)
	   ref = ui.tree_add_node(CPLC,"cplc data","9F7F",#cplc_data)
	   ui.tree_set_value(ref,tostring(cplc_data)) 
	   pos = 0
	   for i=1,#CPLC_DATA do
	       ref2 = ui.tree_add_node(CPLC,
				       CPLC_DATA[i][1],
				       nil,
				       nil,
				       string.format("pos=%i",pos))
	       ui.tree_set_value(ref2,tostring(bytes.sub(cplc_data,pos,pos+CPLC_DATA[i][2]-1)))
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
        -- print(AID_LIST[i])
	emv_process_application(mycard,AID_LIST[i])
	card.warm_reset()
end

emv_process_cplc(mycard)

card.disconnect()



