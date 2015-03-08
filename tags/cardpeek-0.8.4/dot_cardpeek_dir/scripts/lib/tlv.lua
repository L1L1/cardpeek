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

-- HELPER LIBRARY : implements some TLV processing

require("lib.currency_codes")
require("lib.country_codes")

function tlv_tag_msb(tag)
	while (tag>255) do tag = tag / 256 end
	return tag
end

function tlv_tag_is_compound(tag)
        return (bit.AND(tlv_tag_msb(tag),0x20)==0x20)
end

-- UI_PARSE_* FUNCTIONS : automated parsing of tlv values
--
-- All UI_PARSE functions must take 2 parameters: a tree node and
-- a bytestring. These function return true upon success and false
-- otherwise.

function ui_parse_digits(node,data)
	nodes.set_attribute(node,"val",data)
	nodes.set_attribute(node,"alt",bytes.format(data,"%D"))
end

function ui_parse_number(node,data)
	nodes.set_attribute(node,"val",data)
	nodes.set_attribute(node,"alt",bytes.tonumber(data))
end

function ui_parse_YYMMDD(node,data)
	local d = tostring(data)
	local d_table = {
	  ['year']=1900+string.sub(d,1,2),
	  ['month']=0+string.sub(d,3,4),
	  ['day']=0+string.sub(d,5,6)}
	local t

	if d_table['year']<1980 then
	   d_table['year']=d_table['year']+100
	end
	t = os.time(d_table)

	nodes.set_attribute(node,"val",data)
	nodes.set_attribute(node,"alt",os.date("%x",t))
end

function ui_parse_country_code(node,data)
        local cc_name = iso_country_code_name(tonumber(tostring(data)))

	nodes.set_attribute(node,"val",data)
	nodes.set_attribute(node,"alt",cc_name)
	return true
end

function ui_parse_currency_code(node,data)
        local cc_name = iso_currency_code_name(tonumber(tostring(data)))

	nodes.set_attribute(node,"val",data)
	nodes.set_attribute(node,"alt",cc_name)
	return true
end

function ui_parse_printable(node,data)
	nodes.set_attribute(node,"val",data)

	if #data==0 then
		return false
	end

	if bytes.is_printable(data) then
		nodes.set_attribute(node,"alt",bytes.format(data,"%P"))
	else 
		nodes.set_attribute(node,"alt",bytes.format(data,"%P (%D)"))
	end
	return true
end


function ui_parse_oid(node,data)
	local ret
	local i,v
	local item

	ret = "{ "..math.floor(data:get(0)/40) .." ".. (data:get(0)%40)
	item = 0
	for i,v in data:ipairs() do
		item = item*128+bit.AND(v,0x7F)
		if v>0x80 then
			ret = ret .. " " .. item
			item = 0
		end
	end
	ret = ret .. " }"
	nodes.set_attribute(node,"val",data)
	nodes.set_attribute(node,"alt",ret)
	return true
end

ISO_7816_IDO = {
   ['2'] = {"Integer" },
   ['3'] = {"Bit String" },
   ['4'] = {"Octet String" },
   ['5'] = {"Null" },
   ['6'] = {"Object identifier", ui_parse_oid },
   ['13'] = {"Printable String", ui_parse_printable },
   ['14'] = {"T61 String" },
   ['16'] = {"IA5 String" },
   ['17'] = {"UTC Time", ui_parse_printable },
   ['30'] = {"Sequence" },
   ['31'] = {"Set" },
   ['41'] = {"Country authority" },
   ['42'] = {"Issuer authority" },
   ['43'] = {"Card service data" },
   ['44'] = {"Initial access data" },
   ['45'] = {"Card issuer's data" },
   ['46'] = {"Pre-issuing data" },
   ['47'] = {"Card capabilities" },
   ['48'] = {"Status information" },
   ['4F'] = {"Application Identifier (AID)" },
   ['50'] = {"Application Label", ui_parse_printable },
   ['51'] = {"Path" },
   ['52'] = {"Command to perform" },
   ['53'] = {"Discretionary data" },
   ['56'] = {"Track 1 Equivalent Data" },
   ['57'] = {"Track 2 Equivalent Data" },
   ['58'] = {"Track 3 Equivalent Data" },
   ['59'] = {"Card expiration date" },
   ['5A'] = {"Application PAN", ui_parse_digits },
   ['5B'] = {"Name", ui_parse_printable },
   ['5C'] = {"Tag list" },
   ['5D'] = {"Tag length list" },
   ['5E'] = {"Log-in data" },
   ['5F20'] = {"Cardholder Name", ui_parse_printable },
   ['5F21'] = {"Track 1 Data" },
   ['5F22'] = {"Track 2 Data" },
   ['5F23'] = {"Track 3 Data" },
   ['5F24'] = {"Application Expiration Date", ui_parse_YYMMDD },
   ['5F25'] = {"Application Effective Date", ui_parse_YYMMDD },
   ['5F26'] = {"Card Effective Date", ui_parse_YYMMDD },
   ['5F27'] = {"Interchange control" },
   ['5F28'] = {"Country Code", ui_parse_country_code },
   ['5F29'] = {"Interchange profile" },
   ['5F2A'] = {"Currency code", ui_parse_currency_code },
   ['5F2B'] = {"Date of birth" },
   ['5F2C'] = {"Cardholder nationality" },
   ['5F2D'] = {"Language Preference", ui_parse_printable },
   ['5F2E'] = {"Cardholder biometric data" },
   ['5F2F'] = {"Pin usage policy" },
   ['5F30'] = {"Service Code" },
   ['5F32'] = {"Transaction counter" },
   ['5F34'] = {"Application PAN Sequence Number" },
   -- MISSING DATA HERE
   ['5F50'] = {"Issuer URL" },
   ['62'] = {"File Control Parameters (FCP) Template" },
   ['62/80'] = {"Data size excluding structural information", ui_parse_number },
   ['62/81'] = {"Data size including structural information", ui_parse_number },
   ['62/82'] = {"File descriptor" },
   ['62/83'] = {"File identifier" },
   ['62/84'] = {"Dedicated File (DF) Name", ui_parse_printable },
   ['62/85'] = {"Proprietary information" },
   ['62/86'] = {"Security attributes" },
   ['62/87'] = {"EF containing FCI extension" },
   ['63'] = {"wrapper" },
   ['64'] = {"File Management Data (FMD) Template" },
   -- MISSING DATA HERE
   ['6F'] = {"File Control Information (FCI) Template" },
   ['6F/80'] = {"Data size excluding structural information", ui_parse_number },
   ['6F/81'] = {"Data size including structural information", ui_parse_number },
   ['6F/82'] = {"File descriptor" },
   ['6F/83'] = {"File identifier" },
   ['6F/84'] = {"Dedicated File (DF) Name", ui_parse_printable },
   ['6F/85'] = {"Proprietary information" },
   ['6F/86'] = {"Security attributes" },
   ['6F/87'] = {"EF containing FCI extension" },
   ['72'] = {"Issuer Script Template 2" },
   ['73'] = {"Discretionary DOs" },
   ['A5'] = {"FCI Proprietary Template" },
}

