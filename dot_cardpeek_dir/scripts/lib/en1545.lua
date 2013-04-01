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
require('lib.country_codes')

EPOCH = 852073200
date_days = 0
function en1545_DATE(source)
        local part =  bytes.sub(source, 0, 13) -- 14 bits
        bytes.pad_left(part,32,0)
        date_days = EPOCH+bytes.tonumber(part)*24*3600
        return os.date("%x",date_days)
end

function en1545_TIME(source)
        local date_minutes
        local part = bytes.sub(source, 0, 10) -- 11 bits
        bytes.pad_left(part,32,0)
        date_minutes = date_days + bytes.tonumber(part)*60
        date_days = 0
        return os.date("%X",date_minutes)
end

ALPHA = { "-","A","B","C","D","E","F","G",
          "H","I","J","K","L","M","N","O",
          "P","Q","R","S","T","U","V","W",
          "X","Y","Z","?","?","?","?"," " }

function en1545_ALPHA(source)
        local i
        local c
        local str = {""}

        for i=0,#source-4,5 do
                c=bytes.tonumber(bytes.sub(source,i,i+4))
                table.insert(str,ALPHA[c+1])
        end
        return table.concat(str)
end

function en1545_NETWORKID(source)
        local country = bytes.sub(source, 0, 11)
        local region  = bytes.sub(source, 12, 23)
        local country_code
        local region_code

        country_code = iso_country_code_name(tonumber(bytes.format("%D",bytes.convert(4,country))))
        region_code  = tonumber(bytes.format("%D",bytes.convert(4,region)))
        if region_code then
          return "country "..country_code.." / network "..region_code
        end
        return "country "..country_code
end

function en1545_NUMBER(source)
        return bytes.tonumber(source)
end
function en1545_AMOUNT(source)
        return string.format("%.2f euros",bytes.tonumber(source)/100)
end

function en1545_UNDEFINED(source)
        local hex_info = bytes.convert(8,source)
        return bytes.format("0x%D",hex_info)
end


en1545_BITMAP = 1
en1545_REPEAT = 2

--[[ 
	in en1545_parse_item, the "format" parameter is a table with entries containing 3 or 4 elements:
	 - format[1]: the type of the entry, which is either
			a) en1545_BITMAP: indicates a bitmap field (1 => field is present)
			b) en1545_REPEAT: indicates the field is repeated n times.
			c) en1545_XXXXXX: a function to call on the data for further processing.
	- format[2]: the length of the entry in bits.
	- format[3]: the name of the entry
	- format[4]: used only for en1545_BITMAP, points to a sub-table of entries.

--]]

function en1545_parse_item(ctx, format, data, position, reference_index)
        local parsed = 0
        local index
        local item_node
        local bitmap_node
        local bitmap_size
        local item

        if format == nil then
           return 0
        end

        parsed = format[2]

        item = bytes.sub(data,position,position+parsed-1)

	item_node = ctx:append{ classname="item", 
				label=format[3], 
				id=reference_index }

        if format[1] == en1545_BITMAP then

           bitmap_size = parsed
           parsed = bitmap_size
           item_node:append{ classname="item", 
			     label="("..format[3].."Bitmap)", 
			     val=item }

           for index=0,format[2]-1 do
               if data[position+bitmap_size-index-1]~=0 then
                  parsed = parsed + en1545_parse_item(item_node, format[4][index], data, position+parsed, index)
               end
           end

        elseif format[1] == en1545_REPEAT then

           item_node:val(item)
           item_node:alt(bytes.tonumber(item))

           for index=1,bytes.tonumber(item) do
               parsed = parsed + en1545_parse_item(ctx, format[4][0], data, position+parsed, reference_index+index)
           end

        else

           item_node:val(item)
           item_node:alt(format[1](item))

        end
        return parsed
end

function en1545_parse(ctx, format, data)
        local index
        local parsed = 0

        for index=0,#format do
            parsed = parsed + en1545_parse_item(ctx,format[index],data,parsed,index)
        end
        return parsed
end

function en1545_unparsed(ctx, data)
        if bytes.tonumber(data)>0 then
		ctx:append{ classname="item", 
			    label="(remaining unparsed data)", 
			    val=data }
        end
end

function en1545_map(cardenv, data_type, ...)
        local record_node
        local data
        local bits
        local index
        local i
        local parsed
        local block
	local foo

	files = cardenv:find(data_type)

	for foo,file in ipairs(files:toArray()) do

                if file==nil then return end

                for index=1,15 do
                        record_node = _n(file):find("record#"..index)

                        if record_node:length()==0 then break end

                        data = record_node:val()
                        bits = bytes.convert(1,data)
                        parsed = 0
                        for i,template in ipairs({...}) do
                                block = bytes.sub(bits,parsed)
                                if bytes.is_all(block,0)==false then
                                        parsed = parsed + en1545_parse(record_node,template,block)
                                end
                        end

                        en1545_unparsed(record_node,bytes.sub(bits,parsed))
                end

		_n(file):attr("label",data_type..", parsed")
	end
end


