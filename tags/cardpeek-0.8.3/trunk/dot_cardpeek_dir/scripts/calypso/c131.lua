--
-- This file is part of Cardpeek, the smart card reader utility.
--
-- Copyright 2009-2014 by Alain Pannetrat <L1L1@gmx.com>
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
--********************************************************************--
--
-- This file, c131.lua, was authored and contributed to cardpeek by 
-- David Ludovino	<david.ludovino@gmail.com>
-- Rafael Santos	<raffa256@gmail.com>
-- Duarte Barbosa	<duarte.barbosa@gmail.com>
-- (C) 2013
-- 
--********************************************************************--

require('lib.calypso_card_num')
require('lib.en1545')
require('etc.lisbon-network')

SECONDS_DAY = 86400

local leave_raw = false

-- Modified from original by RichardWarburton
function get_days_in_month(timestamp)
	local yr = tonumber(os.date("%Y", timestamp))
	local mnth = tonumber(os.date("%m", timestamp))
	return os.date('*t',os.time{year=yr,month=mnth+1,day=0})['day']
end

function viva_CONTRACT_USED(source)
	source = source:reverse()
	for i,v in source:ipairs() do
		if v==1 then
			return i + 1
		end
	end
end

function viva_TRANSITION(source)
	return TRANSITION_LIST[source:tonumber()]
end

local operator_num = nil
local operator_info = nil
function viva_OPERATOR(source)
	operator_num = source:tonumber()
	operator_info = LISBON_NETWORK[operator_num]
	if operator_info==nil then
		return "unknown"
	else
		return operator_info['name']
	end
end

function viva_CARD_NUM(source)
	return string.format("%03d", operator_num) .. " " .. string.format("%09d", source:tonumber())
end

local line_num = nil
local line_info = nil
function viva_LINE(source)
	if operator_info==nil then
		return "unknown"
	else
		if operator_num==16 then -- MTS
			source = source:sub(12,15)
		end
		line_num = source:tonumber()
		if operator_num==1 then -- Carris
			line_info = nil
			return line_num
		else
			if operator_num==3 and line_num~=40960 then -- CP
				line_num = 4096
			end
			line_info = operator_info[line_num]
			if line_info==nil then
				return "unknown"
			else
				return line_info['name']
			end
		end
	end
end

function viva_STATION(source, node)
	if line_info==nil then
		return "unknown"
	else
		if operator_num==2 then -- Metro Lisboa
			source = source:sub(0,5)
		end
		local station_num = source:tonumber()
		if operator_num==3 and line_num==40960 then -- CP Sado/Cascais line
			local line_node = node:parent():find_first({label="line"})
			if station_num<=54 then
				line_node:set_attribute("alt", "Cascais")
			else
				line_node:set_attribute("alt", "Sado")
			end
		end
		local station_info = line_info[station_num]
		if station_info==nil then
			return "unknown"
		else
			return station_info
		end
	end
end

local product_num
local contract_num = 0
local is_zapping = {}
function viva_PRODUCT(source)
	is_zapping[contract_num] = false
	product_num = source:tonumber()
	local ret = product_num
	local op_info = LISBON_NETWORK[operator_num]
	if op_info~=nil then
		local product_desc = op_info[product_num]
		if product_desc~=nil then
			ret = product_desc
			if product_desc=="Zapping" then
				is_zapping[contract_num] = true
			end
		end
	end
	contract_num = contract_num + 1
	return ret
end

local units
function viva_PERIOD_UNITS(source)
	units = source:tonumber()
	return PERIOD_UNITS[units]
end

function viva_VALIDITY(source)
	local validity = source:tonumber()
	local end_time
	if units==266 then --months
		end_time = date_days
		for i=1,validity do
			end_time = end_time + get_days_in_month(end_time)*SECONDS_DAY
		end
		end_time = end_time - SECONDS_DAY
		return validity .. " month(s), until " .. os.date("%a %x", end_time)
	elseif units==265 then --days
		-- Subtracting one day because contract is already valid during the start day.
		end_time = date_days + (validity - 1)*24*3600
		return validity .. " days, until " .. os.date("%a %x", end_time)
	else
		return nil
	end
end

local combined_type
function viva_COMBINED_TYPE(source)
	combined_type = source:tonumber()
	return nil
end

function viva_OPERATOR1(source)
	if operator_num~=30 or product_num==720 or product_num==725 then --Navegantes
		return nil
	end
	local op_info = LISBON_NETWORK[source:tonumber()]
	if op_info==nil then
		return "unknown"
	else
		return op_info['name']
	end
end

function viva_OPERATOR21(source)
	if combined_type==8 then
		return viva_OPERATOR1(source)
	end
end

