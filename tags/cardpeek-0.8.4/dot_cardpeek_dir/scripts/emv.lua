--
-- This file is part of Cardpeek, the smartcard reader utility.
--
-- Copyright 2009-2015 by 'L1L1'
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
-- @name EMV
-- @description Bank cards 'PIN and chip'
-- @targets 0.8
--
-- History:
-- Aug 07 2010: Corrected bug in GPO command.
-- Aug 08 2010: Corrected bug in AFL data processing
-- Mar 27 2011: Added CVM patch from Adam Laurie.
-- Jan 23 2012: Added UK Post Office Card Account in AID list from Tyson Key.
-- Mar 25 2012: Added a few AIDs
-- Feb 21 2014: Better NFC compatibility by using current date in GPO.
-- Nov 16 2014: Many improvements from Andrew Kozlic (Many new AIDs, parses AIP, GPO Format 1, AFLs, and more.)
-- @update March 05 2014: Avoid error when card don't have tag 82 or 94 in GPO (issue 62)

require('lib.tlv')
require('lib.strict')

--------------------------------------------------------------------------
-- GLOBAL EMV CARD COMMANDS extending general lib.apdu 
--------------------------------------------------------------------------

function build_empty_pdol_data_block(pdol)
    local data = bytes.new(8);
    local o_tag
    local o_len

    while pdol do
        o_tag, pdol = asn1.split_tag(pdol)
        o_len, pdol = asn1.split_tag(pdol)

        if o_tag==0x9F66 and o_len==4 then
            --  Terminal Transaction Qualifiers (VISA)
            data = data .. "30 00 00 00"
        elseif o_tag==0x9F1A and o_len==2 then
            -- Terminal country code
            data = data .. "0250"
        elseif o_tag==0x5F2A and o_len==2 then
            -- Transaction currency code
            data = data .. "0978"
        elseif o_tag==0x9A and o_len==3 then
            -- Transaction date
            data = data .. os.date("%y %m %d")
        elseif o_tag==0x9F37 and o_len==4 then
            -- Unpredictable number
            data = data .. "DEADBEEF"
        else
            -- When in doubt Zeroize
            while o_len > 0 do
                data = data .. 0x00
                o_len = o_len-1 
            end
        end
    end
    return data
end