function tlv_tag_info(tlv_tag,reference,parent)
	local tlv_ref 

	tlv_ref = reference[string.format("%X/%X",parent,tlv_tag)]
	if tlv_ref==nil then
	   tlv_ref = reference[string.format("%X",tlv_tag)]
	   if tlv_ref==nil then
	      tlv_ref = ISO_7816_IDO[string.format("%X/%X",parent,tlv_tag)]
	      if tlv_ref==nil then
	         tlv_ref = ISO_7816_IDO[string.format("%X",tlv_tag)]
		 if tlv_ref==nil then
		    return nil,nil
		 end
	      end
	   end
	end
	return tlv_ref[1], tlv_ref[2]
end

TLV_TYPES = {
  "TLV (Universal)",
  "TLV (Application-specific)",
  "TLV (Context-specific)",
  "TLV (Private)"
}


function internal_tag_value_parse(cardenv,tlv_tag,tlv_value,reference,parent)
    local tlv_name
    local tlv_ui_func
    local ref

    tlv_name, tlv_ui_func = tlv_tag_info(tlv_tag,reference,parent)
	    
    if tlv_name==nil then
        tlv_name = TLV_TYPES[bit.SHR(tlv_tag_msb(tlv_tag),6)+1]
    end--if

    ref = nodes.append(cardenv,{ classname="item",
                                 label=tostring(tlv_name),
                                 id=string.format('%X',tlv_tag), 
                                 size=#tlv_value })

    if (tlv_tag_is_compound(tlv_tag)) then
        if (internal_tlv_parse(ref,tlv_value,reference,tlv_tag)==false) then
            nodes.remove(ref)
            return false
        end
        else
        if tlv_ui_func then
            tlv_ui_func(ref,tlv_value)
            else
            nodes.set_attribute(ref,"val",tlv_value)
        end--if
    end--if
    return ref
end

function internal_tlv_parse(cardenv,tlv,reference,parent)
    local ref = false
    local tlv_tag
    local tlv_value
    local tlv_tail

    while tlv do
        tlv_tag, tlv_value, tlv_tail = asn1.split(tlv)
    
        if tlv_tag==nil or tlv_tag==0 then 
           ref = nodes.append(cardenv,{ classname="card",
                                        label="Unparsed ASN1 data",
                                        val=tlv,
                                        alt="(error)", 
                                        size=#tlv })
           return ref
        end--if

        ref = internal_tag_value_parse(cardenv,tlv_tag,tlv_value,reference,parent)
        if ref==false then
            return false
        end

        tlv = tlv_tail
    end--while
    return ref
end

function tlv_parse(cardenv,tlv,reference)
	if reference == nil then
        reference = {}
	end
	return internal_tlv_parse(cardenv,tlv,reference,0)
end

function tag_value_parse(cardenv,tlv_tag,tlv_value,reference)
	if reference == nil then
        reference = {}
	end
	return internal_tag_value_parse(cardenv,tlv_tag,tlv_value,reference,0)
end

