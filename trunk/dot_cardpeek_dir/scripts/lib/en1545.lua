--
-- This file is part of Cardpeek, the smartcard reader utility.
--
-- Copyright 2009-2010 by 'L1L1'
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

function en1545_parse_item(ctx, format, data, position, reference_index)
        local parsed = 0
        local index
        local NODE
        local BITMAP_NODE
        local bitmap_size
        local hex_info
        local item

        if format == nil then
           return 0
        end

        parsed = format[2]
        item = bytes.sub(data,position,position+parsed-1)
        NODE = ui.tree_add_node(ctx,format[3],reference_index)

        if format[1] == en1545_BITMAP then
           bitmap_size = parsed
           parsed = bitmap_size
           BITMAP_NODE = ui.tree_add_node(NODE,"("..format[3].."Bitmap)",nil,parsed)
           ui.tree_set_value(BITMAP_NODE,item);
           for index=0,format[2]-1 do
               if data[position+bitmap_size-index-1]~=0 then
                  parsed = parsed + en1545_parse_item(NODE, format[4][index], data, position+parsed, index)
               end
           end
        elseif format[1] == en1545_REPEAT then
           ui.tree_set_value(NODE,item);
           ui.tree_set_alt_value(NODE,bytes.tonumber(item))
           for index=1,bytes.tonumber(item) do
               parsed = parsed + en1545_parse_item(ctx, format[4][0], data, position+parsed, reference_index+index)
           end
        else
           ui.tree_set_value(NODE,item);
           ui.tree_set_alt_value(NODE,format[1](item))
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
        local NODE
        if bytes.tonumber(data)>0 then
                NODE = ui.tree_add_node(ctx,"(remaining unparsed data)");
                ui.tree_set_value(NODE,data)
        end
end

function en1545_map(ctx, data_type, ...)
        local RECORD_REF
        local DATA_REF
        local RECORD_REF
        local NODE_REF
        local data
        local bits
        local index
        local i
        local parsed
        local block

        while true do
                NODE_REF = ui.tree_find_node(ctx,data_type)
                if NODE_REF==nil then break end
                for index=1,15 do
                        RECORD_REF=ui.tree_find_node(NODE_REF,"record",index)
                        if RECORD_REF==nil then break end
                        DATA_REF = ui.tree_find_node(RECORD_REF,"raw data")
                        data = ui.tree_get_value(DATA_REF)
                        bits = bytes.convert(1,data)
                        DATA_REF = ui.tree_add_node(RECORD_REF,"parsed data")
                        parsed = 0
                        for i,template in ipairs({...}) do
                                block = bytes.sub(bits,parsed)
                                if bytes.is_all(block,0)==false then
                                        parsed = parsed + en1545_parse(DATA_REF,template,block)
                                end
                        end
                        en1545_unparsed(DATA_REF,bytes.sub(bits,parsed))
                end
                ui.tree_set_attributes(NODE_REF,"name",data_type..", parsed")
        end
end

function en1545_guess_network(APP)
        local country_bin
	local network_bin
        local ENV_REF
        local RECORD_REF
        local DATA_REF
        local data

        ENV_REF    = ui.tree_find_node(APP,"Environment")

        if ENV_REF then
                RECORD_REF = ui.tree_find_node(ENV_REF,"record",1)
                DATA_REF   = ui.tree_find_node(RECORD_REF,"raw data")
                if DATA_REF then
                        data = bytes.convert(1,ui.tree_get_value(DATA_REF))
			country_bin = bytes.sub(data,13,24)
			network_bin = bytes.sub(data,25,36)
			print(bytes.convert(4,country_bin),bytes.convert(4,network_bin))			
			return tonumber(bytes.format("%D",bytes.convert(4,country_bin))),tonumber(bytes.format("%D",bytes.convert(4,network_bin)))
                else
                        log.print(log.WARNING,"Could not find data in 'Environement/record 1/'")
                end
	else
		log.print(log.WARNING,"Could not find data in 'Environement'")
        end
	return false
end

