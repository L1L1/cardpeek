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
require('lib.network_codes')

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
    local country_code = tonumber(country:convert(4):format("%D"))
    local region_code = tonumber(region:convert(4):format("%D"))

    local country_name = iso_country_code_name(country_code)
    if region_code then
        local region_name = network_code_name(country_code, region_code)
        return "country "..country_name.." / network "..region_name
    end
    return "country "..country_name
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

function _l.split_bitmap_subtable(subtable, bitmap_size, subtable_name)
    --[[
    Split sub-table according to the size of the bitmap.
    @return 2 arrays.
        a) head: part of subtable considered "always present". It is applicable (non empty) only if the size of subtable is greater than the size of bitmap.
        b) tail: part of subtable on which the bitmap applies. It has the same size as the bitmap.
    ]]
    subtable_name = subtable_name or ""

    -- List and sort all keys in the subtable
    local total_count = 0
    local keys = {}
    for key in pairs(subtable) do
            table.insert(keys, key)
            total_count = total_count + 1
    end
    table.sort(keys)

    -- Do the split
    local head_subtable = {}
    local bitmap_subtable = {}
    
    if total_count > bitmap_size then
        local head_size = total_count - bitmap_size
        local count = 0
        for _, key in ipairs(keys) do
            local value = subtable[key]
            count = count + 1
            if count <= head_size then
                head_subtable[count] = value
            else
                bitmap_subtable[count - head_size - 1] = value -- Bitmap subtable indices must start with 0
            end
        end
        if count ~= total_count then
            error("subtable "..subtable_name.." contains "..total_count.." elements, but only "..count.." where iterated using ipairs: please ensure subtable keys are successive integers");
        end
    else
        bitmap_subtable = subtable
    end
    
    return head_subtable, bitmap_subtable
end


en1545_BITMAP = 1
en1545_REPEAT = 2
en1545_BITMAP_BE = 3

function _l.parse_item(ctx, format, data, position)
    --[[ 
	@param format Table with entries containing 3 or 4 elements:
	 - format[1]: the type of the entry, which is either
        a) en1545_BITMAP: indicates a bitmap field (1 => field is present) in little-endian order (lowest bit byte = first bit)
        b) en1545_BITMAP_BE : same as en1545_BITMAP but in big-endian order (lowest significant bit = last bit)
        c) en1545_REPEAT: indicates the field is repeated n times.
        d) en1545_XXXXXX: a function to call on the data for further processing.
    - format[2]:
        * for en1545_BITMAP, en1545_BITMAP_BE or en1545_XXXXXX: the length of the entry in bits.
        * for en1545_REPEAT: not used.
	- format[3]: the name of the entry
    - format[4]:
        * for en1545_BITMAP or en1545_BITMAP_BE: points to a sub-table of entries.
        * for en1545_REPEAT: the sub-table to repeat.
        * for en1545_XXXXXX: optional argument for the callback function (in addition to the "source" argument).
    --]]

    local parsed = 0
    local index, item_node, bitmap_node, bitmap_size, item, alt

    if format == nil then
        return 0
    end

    parsed = format[2] -- entry length

    item = bytes.sub(data, position, position + parsed - 1)

    item_node = ctx:append{ classname="item", label=format[3] }

    -- ------------------------------------------------------------------------
    if format[1] == en1545_BITMAP or format[1] == en1545_BITMAP_BE then -- entry type is bitmap 

        bitmap_size = parsed
        parsed = bitmap_size

        if bitmap_size > 0 then
            -- Bitmap is not empty

            if item == nil then
				log.print(log.WARNING,"item is empty for non-empty bitmap "..format[3])
                return 0
            end

            item_node:append{ classname = "item", 
                    label = "("..format[3].."Bitmap)", 
                    size = #item,
                    val = item }

            local head_subtable
            local bitmap_subtable
            head_subtable, bitmap_subtable = _l.split_bitmap_subtable(format[4], bitmap_size, format[3].."Bitmap")

            for index, sub in pairs(head_subtable) do
                    parsed = parsed + _l.parse_item(item_node, sub, data, position + parsed)
            end

            local bitmap_ordered
            if format[1] == en1545_BITMAP_BE then
                bitmap_ordered = item
            else
                -- go through bit table in reverse order
                bitmap_ordered = item:reverse()
            end

            for index,bit in bitmap_ordered:ipairs() do
                if bit==1 then
                    parsed = parsed + _l.parse_item(item_node, bitmap_subtable[index], data, position + parsed)
                end
            end
        else
            -- Bitmap is empty: all fields are considered as accepted
            for index, sub in pairs(format[4]) do
                parsed = parsed + _l.parse_item(item_node, sub, data, position + parsed)
            end
        end

    -- ------------------------------------------------------------------------
    elseif format[1] == en1545_REPEAT then -- entry type is repeat
        if item == nil then
            log.print(log.WARNING,"item is empty for repeat entry "..format[3])
            return 0
        end

        item_node:set_attribute("val",item)
        item_node:set_attribute("size",#item)
        item_node:set_attribute("alt",bytes.tonumber(item))

        for index=1,bytes.tonumber(item) do
            parsed = parsed + _l.parse_item(item_node, format[4][0], data, position + parsed)
        end

    -- ------------------------------------------------------------------------
    else -- entry type is item
        if item == nil then
            log.print(log.WARNING,"item is empty for entry "..format[3])
            return 0
        end

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
        parsed = parsed + _l.parse_item(ctx, format[index], data, parsed)
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

function en1545_map(cardenv, condition, ...)
	--[[
	Map card records to en1545 items (as defined in _l.parse_item() function).
	@param condition:
		- If condition is a string, all records under the file having label "condition" will be mapped.
		- If condition is an array, records with find(condition.records) under files with find(condition.file) will be mapped.
	]]
    local record_node
    local bits
    local i
    local parsed
    local block

	local file_condition
	local record_condition
	if type(condition) == "string" then
		file_condition = { label = condition }
		record_condition = { label = "record" }
	else
		file_condition = condition.file
		record_condition = condition.record
	end

	for file in cardenv:find(file_condition) do
        for record_node in file:find(record_condition) do
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