function viva_OPERATOR22(source)
	if combined_type==0 then
		return viva_OPERATOR1(source)
	end
end

local counter_num = 0
function viva_AMOUNT(source)
	local ret = nil
	if is_zapping[counter_num] then
		ret = en1545_AMOUNT(source) .. " (Zapping)"
	end
	counter_num = counter_num + 1
	return ret
end

function viva_SALES_POINT(source)
	local sales_point_num = source:tonumber()
	local sales_point_name = SALES_POINT[sales_point_num]
	if sales_point_name~=nil then
		return sales_point_name
	else
		return sales_point_num
	end
end

function viva_HIDDEN(source)
	return nil
end

calypso_card_num()

-- Get card holder name.
local converter = iconv.open("ISO-8859-1","UTF-8")
local resp, sw
sw,resp = card.verify("0000", 0x80)
if sw==0x9000 then
	sw,resp = card.select("#0003") -- ID folder
	sw,resp = card.read_record(0,1,0x1D)
	if sw==0x9000 then
		CARD:find_first({label="ID"}):append{
			classname = "record",
			label = "name",
			id = "1",
			size = #resp, 
			val = resp,
			alt = converter:iconv(resp:sub(0, 20):format("%P"))}
	end
end

if leave_raw then return end

viva_ICC = {
	[0] = { viva_HIDDEN, 128, "unknown" },
	[1] = { en1545_NUMBER, 32, "card number" },
	[2] = { viva_HIDDEN, 9, "unknown" },
	[3] = { viva_HIDDEN, 5, "operator in some cards" }
}
en1545_map(CARD,"ICC",viva_ICC)

viva_Environment = {
	[0] = { viva_HIDDEN, 30, "unknown" },
	[1] = { viva_OPERATOR, 8, "issuer" },
	[2] = { viva_CARD_NUM, 24, "LV card number" },
	[3] = { en1545_DATE, 14, "issued" },
	[4] = { en1545_DATE, 14, "valid until" },
	[5] = { viva_HIDDEN, 15, "unknown" },
	[6] = { en1545_BCD_DATE, 32, "holder birthdate" }
}
en1545_map(CARD,"Environment",viva_Environment)

viva_Event = {
	[0] = { en1545_DATE_TIME, 30, "time" },
	[1] = { viva_HIDDEN, 38, "contract signature?" },
	[2] = { viva_CONTRACT_USED, 4, "contract used" },
	[3] = { viva_HIDDEN, 24, "card pre-set info" },
	[4] = { viva_HIDDEN, 5, "unknown" },
	[5] = { viva_TRANSITION, 3, "transition" },
	[6] = { viva_OPERATOR, 5, "operator" },
	[7] = { viva_HIDDEN, 20, "vehicule data?" },
	[8] = { en1545_NUMBER, 16, "device" },
	[9] = { viva_LINE, 16, "line" },
	[10] = { viva_STATION, 8, "station" },
	[11] = { viva_HIDDEN, 63, "unknown" }
}
en1545_map(CARD,"Event logs",viva_Event)

viva_Contract = {
	[0] = { viva_OPERATOR, 7, "operator" },
	[1] = { viva_PRODUCT, 16, "product" },
	[2] = { viva_HIDDEN, 2, "unknown" },
	[3] = { en1545_DATE, 14, "start date" },
	[4] = { viva_SALES_POINT, 5, "sales point" },
	[5] = { viva_HIDDEN, 19, "sales information" },
	[6] = { viva_PERIOD_UNITS, 16, "period units" },
	[7] = { en1545_DATE, 14, "start/end date" },
	[8] = { viva_VALIDITY, 7, "validity period" },
	[9] = { viva_HIDDEN, 3, "unknown" },
	[10] = { viva_OPERATOR1, 5, "operator 1" },
	[11] = { viva_COMBINED_TYPE, 4, "combined type" },
	[12] = { viva_HIDDEN, 11, "unknown" },
	[13] = { viva_OPERATOR21, 5, "operator 2" },
	[14] = { viva_HIDDEN, 5, "unknown" },
	[15] = { viva_OPERATOR22, 5, "operator 2" }
}
en1545_map(CARD,"Contracts",viva_Contract)

viva_Counter = {
	[0] = { viva_AMOUNT, 24, "amount" }
}
en1545_map(CARD,"Counters",viva_Counter)

function viva_parse_dump()
	-- Reset variables before parsing card.
	counter_num = 0
	contract_num = 0
	en1545_map(nodes.root(),"ICC",viva_ICC)
	en1545_map(nodes.root(),"Environment",viva_Environment)
	en1545_map(nodes.root(),"Event logs",viva_Event)
	en1545_map(nodes.root(),"Contracts",viva_Contract)
	en1545_map(nodes.root(),"Counters",viva_Counter)
end