function card.get_processing_options(pdol)
	local command
    local pdol_data
    if pdol and #pdol>0 then
       pdol_data = build_empty_pdol_data_block(pdol)
	   command = bytes.new(8,"80 A8 00 00",#pdol_data+2,0x83,#pdol_data,pdol_data,"00")
	else
	   command = bytes.new(8,"80 A8 00 00 02 83 00 00")
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

function ui_parse_transaction_type(node,cc)
	local tt = TRANSACTION_TYPE[cc[0]]
	if tt == nil then 
	   return tostring(cc)
	end
	nodes.set_attribute(node,"val",cc)
	nodes.set_attribute(node,"alt",tt)
	return true
end

function ui_parse_CVM(node,data)
	local i
	local left
	local right
	local leftstring
	local rightstring
	local subnode
	local out 

	nodes.append(node,{	classname="item", 
			      	label="X", 
				size=4,
				val=bytes.sub(data,0,3)})

	nodes.append(node,{	classname="item", 
				label="Y", 
				size=4,
				val=bytes.sub(data,4,7)})

	for i= 4,(#data/2)-1 do
		left = data:get(i*2)
		right = data:get(i*2+1)

		if left == 0 and right == 0 then
			-- Amex pads the list with zeroes at the end
			-- Once we reach the zeroes, we're done parsing
			break
		end
		
		subnode = nodes.append(node, { classname="item",
						   label="CVM",
						   id=i-3,
						   size=2,
						   val=bytes.sub(data,i*2,i*2+1)})

		if bit.AND(left,0x40) == 0x40 then
			out = "Apply succeeding CV rule if this rule is unsuccessful: "
		else
			out = "Fail cardholder verification if this CVM is unsuccessful: "
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
		nodes.set_attribute(subnode,"alt",out)
	end
        nodes.set_attribute(node,"val",data)
	return true
end


function ui_parse_bits(node,data,reference,data_label)
    local i
    local j
    local meaning
    
    nodes.set_attribute(node,"val",data)
    for i = 1,#data do
        for j = 8,1,-1 do
            if bit32.extract(data[i-1], j-1) ~= 0 then
    	        if reference[i][j] == nil then
                    meaning = "Unknown"
    	        else
                    meaning = reference[i][j]
                end
                nodes.append(node,{ classname="item",
                                                  label=data_label,
                                                  id=string.format("%d.%d",i,j),
                                                  val=meaning })
    	    end
        end
    end
end

function ui_parse_AIP(node,data)
    ui_parse_bits(node,data,AIP_REFERENCE,"Application Function")
end

function ui_parse_AUC(node,data)
    ui_parse_bits(node,data,AUC_REFERENCE,"Application Usage")
end

function ui_parse_IAC(node,data)
    ui_parse_bits(node,data,IAC_REFERENCE,"Condition")
end

function ui_parse_DOL(node,data)
    local tag
    local len
    local tag_name
    local tag_func
    local list

    nodes.set_attribute(node, "val", data)

    list = ""
    while data 
    do
        tag, data = asn1.split_tag(data)
        len, data = asn1.split_length(data)
        tag_name, tag_func = tlv_tag_info(tag, EMV_REFERENCE, 0);
        if tag_name == nil then
            tag_name = TLV_TYPES[bit.SHR(tlv_tag_msb(tag),6)+1];
        end
        list = list .. string.format("%s %X [%d]. ", tag_name, tag, len);
    end
    nodes.set_attribute(node, "alt", list);
end

function ui_parse_TL(node,data)
    local tag
    local tag_name
    local tag_func
    local list

    nodes.set_attribute(node, "val", data)

    list = ""
    while data 
    do
        tag, data = asn1.split_tag(data)
        tag_name, tag_func = tlv_tag_info(tag, EMV_REFERENCE, 0);
        if tag_name == nil then
            tag_name = TLV_TYPES[bit.SHR(tlv_tag_msb(tag),6)+1];
        end
        list = list .. string.format("%s %X. ", tag_name, tag);
    end
    nodes.set_attribute(node, "alt", list);
end

function ui_parse_AFL(node,data)
    local ref
    local i

    nodes.set_attribute(node, "val", data)

    if #data % 4 ~= 0 then
	log.print(log.WARNING,"Size of AFL is not a multiple of 4")
        return
    end

    for i=0,#data-1,4 do 
        ref = nodes.append(node,{ classname="item",
                                  label="Item",
                                  id=i/4 + 1,
                                  val=bytes.sub(data,i,i+3),
                                  size=4 })
        nodes.append(ref,{ classname="item",
                           label="Short File Identifier (SFI)",
                           id='88',
                           val=bit.SHR(data:get(i),3),
                           size=1 })
        nodes.append(ref,{ classname="item",
                           label="First record",
                           val=data:get(i+1),
                           size=1 })
        nodes.append(ref,{ classname="item",
                           label="Last record",
                           val=data:get(i+2),
                           size=1 })
        nodes.append(ref,{ classname="item",
                           label="Number of records involved in offline data authentication",
                           val=data:get(i+3),
                           size=1 })
        
    end
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

AIP_REFERENCE = {
  [1] = { -- Byte 1
    [1] = "CDA supported",
    [3] = "Issuer authentication is supported",
    [4] = "Terminal risk management is to be performed",
    [5] = "Cardholder verification is supported",
    [6] = "DDA supported",
    [7] = "SDA supported"
  },
  [2] = { -- Byte 2
  }
}

AUC_REFERENCE = {
  [1] = { -- Byte 1
    [1] = "Valid at terminals other than ATMs",
    [2] = "Valid at ATMs",
    [3] = "Valid for international services",
    [4] = "Valid for domestic services",
    [5] = "Valid for international goods",
    [6] = "Valid for domestic goods",
    [7] = "Valid for international cash transactions",
    [8] = "Valid for domestic cash transactions"
  },
  [2] = { -- Byte 2
    [7] = "International cashback allowed",
    [8] = "Domestic cashback allowed"
  }
}

IAC_REFERENCE = {
  [1] = { -- Byte 1
    [3] = "CDA failed",
    [4] = "DDA failed",
    [5] = "Card appears on terminal exception file",
    [6] = "ICC data missing",
    [7] = "SDA failed",
    [8] = "Offline data authentication was not performed"
  },
  [2] = { -- Byte 2
    [4] = "New card",
    [5] = "Requested service not allowed for card product",
    [6] = "Application not yet effective",
    [7] = "Expired application",
    [8] = "ICC and terminal have different application versions"
  },
  [3] = { -- Byte 3
    [3] = "Online PIN entered",
    [4] = "PIN entry required, PIN pad present, but PIN was not entered",
    [5] = "PIN entry required and PIN pad not present or not working",
    [6] = "PIN Try Limit exceeded",
    [7] = "Unrecognised CVM",
    [8] = "Cardholder verification was not successful"
  },
  [4] = { -- Byte 4
    [4] = "Merchant forced transaction online",
    [5] = "Transaction selected randomly for online processing",
    [6] = "Upper consecutive offline limit exceeded",
    [7] = "Lower consecutive offline limit exceeded",
    [8] = "Transaction exceeds floor limit"
  },
  [5] = { -- Byte 5
    [5] = "Script processing failed after final GENERATE AC",
    [6] = "Script processing failed before final GENERATE AC",
    [7] = "Issuer authentication failed",
    [8] = "Default TDOL used"
  }
}

EMV_REFERENCE = {
   ['5D'] = {"Directory Definition File (DDF) Name" }, 
   ['61'] = {"Application Template" },
   ['70'] = {"Application Data File (ADF)" }, 
   ['71'] = {"Issuer Script Template 1" },
   ['77'] = {"Response Message Template Format 2" },
   ['80'] = {"Response Message Template Format 1" }, 
   ['82'] = {"Application Interchange Profile (AIP)", ui_parse_AIP }, 
   ['87'] = {"Application Priority Indicator" }, 
   ['88'] = {"Short File Identifier (SFI)" }, 
   ['89'] = {"Authorisation Code" },
   ['8A'] = {"Authorisation Response Code" },
   ['8C'] = {"Card Risk Management Data Object List 1 (CDOL1)", ui_parse_DOL }, 
   ['8D'] = {"Card Risk Management Data Object List 2 (CDOL2)", ui_parse_DOL }, 
   ['8E'] = {"Cardholder Verification Method (CVM) List", ui_parse_CVM }, 
   ['8F'] = {"Certificate Authority Public Key Index (PKI)" }, 
   ['90'] = {"Issuer PK Certificate" }, 
   ['91'] = {"Issuer Authentication Data" }, 
   ['92'] = {"Issuer PK Remainder" }, 
   ['93'] = {"Signed Static Application Data" }, 
   ['94'] = {"Application File Locator (AFL)", ui_parse_AFL },
   ['95'] = {"Terminal Verification Results (TVR)" },
   ['97'] = {"Transaction Certificate Data Object List (TDOL)", ui_parse_DOL }, 
   ['98'] = {"Transaction Certificate (TC) Hash Value" },
   ['99'] = {"Transaction Personal Identification Number (PIN) Data" },
   ['9A'] = {"Transaction Date", ui_parse_YYMMDD },
   ['9B'] = {"Transaction Status Information" },
   ['9C'] = {"Transaction Type", ui_parse_transaction_type },
   ['9D'] = {"Directory Definition File (DDF) Name" },
   ['A5'] = {"FCI Proprietary Template" }, 
   ['5F36'] = {"Transaction Currency Exponent" },
   ['5F53'] = {"International Bank Account Number (IBAN)" },
   ['5F54'] = {"Bank Identifier Code (BIC)" },
   ['5F55'] = {"Issuer Country Code (alpha2 format)" },
   ['5F56'] = {"Issuer Country Code (alpha3 format)" },
   ['9F01'] = {"Acquirer Identifier" },
   ['9F02'] = {"Amount, Authorized" },
   ['9F03'] = {"Amount, Other" },
   ['9F04'] = {"Amount, Other (Binary)" },
   ['9F05'] = {"Application Discretionary Data" }, 
   ['9F06'] = {"Application Identifier (AID) - terminal" },
   ['9F07'] = {"Application Usage Control", ui_parse_AUC }, 
   ['9F08'] = {"Application Version Number" }, 
   ['9F09'] = {"Application Version Number" },
   ['9F0B'] = {"Cardholder Name - Extended" }, 
   ['9F0D'] = {"Issuer Action Code - Default", ui_parse_IAC }, 
   ['9F0E'] = {"Issuer Action Code - Denial", ui_parse_IAC }, 
   ['9F0F'] = {"Issuer Action Code - Online", ui_parse_IAC }, 
   ['9F10'] = {"Issuer Application Data" }, 
   ['9F11'] = {"Issuer Code Table Index" }, 
   ['9F12'] = {"Application Preferred Name" }, 
   ['9F13'] = {"Last Online ATC Register", ui_parse_number }, 
   ['9F14'] = {"Lower Consecutive Offline Limit (Terminal Check)" }, 
   ['9F15'] = {"Merchant Category Code" },
   ['9F16'] = {"Merchant Identifier" },
   ['9F17'] = {"PIN Try Counter", ui_parse_number }, 
   ['9F18'] = {"Issuer Script Identifier" },
   ['9F19'] = {"Dynamic Data Authentication Data Object List (DDOL)", ui_parse_DOL }, 
   ['9F1A'] = {"Terminal Country Code", ui_parse_country_code },
   ['9F1B'] = {"Terminal Floor Limit" },
   ['9F1C'] = {"Terminal Identification" },
   ['9F1D'] = {"Terminal Risk Management Data" },
   ['9F1E'] = {"Interface Device (IFD) Serial Number" },
   ['9F1F'] = {"Track 1 Discretionary Data", ui_parse_printable }, 
   ['9F20'] = {"Track 2 Discretionary Data" },
   ['9F21'] = {"Transaction Time" },
   ['9F22'] = {"Certification Authority Public Key Index" },
   ['9F23'] = {"Upper Consecutive Offline Limit (Terminal Check)" }, 
   ['9F26'] = {"Application Cryptogram (AC)" }, 
   ['9F27'] = {"Cryptogram Information Data" }, 
   ['9F2D'] = {"ICC PIN Encipherment Public Key Certificate" }, 
   ['9F2E'] = {"ICC PIN Encipherment Public Key Exponent" }, 
   ['9F2F'] = {"ICC PIN Encipherment Public Key Remainder" },
   ['9F32'] = {"Issuer PK Exponent" }, 
   ['9F33'] = {"Terminal Capabilities" },
   ['9F34'] = {"Cardholder Verification Method (CVM) Results" },
   ['9F35'] = {"Terminal Type" },
   ['9F36'] = {"Application Transaction Counter (ATC)", ui_parse_number }, 
   ['9F37'] = {"Unpredictable number" }, 
   ['9F38'] = {"Processing Options Data Object List (PDOL)", ui_parse_DOL }, 
   ['9F39'] = {"Point-of-Service (POS) Entry Mode" },
   ['9F3A'] = {"Amount, Reference Currency" },
   ['9F3B'] = {"Application Reference Currency" },
   ['9F3C'] = {"Transaction Reference Currency Code" },
   ['9F3D'] = {"Transaction Reference Currency Exponent" },
   ['9F40'] = {"Additional Terminal Capabilities" },
   ['9F41'] = {"Transaction Sequence Counter" },
   ['9F42'] = {"Application Currency Code", ui_parse_currency_code }, 
   ['9F43'] = {"Application Reference Currency Exponent" },
   ['9F44'] = {"Application Currency Exponent" }, 
   ['9F45'] = {"Data Authentication Code" }, 
   ['9F46'] = {"ICC Public Key Certificate" }, 
   ['9F47'] = {"ICC Public Key Exponent" }, 
   ['9F48'] = {"ICC Public Key Remainder" }, 
   ['9F49'] = {"Dynamic Data Authentication Data Object List (DDOL)", ui_parse_DOL },
   ['9F4A'] = {"Static Data Authentication Tag List", ui_parse_TL }, 
   ['9F4B'] = {"Signed Dynamic Application Data" }, 
   ['9F4C'] = {"Integrated Circuit Card (ICC) Dynamic Number" },
   ['9F4D'] = {"Log Entry" },
   ['9F4E'] = {"Merchant Name and Location" },
   ['9F4F'] = {"Log Format", ui_parse_DOL },
   ['9F51'] = {"Application Currency Code", ui_parse_currency_code },
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
   ['9F76'] = {"Secondary Application Currency Code", ui_parse_currency_code }, 
   ['9F77'] = {"VLP Funds Limit" }, 
   ['9F78'] = {"VLP Single Transaction Limit" }, 
   ['9F79'] = {"VLP Available Funds" }, 
   ['9F7F'] = {"Card Production Life Cycle (CPLC) History File Identifiers" }, 
   ['BF0C'] = {"FCI Issuer Discretionary Data" }
}

function emv_parse(cardenv,tlv)
	return tlv_parse(cardenv,tlv,EMV_REFERENCE)
end

PPSE = "#325041592E5359532E4444463031"
PSE  = "#315041592E5359532E4444463031"

-- AID_LIST enhanced with information from Wikipedia
-- http://en.wikipedia.org/wiki/EMV

AID_LIST = { 
  "#A0000000031010",        -- Visa credit or debit
  "#A000000003101001",      -- VISA Credit
  "#A000000003101002",      -- VISA Debit
  "#A0000000032010",        -- Visa electron
  "#A0000000032020",        -- V pay
  "#A0000000033010",        -- VISA Interlink	
  "#A0000000034010",        -- VISA Specific	
  "#A0000000035010",        -- VISA Specific	
  "#A0000000036010",        -- Domestic Visa Cash Stored Value
  "#A0000000036020",        -- International Visa Cash Stored Value
  "#A0000000038002",        -- Barclays/HBOS		
  "#A0000000038010",        -- Visa Plus
  "#A0000000039010",        -- VISA Loyalty
  "#A000000003999910",      -- VISA ATM	
  "#A000000004",            -- US Debit (MC)
  "#A0000000041010",        -- Mastercard credit or debit 
  "#A00000000410101213",    -- MasterCard
  "#A00000000410101215",    -- MasterCard
  "#A000000004110101213",   -- Mastercard Credit
  "#A0000000042010",        -- MasterCard Specific
  "#A0000000043010",        -- MasterCard Specific
  "#A0000000043060",        -- Mastercard Maestro
  "#A0000000044010",        -- MasterCard Specific
  "#A0000000045010",        -- MasterCard Specific
  "#A0000000046000",        -- Mastercard Cirrus 
  "#A0000000048002",        -- NatWest or SecureCode Auth
  "#A0000000049999",        -- Mastercard
  "#A0000000050001",        -- UK Domestic Maestro - Switch (debit card) Maestro
  "#A0000000050002",        -- UK Domestic Maestro - Switch (debit card) Solo
  "#A0000000250000",        -- American Express (Credit/Debit)
  "#A00000002501",          -- American Express
  "#A000000025010402",      -- American Express
  "#A000000025010701",      -- American Express ExpressPay
  "#A000000025010801",      -- American Express
  "#A0000000291010",        -- ATM card LINK (UK) ATM network
  "#A0000000421010",        -- French CB
  "#A0000000422010",        -- French CB
  "#A00000006510",          -- JCB
  "#A0000000651010",        -- JCB
  "#A00000006900",          -- FR Moneo
  "#A000000098",            -- US Debit (Visa)
  "#A0000000980848",        -- Schwab Bank Debit Card
  "#A0000001211010",        -- Dankort Danish domestic debit card
  "#A0000001410001",        -- Pagobancomat Italian domestic debit card	
  "#A000000152",            -- US Debit (Discover)
  "#A0000001523010",        -- Diners club/Discover
  "#A0000001544442",        -- Banricompras Debito (Brazil)
  "#A0000001850002",        -- UK Post Office Card Account card
  "#A0000002281010",        -- SAMA Saudi Arabia domestic credit/debit card
  "#A0000002282010",        -- SAMA Saudi Arabia domestic credit/debit card
  "#A0000002771010",        -- Interac
  "#A0000003156020",        -- Chipknip
  "#A0000003241010",        -- Discover
  "#A0000003591010028001",  -- ZKA Girocard (Germany)
  "#A0000003710001",        -- InterSwitch Verve Card (Nigeria)
  "#A0000004540010",        -- Etranzact Genesis Card (Nigeria)
  "#A0000004540011",        -- Etranzact Genesis Card 2 (Nigeria)
  "#A0000004766C",          -- Google
  "#A0000005241010",        -- RuPay (India)
  "#A000000620",            -- US Debit (DNA)
  "#D27600002545500100",    -- ZKA Girocard (Germany)
  "#D5780000021010",        -- BankAxept Norwegian domestic debit card
}

EXTRA_DATA = { 0x9F36, 0x9F13, 0x9F17, 0x9F4D, 0x9F4F }


function emv_process_ppse(cardenv)
	local sw, resp
	local APP
	local dirent

	sw, resp = card.select(PPSE)

	if sw ~= 0x9000 then
     	   log.print(log.INFO,"No PPSE")
	   return false
	end

	-- Construct tree
	APP = nodes.append(cardenv, {classname="application", label="application", id=PPSE})
	emv_parse(APP,resp)

	AID_LIST = {}

	for dirent in nodes.find(APP,{ id="4F" }) do
	    aid = tostring(nodes.get_attribute(dirent,"val"))
		log.print(log.INFO,"PPSE contains application #" .. aid)
	    table.insert(AID_LIST,"#"..aid)
	end
	return true

end

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
	local tag4F

        sw, resp = card.select(PSE)
	
	-- Could it be a french card?
	if sw == 0x6E00 then 
	   card.warm_reset()
	   warm_atr = card.last_atr()
	   nodes.append(cardenv, {classname="atr", label="ATR", id="warm", size=#warm_atr, val=warm_atr})
           sw, resp = card.select(PSE)
	end

	-- could it be a contactless smartcard?
	if sw == 0x6A82 then
	   return emv_process_ppse(cardenv)
	end

	if sw ~= 0x9000 then
        log.print(log.INFO,"No PSE")
	    return false
	end

	-- Construct tree
	APP = nodes.append(cardenv, {classname="application", label="application", id=PSE})
	emv_parse(APP,resp)

	ref = nodes.find_first(APP,{id="88"})
	if (ref) then
        sfi = nodes.get_attribute(ref,"val")
	    FILE = nodes.append(APP,{classname="file", label="file", id=sfi[0]})

	    rec = 1
	    AID_LIST = {}
	    repeat 
            sw,resp = card.read_record(sfi:get(0),rec)
	        if sw == 0x9000 then
	            RECORD = nodes.append(FILE, {classname="record", label="record", id=tostring(rec)})
	            emv_parse(RECORD,resp)
                for tag4F in nodes.find(RECORD,{id="4F"}) do
	                aid = tostring(nodes.get_attribute(tag4F,"val"))
		            log.print(log.INFO,"PSE contains application #" .. aid)
	                table.insert(AID_LIST,"#"..aid)
		        end
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
	local log_recno

	LOG_FILE   = nodes.append(application,{ classname="file", 
						    label="file",
						    id=tostring(log_sfi)})

	LOG_DATA   = nodes.append(LOG_FILE, { classname="item", 
						  label = "log data"})
 	
	log.print(log.INFO,string.format("Reading LOG SFI %i",log_sfi))
	for log_recno =1,log_max do
           sw,resp = card.read_record(log_sfi, log_recno)
	   if sw ~= 0x9000 then
                  log.print(log.WARNING,"Read log record failed")
		  break
	   else
	          nodes.append(LOG_DATA,{ classname = "record", 
					      label="record",
					      id=log_recno,
					      size=#resp,
					      val=resp})
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

	LOG_FILE   = nodes.append(application,     { classname="file", label="log file", id=log_sfi})
	LOG_FORMAT = nodes.append(LOG_FILE,	   { classname="block", label="log format"})
 	
	log_format = application:find_first({id = "9F4F"}):get_attribute("val")

	i = 1
	item = ""
	nodes.set_attribute(LOG_FORMAT,"val",log_format);
	while log_format 
	do
	    tag, log_format = asn1.split_tag(log_format)
	    len, log_format = asn1.split_length(log_format)
	    tag_name, tag_func = tlv_tag_info(tag,EMV_REFERENCE,0);
	    log_items[i]= { tag, len, tag_name, tag_func }
	    i = i+1
	    item = item .. string.format("%X[%d] ", tag, len);
	end
	nodes.set_attribute(LOG_FORMAT,"alt",item);

	log.print(log.INFO,string.format("Reading LOG SFI %i",log_sfi))
	for log_recno =1,log_max do
           sw,resp = card.read_record(log_sfi,log_recno)
	   if sw ~= 0x9000 then
                  log.print(log.WARNING,"Read log record failed")
		  break
	   else
	          REC = nodes.append(LOG_FILE,{classname="record", label="record", id=log_recno, size=#resp})
		  item_pos = 0
		  ITEM_AMOUNT = nil
		  currency_digits = 0
		  for i=1,#log_items do
		     if log_items[i][3] then
		        ITEM = nodes.append(REC,{ classname="item", 
						      label=log_items[i][3], 
						      size=log_items[i][2] })
		     else
		        ITEM = nodes.append(REC,{ classname="item", 
						      label=string.format("tag %X",log_items[i][1]),
						      size=log_items[i][2] })
		     end

		     data = bytes.sub(resp,item_pos,item_pos+log_items[i][2]-1)
	             nodes.set_attribute(ITEM,"val",data)

		     if log_items[i][1]==0x9F02 then
		        ITEM_AMOUNT=ITEM
		     elseif log_items[i][1]==0x5F2A then
	                currency_code   = tonumber(tostring(data))
		        currency_name   = iso_currency_code_name(currency_code)
	                currency_digits = iso_currency_code_digits(currency_code)
	                nodes.set_attribute(ITEM,"alt",currency_name)
		     elseif log_items[i][4] then
		        log_items[i][4](ITEM,data)
                     end
	             item_pos = item_pos + log_items[i][2]
	          end

	          if ITEM_AMOUNT then
		     local amount = tostring(nodes.get_attribute(ITEM_AMOUNT,"val"))
		     nodes.set_attribute(ITEM_AMOUNT,"alt",string.format("%.2f",amount/(10^currency_digits)))
		  end
	   end
	end 

end

function emv_process_application(cardenv,aid)
	local sw, resp
	local ref
	local ref2
	local pdol
	local AFL = bytes.new(8);
	local extra
	local j -- counter
	local tag_name
	local tag_func
	local AIP

	log.print(log.INFO,"Processing application "..aid)

	-- Select AID
	sw,resp = card.select(aid)
	if sw ~=0x9000 then
	   return false
	end

	-- Process 'File Control Infomation' and get PDOL
	local APP
	local FCI
	
	APP = nodes.append(cardenv, { classname = "application", label = "application", id=aid })
	FCI = nodes.append(APP, { classname = "header", label = "answer to select", size=#resp, val=resp })
	emv_parse(FCI,resp)
	ref = nodes.find_first(FCI,{id="9F38"})
	if (ref) then
	   pdol = nodes.get_attribute(ref,"val")
	else
	   pdol = nil;
	end

	-- INITIATE get processing options, now that we have pdol
	local GPO
	local query

	query = "Issue a GET PROCESSING OPTIONS command?";
	ref = nodes.find_first(FCI,{id="50"})
	if (ref) then
	   query = "Issue a GET PROCESSING OPTIONS command to the "..nodes.get_attribute(ref,"alt"):match( "^%s*(.-)%s*$").." application?"
	end

        if ui.question(query,{"Yes","No"})==1 then
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
           end
           if sw ~=0x9000 then
                log.print(log.ERROR,"GPO Failed")
		ui.question("GET PROCESSING OPTIONS failed, the script will continue to read the card",{"OK"})
           else
 	   	GPO = nodes.append(APP,{classname="block",label="processing_options", size=#resp, val=resp})
		emv_parse(GPO,resp)
                ref = nodes.find_first(GPO,{id="94"})
                ref2 = nodes.find_first(GPO,{id="80"})
		if ref then
		    AFL = nodes.get_attribute(ref, "val")
		elseif ref2 then
		    -- Parse a Format 1 GET PROCESSING OPTIONS response message.
		    AFL = nodes.get_attribute(ref2, "val")
		    AIP = bytes.sub(AFL,0,1)
		    AFL = bytes.sub(AFL,2)
            if nodes.find_first(GPO,{id="82"}) then 
		        tag_value_parse(ref2, 0x82, AIP, EMV_REFERENCE)
	        end
	        if  nodes.find_first(GPO,{id="94"}) then 			
		        tag_value_parse(ref2, 0x94, AFL, EMV_REFERENCE)
	        end 
	        else
	            log.print(log.WARNING,
		              "GPO Response message contains neither a Format 1 nor a Format 2 Data Field")
		end
	   end
	end

	-- Read extra data 
	--extra = nodes.append(APP,{classname="block",label="extra emv data"})
	for j=1,#EXTRA_DATA do
	    sw,resp = card.get_data(EXTRA_DATA[j])
	    if sw == 0x9000 then
	       emv_parse(APP,resp):set_attribute("classname","block")
	    end
	end
	
	-- find LOG INFO 
	local LOG
	local logformat
	
	logformat=nil
	ref = nodes.find_first(APP,{id="9F4D"})
	-- I've seen this on some cards :
	if ref==nil then
	   -- proprietary style ?
	   ref = nodes.find_first(APP,{id="DF60"})
	   logformat = "VISA"
	else
	   logformat = "EMV"
	end

	if ref then
           LOG = nodes.get_attribute(ref,"val")
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


	local sfi_index
	local rec_index
	local max_rec_index
	local SFI
	local REC
	local i

	-- Process Application File Locator
	local AFL_info = {}
	for i=0,#AFL-1,4 do 
	    sfi_index=bit.SHR(AFL:get(i),3)
	    if AFL_info[sfi_index] == nil then
	       AFL_info[sfi_index] = {['max'] = AFL:get(i+2)}
	    else
	       AFL_info[sfi_index]['max'] = math.max(AFL:get(i+2), AFL_info[sfi_index]['max'])
	    end
	    for j=AFL:get(i+1),AFL:get(i+2) do
	        -- Include in offline data authentication.
	        AFL_info[sfi_index][j] = AFL_info[sfi_index][j] or (j < AFL:get(i+1)+AFL:get(i+3))
	    end
	end

	    
        for sfi_index=1,31 do
		SFI = nil
		max_rec_index = 5
		-- Amex has files where records don't start at 1
		-- The highest starting number I've seen is 4, 
		-- so don't treat a failure of a lower-than-five record number as "end of records in a file"
		if AFL_info[sfi_index] then
		   max_rec_index = math.max(max_rec_index, AFL_info[sfi_index]['max'])
		end

		if (logformat==nil or LOG:get(0)~=sfi_index) then

	    		for rec_index=1,255 do
				log.print(log.INFO,string.format("Reading SFI %i, record %i",sfi_index, rec_index))
			        sw,resp = card.read_record(sfi_index,rec_index)
		                if sw ~= 0x9000 then
	                                  log.print(log.WARNING,string.format("Read record failed for SFI %i, record %i",sfi_index,rec_index))

                                    if rec_index > max_rec_index then
					    break
				    end
			        else
				    if (SFI==nil) then
	   		    		    SFI = nodes.append(APP,{classname="file", label="file",	id=sfi_index})
				    end
			            REC = nodes.append(SFI,{classname="record", label="record", id=rec_index})
				    if (AFL_info[sfi_index] ~= nil) and (AFL_info[sfi_index][rec_index] == true) then
					    nodes.append(REC,{classname="item", label="AFL Information", val="This record is to be included in offline data authentication"})
				    end
			            if (emv_parse(REC,resp)==false) then
					    nodes.set_attribute(REC,"val",resp)
					    nodes.set_attribute(REC,"size",#resp)
				    end
		        	end
	
		        end -- for rec_index

		end -- if log exists

            end -- for sfi_index

	-- Read logs if they exist
	if logformat=="EMV" then
           emv_process_application_logs(APP,LOG:get(0),LOG:get(1))
	elseif logformat=="VISA" then
	   visa_process_application_logs(APP,LOG:get(0),LOG:get(1))
	end


	return true
end

DAYCOUNT_DATA =
{
  [0] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
  [1] = {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366}
}

function yearday_to_day_and_month(yday, leap)
-- yday = 1, ..., 366 (day of year)
-- leap = 0 (non-leap year) or 1 (leap year)
    if (yday < 1) or (yday - leap > 365) then
        return nil, nil;
    end

    local month
    month = math.floor(yday/30)
    if yday > DAYCOUNT_DATA[leap][month + 1] then
        month = month + 1
    end
    return yday - DAYCOUNT_DATA[leap][month], month
end

function ui_parse_cplc_date(node,data)
    local date = tostring(data)
    local day
    local month
    local year
    local yday

    if string.len(date) ~= 4 then
        return
    end

    year = 0 + string.sub(date, 1, 1)
    yday = 0 + string.sub(date, 2, 4)

    if yday == 366 then
        date = "???" .. year .. "-12-31"
	nodes.set_attribute(node, "alt", date)
	return
    end

    day, month = yearday_to_day_and_month(yday, 0)
    if day == nil then
        return
    end

    date = string.format("???%d-%02d-%02d", year, month, day)

    if (year % 2 == 0) and (yday >= 60) then
        day, month = yearday_to_day_and_month(yday, 1)
        date = date .. string.format(" or ???%d-%02d-%02d", year, month, day)
    end

    nodes.set_attribute(node, "alt", date)
end

CPLC_DATA =
{
  { "IC Fabricator", 2 } ,
  { "IC Type", 2 },
  { "Operating System Provider Identifier", 2 },
  { "Operating System Release Date", 2, ui_parse_cplc_date },
  { "Operating System Release Level", 2 },
  { "IC Fabrication Date", 2, ui_parse_cplc_date },
  { "IC Serial Number", 4 },
  { "IC Batch Identifier", 2 },
  { "IC ModuleFabricator", 2 },
  { "IC ModulePackaging Date", 2, ui_parse_cplc_date },
  { "ICC Manufacturer", 2 },
  { "IC Embedding Date", 2, ui_parse_cplc_date },
  { "Prepersonalizer Identifier", 2 },
  { "Prepersonalization Date", 2, ui_parse_cplc_date },
  { "Prepersonalization Equipment", 4 },
  { "Personalizer Identifier", 2 },
  { "Personalization Date", 2, ui_parse_cplc_date },
  { "Personalization Equipment", 4 },
}

function emv_process_cplc(cardenv)
	local sw, resp
	local CPLC
	local cplc_data
	local cplc_tag
	local i
	local pos
	local ref2
	local tlv_value
	local tlv_ui_func

	log.print(log.INFO,"Processing CPLC data")
	sw,resp = card.get_data(0x9F7F)
	if sw == 0x9000 then
	   cplc_tag, cplc_data = asn1.split(resp)
	   CPLC      = nodes.append(cardenv,{classname="block", label="cpcl data", id="9F7F", size=#cplc_data})
	   nodes.set_attribute(CPLC,"val",cplc_data) 
	   pos = 0
	   for i=1,#CPLC_DATA do
	       ref2 = nodes.append(CPLC,{classname="item", label=CPLC_DATA[i][1], id=pos});
	       tlv_value = bytes.sub(cplc_data,pos,pos+CPLC_DATA[i][2]-1)
               nodes.set_attribute(ref2,"val",tlv_value)
	       tlv_ui_func = CPLC_DATA[i][3]
	       if tlv_ui_func then
	           tlv_ui_func(ref2,tlv_value)
               end
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
