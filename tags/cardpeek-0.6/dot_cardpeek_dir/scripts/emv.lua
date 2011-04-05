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
-- History:
-- Aug 07 2010: Corrected bug in GPO command.
-- Aug 08 2010: Corrected bug in AFL data processing
-- Mar 27 2011: Added CVM patch from Adam Laurie.

require('lib.tlv')
require('lib.strict')

--------------------------------------------------------------------------
-- GLOBAL EMV CARD COMMANDS extending general lib.apdu 
--------------------------------------------------------------------------

function card.get_processing_options(pdol)
	local command;
        if pdol and #pdol>0 then
	   command = bytes.new(8,"80 A8 00 00",#pdol+2,0x83,#pdol,pdol)
	else
	   command = bytes.new(8,"80 A8 00 00 02 83 00")
	end
	return card.send(command)
end

-- override card.get_data() for EMV
function card.get_data(data)
        local command = bytes.new(8,"80 CA",bit.AND(bit.SHR(data,8),0xFF),bit.AND(data,0xFF),0)
        return card.send(command)
end


-- ------------------------------------------------------------------------
-- EMV 
-- ------------------------------------------------------------------------

TRANSACTION_TYPE = {
  [0]="Purchase",
  [1]="Cash"
}

function ui_parse_transaction_type(cc)
	local tt = TRANSACTION_TYPE[cc[0]]
	if tt == nil then 
	   return tostring(cc)
	end
	return tt
end

function ui_parse_CVM(node,data)
	local i
	local left
	local right
	local leftstring
	local rightstring
	local out = "X="
	for i = 0, 3 do
		out = out .. string.format("%02X",data[i])
	end
	out = out .. ", Y="
	for i = 4, 7 do
		out = out .. string.format("%02X",data[i])
	end
	for i= 4,(#data/2)-1 do
		out = out .. "\n * "
		left = data[i*2]
		right = data[i*2+1]
		if bit.AND(left,0x40) == 0x40 then
			out = out .. "Apply succeeding CV rule if this rule is unsuccessful: "
		else
			out = out .. "Fail cardholder verification if this CVM is unsuccessful: "
		end
		if CVM_REFERENCE_BYTE1[bit.AND(left,0xBF)] == nil then
			leftstring = string.format("Unknown (%02X)",left)
		else
			leftstring = CVM_REFERENCE_BYTE1[bit.AND(left,0xBF)]
		end
		if CVM_REFERENCE_BYTE2[right] == nil then
			rightstring = string.format("Unknown (%02X)",right)
		else
			rightstring = CVM_REFERENCE_BYTE2[right]
		end
		out = out .. leftstring .. " - " .. rightstring
	end
        ui.tree_set_value(node,data)
	ui.tree_set_alt_value(node,out)
	return true
end

CVM_REFERENCE_BYTE1 = {
   [0x00] = "Fail CVM processing",
   [0x01] = "Plaintext PIN verification performed by ICC",
   [0x02] = "Enciphered PIN verified online",
   [0x03] = "Plaintext PIN verification performed by ICC and signature (paper)",
   [0x04] = "Enciphered PIN verification performed by ICC",
   [0x05] = "Enciphered PIN verification performed by ICC and signature (paper)",
   [0x1E] = "Signature (paper)",
   [0x1F] = "No CVM Required",
}

CVM_REFERENCE_BYTE2 = {
   [0x00] = "Always",
   [0x01] = "If unattended cash",
   [0x02] = "If not attended cash and not manual cash and not purchase with cashback",
   [0x03] = "If terminal supports the CVM",
   [0x04] = "If manual cash",
   [0x05] = "If purchase with cashback",
   [0x06] = "If transaction is in the application currency and is under X value",
   [0x07] = "If transaction is in the application currency and is over X value",
   [0x08] = "If transaction is in the application currency and is under Y value",
   [0x09] = "If transaction is in the application currency and is over Y value"
}	

EMV_REFERENCE = {
   ['5D'] = {"Directory Definition File (DDF) Name" }, 
   ['70'] = {"Application Data File (ADF)" }, 
   ['80'] = {"Response Message Template Format 1" }, 
   ['82'] = {"Application Interchange Profile (AIP)" }, 
   ['87'] = {"Application Priority Indicator" }, 
   ['88'] = {"Short File Identifier (SFI)" }, 
   ['8C'] = {"Card Risk Management Data Object List 1 (CDOL1)" }, 
   ['8D'] = {"Card Risk Management Data Object List 2 (CDOL2)" }, 
   ['8E'] = {"Cardholder Verification Method (CVM) List", ui_parse_CVM }, 
   ['8F'] = {"Certificate Authority Public Key Index (PKI)" }, 
   ['90'] = {"Issuer PK Certificate" }, 
   ['91'] = {"Issuer Authentication Data" }, 
   ['92'] = {"Issuer PK Remainder" }, 
   ['93'] = {"Signed Static Application Data" }, 
   ['94'] = {"Application File Locator (AFL)" },
   ['95'] = {"Terminal Verification Results (TVR)" },
   ['97'] = {"Transaction Certificate Data Object List (TDOL)" }, 
   ['9A'] = {"Transaction Date", ui_parse_YYMMDD },
   ['9C'] = {"Transaction Type", ui_parse_transaction_type },
   ['A5'] = {"FCI Proprietary Template" }, 
   ['9F02'] = {"Amount, Authorized" },
   ['9F03'] = {"Amount, Other" },
   ['9F05'] = {"Application Discretionary Data" }, 
   ['9F07'] = {"Application Usage Control" }, 
   ['9F08'] = {"Application Version Number" }, 
   ['9F0B'] = {"Cardholder Name - Extended" }, 
   ['9F0D'] = {"Issuer Action Code - Default" }, 
   ['9F0E'] = {"Issuer Action Code - Denial" }, 
   ['9F0F'] = {"Issuer Action Code - Online" }, 
   ['9F10'] = {"Issuer Application Data" }, 
   ['9F11'] = {"Issuer Code Table Index" }, 
   ['9F12'] = {"Application Preferred Name" }, 
   ['9F13'] = {"Last Online ATC Register" }, 
   ['9F14'] = {"Lower Consecutive Offline Limit (Terminal Check)" }, 
   ['9F17'] = {"PIN Try Counter" }, 
   ['9F19'] = {"Dynamic Data Authentication Data Object List (DDOL)" }, 
   ['9F1A'] = {"Terminal Country Code", ui_parse_country_code },
   ['9F1F'] = {"Track 1 Discretionary Data", ui_parse_printable }, 
   ['9F23'] = {"Upper Consecutive Offline Limit (Terminal Check)" }, 
   ['9F26'] = {"Application Cryptogram (AC)" }, 
   ['9F27'] = {"Cryptogram Information Data" }, 
   ['9F2D'] = {"ICC PIN Encipherment Public Key Certificate" }, 
   ['9F2E'] = {"ICC PIN Encipherment Public Key Exponent" }, 
   ['9F2F'] = {"ICC PIN Encipherment Public Key Remainder" },
   ['9F32'] = {"Issuer PK Exponent" }, 
   ['9F36'] = {"Application Transaction Counter (ATC)" }, 
   ['9F38'] = {"Processing Options Data Object List (PDOL)" }, 
   ['9F42'] = {"Application Currency Code" }, 
   ['9F44'] = {"Application Currency Exponent" }, 
   ['9F45'] = {"Data Authentication Code" }, 
   ['9F46'] = {"ICC Public Key Certificate" }, 
   ['9F47'] = {"ICC Public Key Exponent" }, 
   ['9F48'] = {"ICC Public Key Remainder" }, 
   ['9F4A'] = {"Static Data Authentication Tag List" }, 
   ['9F4B'] = {"Signed Dynamic Application Data" }, 
   ['9F4C'] = {"Integrated Circuit Card (ICC) Dynamic Number" },
   ['9F4F'] = {"Log Fromat" },
   ['9F51'] = {"Application Currency Code" },
   ['9F52'] = {"Card Verification Results (CVR)" }, 
   ['9F53'] = {"Consecutive Transaction Limit (International)" }, 
   ['9F54'] = {"Cumulative Total Transaction Amount Limit" }, 
   ['9F55'] = {"Geographic Indicator" }, 
   ['9F56'] = {"Issuer Authentication Indicator" },
   ['9F57'] = {"Issuer Country Code" }, 
   ['9F58'] = {"Lower Consecutive Offline Limit (Card Check)" },
   ['9F59'] = {"Upper Consecutive Offline Limit (Card Check)" }, 
   ['9F5A'] = {"Issuer URL2" }, 
   ['9F5C'] = {"Cumulative Total Transaction Amount Upper Limit" }, 
   ['9F72'] = {"Consecutive Transaction Limit (International - Country)" }, 
   ['9F73'] = {"Currency Conversion Factor" }, 
   ['9F74'] = {"VLP Issuer Authorization Code" }, 
   ['9F75'] = {"Cumulative Total Transaction Amount Limit - Dual Currency" }, 
   ['9F76'] = {"Secondary Application Currency Code" }, 
   ['9F77'] = {"VLP Funds Limit" }, 
   ['9F78'] = {"VLP Single Transaction Limit" }, 
   ['9F79'] = {"VLP Available Funds" }, 
   ['9F7F'] = {"Card Production Life Cycle (CPLC) History File Identifiers" }, 
   ['BF0C'] = {"FCI Issuer Discretionary Data" }
}

function emv_parse(cardenv,tlv)
	tlv_parse(cardenv,tlv,EMV_REFERENCE)
end

PSE = "#315041592E5359532E4444463031"

AID_LIST = { 
  "#A0000000421010",
  "#A0000000422010",
  "#A0000000031010",
  "#A0000000032010",
  "#A0000000041010",
  "#A0000000042010",
  "#A00000006900"
}

EXTRA_DATA = { 0x9F36, 0x9F13, 0x9F17, 0x9F4D, 0x9F4F }


function emv_process_pse(cardenv)
	local sw, resp
	local APP
	local ref
	local sfi
	local FILE
	local rec
	local REC
	local RECORD
	local aid
	local warm_atr

        sw, resp = card.select(PSE)
	
	-- Could it be a french card ?
	if sw == 0x6E00 then 
	   local ATR
	   card.warm_reset()
	   warm_atr = card.last_atr()
	   ATR = ui.tree_add_node(cardenv, "ATR", "warm", #warm_atr, "block")
	   ui.tree_set_value(ATR,warm_atr)
           sw, resp = card.select(PSE)
	end

	if sw ~= 0x9000 then
     	   log.print(log.INFO,"No PSE")
	   return false
	end

	-- Construct tree
	APP = ui.tree_add_node(cardenv,"application",PSE,nil,"application")
	emv_parse(APP,resp)

	ref = ui.tree_find_node(APP,nil,"88")
	if (ref) then
	   sfi = ui.tree_get_value(ref)
	   FILE = ui.tree_add_node(APP,"file",sfi[0],nil,"file")

	   rec = 1
	   AID_LIST = {}
	   repeat 
	     sw,resp = card.read_record(sfi[0],rec)
	     if sw == 0x9000 then
	        RECORD = ui.tree_add_node(FILE,"record",tostring(rec),nil,"record")
	        emv_parse(RECORD,resp)
	        aid = tostring(ui.tree_get_value(ui.tree_find_node(RECORD,nil,"4F")))
	        table.insert(AID_LIST,"#"..aid)
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

	LOG_FILE   = ui.tree_add_node(application,"file",tostring(log_sfi),nil,"file")
	LOG_DATA   = ui.tree_add_node(LOG_FILE,"log data")
 	
	log.print(log.INFO,string.format("Reading LOG SFI %i",log_sfi))
	for log_recno =1,log_max do
           sw,resp = card.read_record(log_sfi, log_recno)
	   if sw ~= 0x9000 then
                  log.print(log.WARNING,"Read log record failed")
		  break
	   else
	          REC = ui.tree_add_node(LOG_DATA,"record",log_recno,#resp,"record")
		  ui.tree_set_value(REC,resp)
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
	local tag_func
	local len
	local i
	local item
	local item_pos
	local LOG_FILE
	local LOG_DATA
	local LOG_FORMAT
	local REC
	local ITEM
	local ITEM_AMOUNT
	local log_recno
	local data
	local currency_code, currency_name, currency_digits

	LOG_FILE   = ui.tree_add_node(application,"file",log_sfi,nil,"file")
	LOG_FORMAT = ui.tree_add_node(LOG_FILE,"log format")
	LOG_DATA   = ui.tree_add_node(LOG_FILE,"log data")
 	
	sw, log_format = card.get_data(0x9F4F)
	if sw ~=0x9000 then
	   log.print(log.ERROR,"Failed to get log format information, transaction logs won't be analyzed")
	   return false
	end

	log_tag, log_format = asn1.split(log_format)

	i = 1
	item = ""
	ui.tree_set_value(LOG_FORMAT,log_format);
	while log_format 
	do
	    tag, log_format = asn1.split_tag(log_format)
	    len, log_format = asn1.split_length(log_format)
	    tag_name, tag_func = tlv_tag_info(tag,EMV_REFERENCE,0);
	    log_items[i]= { tag, len, tag_name, tag_func }
	    i = i+1
	    item = item .. string.format("%X[%d] ", tag, len);
	end
	ui.tree_set_alt_value(LOG_FORMAT,item);

	log.print(log.INFO,string.format("Reading LOG SFI %i",log_sfi))
	for log_recno =1,log_max do
           sw,resp = card.read_record(log_sfi,log_recno)
	   if sw ~= 0x9000 then
                  log.print(log.WARNING,"Read log record failed")
		  break
	   else
	          REC = ui.tree_add_node(LOG_DATA,"record",log_recno,#resp,"record")
		  item_pos = 0
		  ITEM_AMOUNT = nil
		  currency_digits = 0
		  for i=1,#log_items do
		     if log_items[i][3] then
		        ITEM = ui.tree_add_node(REC,log_items[i][3],nil,log_items[i][2])
		     else
		        ITEM = ui.tree_add_node(REC,string.format("tag %X",log_items[i][1]),nil,log_items[i][2])
		     end

		     data = bytes.sub(resp,item_pos,item_pos+log_items[i][2]-1)
	             ui.tree_set_value(ITEM,data)

		     if log_items[i][1]==0x9F02 then
		        ITEM_AMOUNT=ITEM
		     elseif log_items[i][1]==0x5F2A then
	                currency_code   = tonumber(tostring(data))
		        currency_name   = iso_currency_code_name(currency_code)
	                currency_digits = iso_currency_code_digits(currency_code)
	                ui.tree_set_alt_value(ITEM,currency_name)
		     elseif log_items[i][4] then
		        log_items[i][4](ITEM,data)
                     end
	             item_pos = item_pos + log_items[i][2]
	          end

	          if ITEM_AMOUNT then
		     ui.tree_set_alt_value(ITEM_AMOUNT,string.format("%.2f",tostring(ui.tree_get_value(ITEM_AMOUNT))/(10^currency_digits)))
		  end
	   end
	end 

end

function emv_process_application(cardenv,aid)
	local sw, resp
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
	sw,resp = card.select(aid)
	if sw ~=0x9000 then
	   return false
	end


	-- Process 'File Control Infomation' and get PDOL
	APP = ui.tree_add_node(cardenv,"application",aid,nil,"application")
	emv_parse(APP,resp)
	ref = ui.tree_find_node(APP,nil,"9F38")
	if (ref) then
	   pdol = ui.tree_get_value(ref)
	else
	   pdol = nil;
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
           LOG = ui.tree_get_value(ref)
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
                if pdol then
                    -- try empty GPO just in case the card is blocking some stuff
                    log.print(log.WARNING,
                              string.format("GPO with data failed with code %X, retrying GPO without data",sw))
                    sw,resp = card.get_processing_options(nil)	
                end
                if sw ~=0x9000 then
                    log.print(log.ERROR,"GPO Failed")
                    return false
                end
            end
	    GPO = ui.tree_add_node(APP,"processing_options");
	    emv_parse(GPO,resp)


	    -- find AFL
	    ref = ui.tree_find_node(GPO,nil,"80")
	    AFL = ui.tree_get_value(ref)

            if AFL then
	        -- Read all the application data
	        for i=2,#AFL-1,4 do
	            local sfi
	            local rec
	            log.print(log.INFO,string.format("Reading SFI %i",bit.SHR(AFL[i],3)))
	            sfi = ui.tree_add_node(APP,"file",bit.SHR(AFL[i],3),nil,"file")
	            for j=AFL[i+1],AFL[i+2] do
		        log.print(log.INFO,string.format("Reading record %i",j))
		        sw,resp = card.read_record(bit.SHR(AFL[i],3),j)
	                if sw ~= 0x9000 then
		            log.print(log.ERROR,"Read record failed")
		        else
		            rec = ui.tree_add_node(sfi,"record",j,nil,"record")
		            emv_parse(rec,resp)
		        end
	            end -- for
	        end -- for
            else
                log.print(log.LOG_WARNING,"No AFL (Application File Locator) found in GPO data.")
            end -- AFL
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
	local sw, resp
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
	   CPLC      = ui.tree_add_node(cardenv,"cpcl",nil,nil,"block")
	   cplc_tag, cplc_data = asn1.split(resp)
	   ref = ui.tree_add_node(CPLC,"cplc data","9F7F",#cplc_data)
	   ui.tree_set_value(ref,cplc_data) 
	   pos = 0
	   for i=1,#CPLC_DATA do
	       ref2 = ui.tree_add_node(CPLC,
				       CPLC_DATA[i][1],
				       pos);
	       ui.tree_set_value(ref2,bytes.sub(cplc_data,pos,pos+CPLC_DATA[i][2]-1))
	       pos = pos + CPLC_DATA[i][2]
	   end
	end
end


-- PROCESSING

if card.connect() then

   local mycard = card.tree_startup("EMV")

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
else
   ui.question("No card detected in reader",{"OK"})
end
