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

-- HELPER LIBRARY : implements some TLV processing

function tlv_tag_msb(tag)
	while (tag>255) do tag = tag / 256 end
	return tag
end

function tlv_tag_is_compound(tag)
        return (bit.AND(tlv_tag_msb(tag),0x20)==0x20)
end

ISO_7816_IDO = {
   ['2'] = "Integer",
   ['3'] = "Bit String",
   ['4'] = "Octet String",
   ['5'] = "Null",
   ['6'] = "Object identifier",
   ['13'] = "Printable String",
   ['14'] = "T61 String",
   ['16'] = "IA5 String",
   ['17'] = "UTC Time",
   ['30'] = "Sequence",
   ['31'] = "Set",
   ['41'] = "Country authority",
   ['42'] = "Issuer authority",
   ['43'] = "Card service data",
   ['44'] = "Initial access data",
   ['45'] = "Card issuer's data",
   ['46'] = "Pre-issuing data",
   ['47'] = "Card capabilities",
   ['48'] = "Status information",
   ['4F'] = "Application Identifier (AID)",
   ['50'] = "Application Label",
   ['51'] = "Path",
   ['52'] = "Command to perform",
   ['53'] = "Discretionary data",
   ['56'] = "Track 1 Equivalent Data",
   ['57'] = "Track 2 Equivalent Data",
   ['58'] = "Track 3 Equivalent Data",
   ['59'] = "Card expiration date",
   ['5A'] = "Application PAN",
   ['5B'] = "Name",
   ['5C'] = "Tag list",
   ['5D'] = "Tag length list",
   ['5E'] = "Log-in data",
   ['5F20'] = "Cardholder Name",
   ['5F21'] = "Track 1 Data",
   ['5F22'] = "Track 2 Data",
   ['5F23'] = "Track 3 Data",
   ['5F24'] = "Application Expiration Date",
   ['5F25'] = "Application Effective Date",
   ['5F26'] = "Card Effective Date",
   ['5F27'] = "Interchange control",
   ['5F28'] = "Country Code",
   ['5F29'] = "Interchange profile",
   ['5F2A'] = "Currency code",
   ['5F2B'] = "Date of birth",
   ['5F2C'] = "Cardholder nationality",
   ['5F2D'] = "Language Preference",
   ['5F2E'] = "Cardholder biometric data",
   ['5F2F'] = "Pin usage policy",
   ['5F30'] = "Service Code",
   ['5F32'] = "Transaction counter",
   ['5F34'] = "Application PAN Sequence Number",
   -- MISSING DATA HERE
   ['5F50'] = "Issuer URL",
   ['62'] = "File Control Parameters (FCP) Template",
   ['62/80'] = "Data size excluding structural information",
   ['62/81'] = "Data size including structural information",
   ['62/82'] = "File descriptor",
   ['62/83'] = "File identifier",
   ['62/84'] = "Dedicated File (DF) Name",
   ['62/85'] = "Proprietary information",
   ['62/86'] = "Security attributes",
   ['62/87'] = "EF containing FCI extension",
   ['63'] = "wrapper",
   ['64'] = "File Management Data (FMD) Template",
   -- MISSING DATA HERE
   ['6F'] = "File Control Information (FCI) Template",
   ['6F/80'] = "Data size excluding structural information",
   ['6F/81'] = "Data size including structural information",
   ['6F/82'] = "File descriptor",
   ['6F/83'] = "File identifier",
   ['6F/84'] = "Dedicated File (DF) Name",
   ['6F/85'] = "Proprietary information",
   ['6F/86'] = "Security attributes",
   ['6F/87'] = "EF containing FCI extension",
   ['72'] = "Issuer Script Template 2",
   ['73'] = "Discretionary DOs",
   ['A5'] = "FCI Proprietary Template",
}

function tlv_tag_name(tlv_tag,reference,parent)
	local tlv_ref 

	tlv_ref = reference[string.format("%X/%X",parent,tlv_tag)]
	if tlv_ref==nil then
	   tlv_ref = reference[string.format("%X",tlv_tag)]
	   if tlv_ref==nil then
	      tlv_ref = ISO_7816_IDO[string.format("%X/%X",parent,tlv_tag)]
	      if tlv_ref==nil then
	         tlv_ref = ISO_7816_IDO[string.format("%X",tlv_tag)]
	      end
	   end
	end
	return tlv_ref
end

TLV_TYPES = {
  "TLV (Universal)",
  "TLV (Application-specific)",
  "TLV (Context-specific)",
  "TLV (Private)"
}

function internal_tlv_parse(cardenv,tlv,reference,parent)
        local ref
        local tlv_tag
	local tlv_value
	local tlv_tail
        local text
        local tlv_ref

	while tlv do
            tlv_tag, tlv_value, tlv_tail = asn1.split(tlv)
	    
	    if tlv_tag==nil or tlv_tag==0 then 
	       break
	    end--if

            tlv_ref = tlv_tag_name(tlv_tag,reference,parent)
	    
	    if (bytes.is_printable(tlv_value) or tlv_tag==0x50) and (#tlv_value>1) then
               text = "'"..bytes.toprintable(tlv_value).."'"
	    else
	       text = nil
	    end--if
 
	    if tlv_ref==nil then
	       tlv_ref = TLV_TYPES[bit.SHR(tlv_tag_msb(tlv_tag),6)+1]
	    end--if

	    ref = ui.tree_add_node(cardenv,tostring(tlv_ref),string.format('%X',tlv_tag),
                                      #tlv_value,text)

            if (tlv_tag_is_compound(tlv_tag)) then
               internal_tlv_parse(ref,tlv_value,reference,tlv_tag)
            else
               ui.tree_set_value(ref,tostring(tlv_value))
            end--if
	    tlv = tlv_tail
        end--while
end

function tlv_parse(cardenv,tlv,reference)
	if reference == nil then
	   reference = {}
	end
	internal_tlv_parse(cardenv,tlv,reference,0)
end

