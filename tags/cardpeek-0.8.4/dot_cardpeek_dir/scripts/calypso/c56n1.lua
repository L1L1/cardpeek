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
-- 
-- ACKNOWLEDGEMENT:
-- The MOBIB "Event" parsing included in this file was build from
-- information found in public-domain software from Gildas Avoine, 
-- Tania Martin and Jean-Pierre Szikora from the Université catholique 
-- de Louvain, Belgium.


require('lib.strict')
require('lib.en1545')
require('etc.brussels-metro')
require('etc.brussels-bus')

function mobib_BIRTHDATE(source)
	local source_digits = source:convert(4)
	local year   = tonumber(source_digits:sub(0,3):format("%D"))
	local month  = tonumber(source_digits:sub(4,5):format("%D"))
	local day    = tonumber(source_digits:sub(6,7):format("%D"))
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
	local source_digits = source:convert(4)
	return  tostring(source_digits:sub(0,5)).."/"..
		tostring(source_digits:sub(6,17)).."/"..
		tostring(source_digits:sub(18,18))
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
	ref:append{ classname="item",
	 	    label=label,
		    size=endp-beginp+1,
		    val=data,
		    alt=res }
	return res
end

function mobib_process_holderext(ref)
	local block = ref:get_attribute("val"):convert(1)
	if bytes.is_all(block,0) then
		return false
	end
	mobib_process_block(ref,"HolderNumber",18,93,block,mobib_HOLDERNUMBER)
	mobib_process_block(ref,"HolderBirthRate",168,199,block,mobib_BIRTHDATE)
	mobib_process_block(ref,"HolderGender",200,201,block,mobib_GENDER)
	mobib_process_block(ref,"HolderName",205,359,block,en1545_ALPHA)
end

function mobib_process_event(ref)
	local block = ref:get_attribute("val"):convert(1)
	local transport_type
	local ref2

	if bytes.is_all(block,0) then
		ref:set_attribute("alt","NO EVENT RECORDED")
		return false
	end
	mobib_process_block(ref,"EventDataDateStamp",6,19,block,en1545_DATE)
	mobib_process_block(ref,"EventDataTimeStamp",20,30,block,en1545_TIME)
	mobib_process_block(ref,"EventDataDateFirstStamp",186,199,block,en1545_DATE)
	mobib_process_block(ref,"EventDataTimeFirstStamp",200,210,block,en1545_TIME)
	
	mobib_process_block(ref,"EventCountOfCoupons",139,161,block,en1545_NUMBER)
	transport_type = mobib_process_block(ref,"EventTransportType",99,103,block,mobib_TRANSPORT)
	if transport_type=="metro" or transport_type=="premetro" then
		mobib_process_block(ref,"EventLocationId",104,120,block,mobib_TRANSPORT_METRO)
		ref2 = ref:find_first({label="EventLocationId"})	
		mobib_process_block(ref2,"EventLocationZone",104,109,block,en1545_NUMBER)
		mobib_process_block(ref2,"EventLocationSubZone",110,113,block,en1545_NUMBER)
		mobib_process_block(ref2,"EventLocationStation",114,120,block,en1545_NUMBER)
	elseif transport_type=="bus" then
		mobib_process_block(ref,"EventLocationId",71,82,block,mobib_TRANSPORT_BUS)
		mobib_process_block(ref,"EventJourneyRoute",92,98,block,en1545_NUMBER)		
	end
	return true
end

function mobib_process_env(ref)
	local block = ref:get_attribute("val"):convert(1)
	if bytes.is_all(block,0) then
		ref:set_attribute("alt","NO ENVIRONMENT DATA")
	        return false
	end
	mobib_process_block(ref,"EnvNetworkId",13,36,block,en1545_NETWORKID)
	return true
end

function mobib_process(cardenv)
	local record1_node
	local record2_node
	local data

	record1_node = cardenv:find_first({label="Holder Extended"}):find_first({label="record",id="1"})
	if record1_node==nil then 
		log.print(LOG_ERROR,"Could not find record 1 in 'Holder Extended' file")
		return false
	end

	record2_node= cardenv:find_first({label="Holder Extended"}):find_first({label="record",id="2"})
	if record2_node==nil then 
		log.print(LOG_ERROR,"Could not find record 2 in 'Holder Extended' file")
		return false
	end
	
	data = record1_node:get_attribute("val") .. record2_node:get_attribute("val")
	
	record1_node:remove()
	record2_node:remove()

	local node
	node = cardenv:find_first({label="Holder Extended"})
		      :append({ classname = "record", 
		                label = "record",
		      		id = "1+2",
		      		size = #data,
		      		val = data })
	mobib_process_holderext(node)

	for node in cardenv:find_first({label="Event logs"}):find({label="record"}) do
		mobib_process_event(node)
	end
	for node in cardenv:find_first({label="Environment"}):find({label="record"}) do
		mobib_process_env(node)
	end
end

mobib_process(CARD)


