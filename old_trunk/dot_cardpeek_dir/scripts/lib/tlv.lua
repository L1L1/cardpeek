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

function tlv_tag_is_compound(tag)
        while (tag>255) do tag = tag / 256 end
        return (bit_and(tag,0x20)==0x20)
end

function tlv_value_is_printable(data)
        local candidate = card.bytes_unpack(data)
        local index
        if #candidate<2 then
           return nil
        end
        for index=1,#candidate do
            if (candidate[index]<32) or (candidate[index]>126) then return nil end
        end
        return string.char(unpack(candidate))
end

ISO_7816_IDO = {
   ['6']  = "Object identifier",
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



function internal_tlv_parse(cardenv,tlv,reference,parent)
        local ref
        local atlv
        local text
        local len
        local tlv_ref


        atlv = card.tlv_split(tlv)
        for i=1,#atlv do
            tlv_ref = reference[string.format("%X/%X",parent,atlv[i][1])]
	    if tlv_ref==nil then
	       tlv_ref = reference[string.format("%X",atlv[i][1])]
	       if tlv_ref==nil then
	          tlv_ref = ISO_7816_IDO[string.format("%X/%X",parent,atlv[i][1])]
		  if tlv_ref==nil then
		      tlv_ref = ISO_7816_IDO[string.format("%X",atlv[i][1])]
		  end
	       end
	    end
            len = card.bytes_size(atlv[i][2])
            if tlv_ref then
               ref = ui.tree_append(cardenv,false,tlv_ref,string.format('%X',atlv[i][1]),len,nil)
            else
	       -- tlv_ref = ISO78
               ref = ui.tree_append(cardenv,false,"tlv",string.format('%X',atlv[i][1]),len,nil)
            end
            if (tlv_tag_is_compound(atlv[i][1])) then
               internal_tlv_parse(ref,atlv[i][2],reference,atlv[i][1])
            else
               text = tlv_value_is_printable(atlv[i][2])
               if text then
                 text = "'"..text.."'"
               end
               ui.tree_append(ref,true,atlv[i][2],nil,nil,text)
            end
        end
end

function tlv_parse(cardenv,tlv,reference)
	if reference == nil then
	   reference = {}
	end
	internal_tlv_parse(cardenv,tlv,reference,0)
end
