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

EPOCH = os.time({hour=0, min=0, year=1997, month=1, sec=0, day=1}) -- (localized)
local EPOCH_GMT = 852076800 -- (GMT)
local TIMEZONE_DELAY = EPOCH - EPOCH_GMT

local _l = {} -- Local functions

function en1545_DATE(source, ref_date)
    --[[
    @param source Number of days (in a bytes object).
    @param ref_date Refererence date for @source, expressed as a number of days since EPOCH in local time.
    If nil, 1997-01-01 is used.
    If negative, @source is a number of days UNTIL -ref_date.
    ]]
    ref_date = ref_date or EPOCH
    local date_days
    if ref_date < 0 then
        date_days = (-ref_date) - bytes.tonumber(source) * 24 * 3600
    else
        date_days = ref_date + bytes.tonumber(source) * 24 * 3600
    end
    return os.date("%a %x", date_days)
end

function en1545_TIME(source)
    --[[
    @param source Number of minutes (11 first bits of a bytes object).
    ]]
    local part = bytes.sub(source, 0, 10)
    part = bytes.pad_left(part, 32, 0)

    local date_seconds = TIMEZONE_DELAY + bytes.tonumber(part) * 60
    return os.date("%H:%M", date_seconds)
end

function en1545_QUARTERS(source)
    --[[
    @param source Number of quarters (7 first bits of a bytes object).
    Special values:
    - 96 (coding 24h00): end of service
    - 127 (all bits to 1): not significant
    ]]
    local part = bytes.sub(source, 0, 6)
    part = bytes.pad_left(part, 32, 0)
    if part == 96 then
        return "End of service"
    elseif part == 127 then
        return "Not significant"
    end

    local date_seconds = TIMEZONE_DELAY + bytes.tonumber(part) * 60 * 15
    return os.date("%H:%M", date_seconds)
end

function en1545_DATE_TIME(source, ref_date)
    --[[
    @param source Number of seconds (in a bytes object).
    @param ref_date Refererence date for @source, expressed as a number of days since EPOCH in local time.
    If nil, 1997-01-01 is used.
    If negative, @source is a number of days UNTIL -ref_date.
    ]]
    ref_date = ref_date or EPOCH
    local date_seconds
    if ref_date < 0 then
        date_seconds = (-ref_date) - bytes.tonumber(source)
    else
        date_seconds = ref_date + bytes.tonumber(source)
    end
    return os.date("%c", date_seconds)
end

function en1545_BCD_DATE(source)
	local dob = tostring(bytes.convert(source,8))
	local dobYear   = string.sub(dob, 1, 4)
	local dobMonth  = string.sub(dob, 5, 6)
	local dobDay    = string.sub(dob, 7, 8)
	local dateOfBirth = dobDay.."/"..dobMonth.."/"..dobYear
    return dateOfBirth
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

    country_code = iso_country_code_name(tonumber(country:convert(4):format("%D")))
    region_code  = tonumber(region:convert(4):format("%D"))
    if region_code then
        return "country "..country_code.." / network "..region_code
    end
    return "country "..country_code
end

function en1545_NUMBER(source)
    return bytes.tonumber(source)
end

function en1545_AMOUNT(source)
    return string.format("%.2f€",bytes.tonumber(source)/100)
end

function en1545_ZONES(source)
	local zones = bytes.tonumber(source)
	local n = 8
	local maxzone = 0
	local minzone
	while n >= 1 do
		if zones >= 2^(n-1) then
			if maxzone == 0 then
				maxzone = n
			end
			zones = zones - 2^(n-1)
			if zones == 0 then
				minzone = n
			end
		end
		n = n - 1
	end
	return string.format("%d-%d",minzone,maxzone)
end

function en1545_UNDEFINED(source)
    local hex_info = source:convert(8)
    return hex_info:format("0x%D")
end


en1545_BITMAP = 1
en1545_REPEAT = 2

function _l.parse_item(ctx, format, data, position, reference_index)
    --[[ 
	@param format Table with entries containing 3 or 4 elements:
	 - format[1]: the type of the entry, which is either
			a) en1545_BITMAP: indicates a bitmap field (1 => field is present)
			b) en1545_REPEAT: indicates the field is repeated n times.
			c) en1545_XXXXXX: a function to call on the data for further processing.
	- format[2]: the length of the entry in bits.
	- format[3]: the name of the entry
    - format[4]:
        - for en1545_BITMAP: points to a sub-table of entries.
        - for en1545_REPEAT: the sub-table to repeat.
        - for en1545_XXXXXX: optional argument for the callback function (in addition to the "source" argument).
    --]]

    local parsed = 0
    local index, item_node, bitmap_node, bitmap_size, item, alt

    if format == nil then
        return 0
    end

    parsed = format[2] -- entry length

    item = bytes.sub(data,position,position+parsed-1)

    if item == nil then
        return 0
    end

    item_node = ctx:append{ classname="item", 
            label=format[3], 
            --[[ id=reference_index --]] }

    if format[1] == en1545_BITMAP then -- entry type is bitmap 

        bitmap_size = parsed
        parsed = bitmap_size
        item_node:append{ classname="item", 
                label="("..format[3].."Bitmap)", 
                val=item }

        -- go through bit table in reverse order, since lsb=first bit 
        for index,bit in item:reverse():ipairs() do
            if bit==1 then
                parsed = parsed + _l.parse_item(item_node, format[4][index], data, position+parsed, index)
            end
        end

    elseif format[1] == en1545_REPEAT then -- entry type is repeat

        item_node:set_attribute("val",item)
        item_node:set_attribute("alt",bytes.tonumber(item))

        for index=1,bytes.tonumber(item) do
            parsed = parsed + _l.parse_item(ctx, format[4][0], data, position+parsed, reference_index+index)
        end

    else -- entry type is item

        -- call the callback function with the optional additional arguments
        if format[4] then
            if type(format[4]) == 'table' then
                alt = format[1](item, unpack(format[4]))
            else
                alt = format[1](item, format[4])
            end
        else
            alt = format[1](item)
        end

        -- update node
        if alt == nil then
            item_node:remove()
        else
            item_node:set_attribute("val",item)
            item_node:set_attribute("size",#item)
            item_node:set_attribute("alt",alt)
        end

    end
    return parsed
end

function _l.parse(ctx, format, data)
    local index
    local parsed = 0

    for index=0,#format do
        parsed = parsed + _l.parse_item(ctx,format[index],data,parsed,index)
    end
    return parsed
end

function _l.unparsed(ctx, data)
    if data and bytes.tonumber(data)>0 then
        ctx:append{ classname="item", 
                    label="(remaining unparsed data)", 
                    val=data,
                    alt="(binary data)" }
    end
end

function en1545_map(cardenv, data_type, ...)
    local record_node
    local bits
    local i
    local parsed
    local block

	for file in cardenv:find({label=data_type}) do
        for record_node in file:find({label="record"}) do
            if record_node==nil then
                break
            end

            bits = record_node:get_attribute("val"):convert(1)
            parsed = 0
            for i,template in ipairs({...}) do
                block = bytes.sub(bits,parsed)
                if bytes.is_all(block,0)==false then
                    parsed = parsed + _l.parse(record_node,template,block)
                end
            end
            
            _l.unparsed(record_node,bytes.sub(bits,parsed)) 
        end

		file:set_attribute("parsed","true")
	end
end
