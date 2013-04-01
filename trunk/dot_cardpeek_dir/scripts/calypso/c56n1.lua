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
-- 
-- ACKNOWLEDGEMENT:
-- The MOBIB "Event" parsing included in this file was build from
-- information found in public-domain software from Gildas Avoine, 
-- Tania Martin and Jean-Pierre Szikora from the Université catholique 
-- de Louvain, Belgium.


require('lib.strict')
require('lib.en1545')
require('lib.tnode')
require('etc.brussels-metro')
require('etc.brussels-bus')

function mobib_BIRTHDATE(source)
	local source_digits = bytes.convert(4,source)
	local year   = tonumber(bytes.format("%D",bytes.sub(source_digits,0,3)))
	local month  = tonumber(bytes.format("%D",bytes.sub(source_digits,4,5)))
	local day    = tonumber(bytes.format("%D",bytes.sub(source_digits,6,7)))
	return os.date("%x",os.time({['year']=year,['month']=month,['day']=day}))
end

function mobib_GENDER(source)
	local n = bytes.tonumber(source)
	if n==1 then
		return "Male"
	elseif n==2 then
		return "Female"
	end
	return "Unknown"
end

function mobib_HOLDERNUMBER(source)
	local source_digits = bytes.convert(4,source)
	return  tostring(bytes.sub(source_digits,0,5)).."/"..
		tostring(bytes.sub(source_digits,6,17)).."/"..
		tostring(bytes.sub(source_digits,18,18))
end


function mobib_TRANSPORT_BUS(source)
	local stationid = bytes.tonumber(source)
	local retval = BRUSSELS_BUS[stationid]
	if retval then
		return retval
	end
	return "Unknown location"
end

function mobib_TRANSPORT_METRO(source)
	local stationid = bytes.tonumber(source)
	local retval = BRUSSELS_METRO[stationid]
	if retval then
		return retval
	end
	return "Unknown location"
end

function mobib_TRANSPORT(source)
	local t = bytes.tonumber(source)
	if t==0 then return "metro"
	elseif t==7 then return "premetro"
	elseif t==15 then return "bus"
	elseif t==22 then return "tramway"
	else return "UNKNOWN("..t..")"
	end
end

function mobib_process_block(ref,label,beginp,endp,block,func)
	local data = bytes.sub(block,beginp,endp)
	local res = func(data)
	_n(ref):append{ classname="item",
	 	        label=label,
		        size=endp-beginp+1,
		        val=data,
		        alt=res }
	return res
end

function mobib_process_holderext(i,ref)
	local block = bytes.convert(1,ui.tree_get_value(ref))
	if bytes.is_all(block,0) then
		return false
	end
	mobib_process_block(ref,"HolderNumber",18,93,block,mobib_HOLDERNUMBER)
	mobib_process_block(ref,"HolderBirthRate",168,199,block,mobib_BIRTHDATE)
	mobib_process_block(ref,"HolderGender",200,201,block,mobib_GENDER)
	mobib_process_block(ref,"HolderName",205,359,block,en1545_ALPHA)
end

function mobib_process_event(i,ref)
	local block = bytes.convert(1,ui.tree_get_value(ref))
	local transport_type
	local ref2

	if bytes.is_all(block,0) then
		_n(ref):alt("NO EVENT RECORDED")
		return false
	end
	mobib_process_block(ref,"EventDataDateStamp",6,19,block,en1545_DATE)
	mobib_process_block(ref,"EventDataTimeStamp",20,30,block,en1545_TIME)
	mobib_process_block(ref,"EventDataDateFirstStamp",186,199,block,en1545_DATE)
	mobib_process_block(ref,"EventDataTimeFirstStamp",200,210,block,en1545_TIME)
	
	mobib_process_block(ref,"EventCountOfCoupons",139,161,block,en1545_NUMBER)
	transport_type = mobib_process_block(ref,"EventTransportType",99,104,block,mobib_TRANSPORT)
	if transport_type=="metro" or transport_type=="premetro" then
		mobib_process_block(ref,"EventLocationId",104,120,block,mobib_TRANSPORT_METRO)
		ref2 = _n(ref):find("EventLocationId"):get(1)	
		mobib_process_block(ref2,"EventLocationZone",104,109,block,en1545_NUMBER)
		mobib_process_block(ref2,"EventLocationSubZone",110,113,block,en1545_NUMBER)
		mobib_process_block(ref2,"EventLocationStation",114,120,block,en1545_NUMBER)
	elseif transport_type=="bus" then
		mobib_process_block(ref,"EventLocationId",71,82,block,mobib_TRANSPORT_BUS)
		mobib_process_block(ref,"EventJourneyRoute",92,98,block,en1545_NUMBER)		
	end
	return true
end

function mobib_process_env(i,ref)
	local block = bytes.convert(1,ui.tree_get_value(ref))
	if bytes.is_all(block,0) then
		_n(ref):alt("NO ENVIRONMENT DATA")
	        return false
	end
	mobib_process_block(ref,"EnvNetworkId",13,36,block,en1545_NETWORKID)
	return true
end

function mobib_process(cardenv)
	local record1_node
	local record2_node
	local data

	record1_node = cardenv:find("Holder Extended"):find("record#1")
	if record1_node:length()==0 then 
		log.print(LOG_ERROR,"Could not find record 1 in 'Holder Extended' file")
		return false
	end

	record2_node= cardenv:find("Holder Extended"):find("record#2")
	if record2_node:length()==0 then 
		log.print(LOG_ERROR,"Could not find record 2 in 'Holder Extended' file")
		return false
	end
	
	data = record1_node:val() .. record2_node:val()
	
	record1_node:remove()
	record2_node:remove()

	cardenv:find("Holder Extended")
		:append({ classname = "record", 
			  label = "record",
			  id = "1+2",
			  size = #data,
			  val = data })
		:each(mobib_process_holderext)

	cardenv:find("Event logs"):find("record"):each(mobib_process_event)
	cardenv:find("Environment"):find("record"):each(mobib_process_env)
end

mobib_process(CARD)


