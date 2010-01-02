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

-------------------------------------------------------------------------
-- *PLEASE NOTE*
-- This work is based on:
-- * public information about the calypso card specification, 
-- * partial information found on the web about the ticketing data 
--   format, as described in the French "intertitre" documentation.
-- * experimentation and guesses, 
-- This information is incomplete. If you have further data, such 
-- as details of ISO 1543 or calypso card specs, please help send them
-- to L1L1@gmx.com
--------------------------------------------------------------------------

card.CLA=0x94 -- Class for navigo cards

require('lib.apdu')
--require('lib.strict')

LFI_LIST = {
  ["0002"] = "Unknown",
  ["2001"] = "Environment",
  ["2010"] = "Event logs",
  ["2020"] = "Contracts",
  ["2040"] = "Special events",
  ["2050"] = "Contract list",
  ["2069"] = "Counters"
}

en1543_BITMAP = 1
en1543_ITEM = 2
en1543_DATE = 3
en1543_TIME = 4
en1543_NUMBER = 5
en1543_REPEAT = 6

en1543_BestContracts = {
  [0] = { en1543_REPEAT, 4, "Count", {
    [0] = { en1543_BITMAP, 3, "Bitmap", {
      [0] = { en1543_ITEM, 24, "NetworkId" },
      [1] = { en1543_ITEM, 16, "Tariff" },
      [2] = { en1543_ITEM, 5, "Pointer" }
    }}
  }}
}

en1543_Env = {
  [0] = { en1543_ITEM, 6, "VersionNumber" },
  [1] = { en1543_BITMAP, 7, "GeneralBitmap",{
    [0] = { en1543_ITEM, 24, "NetworkId" },
    [1] = { en1543_ITEM, 8, "ApplicationIssuerId" },
    [2] = { en1543_DATE, 14, "ApplicationValidityEndDate" },
    [3] = { en1543_ITEM, 11, "PayMethod" },
    [4] = { en1543_ITEM, 16, "Authenticator" },
    [5] = { en1543_ITEM, 32, "SelectList" },
    [6] = { en1543_ITEM, 2, "Data",
      [0] = { en1543_ITEM, 1, "CardStatus" },
      [1] = { en1543_ITEM, 0, "Extra" },
    }}
  }
}

en1543_Contract = {
  [0] = { en1543_BITMAP, 20, "ContractBitmap",
    {
      [0] = { en1543_ITEM, 24, "NetworkId" },
      [1] = { en1543_ITEM,  8, "Provider" },
      [2] = { en1543_ITEM, 16, "Tariff" },
      [3] = { en1543_ITEM, 32, "Serial Number" },
      [4] = { en1543_BITMAP,  2, "CustomerInfo Bitmap", {
	[0] = { en1543_ITEM,  6, "CustomerProfile" },
	[1] = { en1543_ITEM, 32, "CustomerNumber" },
      }},
      [5] = { en1543_BITMAP,  2, "PassengerInfo Bitmap", {
	[0] = { en1543_ITEM,  6, "PassengerClass" },
	[1] = { en1543_ITEM, 32, "PassengerTotal" },
      }},
      [6] = { en1543_ITEM, 6, "VehiculeClassAllowed" },
      [7] = { en1543_ITEM, 32, "PaymentPointer" },
      [8] = { en1543_ITEM, 11, "PayMethod" },
      [9] = { en1543_ITEM, 16, "Services" },
      [10] = { en1543_NUMBER, 16, "PriceAmount" },
      [11] = { en1543_ITEM, 16, "PriceUnit" },
      [12] = { en1543_BITMAP, 7, "ContractRestrictionBitmap", {
	[0] = { en1543_TIME, 11, "StartTime" },
	[1] = { en1543_TIME, 11, "EndTime" },
	[2] = { en1543_ITEM, 8, "RestrictDay" },
	[3] = { en1543_ITEM, 8, "RestrictTimeCode" },
	[4] = { en1543_ITEM, 8, "RestrictCode" },
	[5] = { en1543_ITEM, 16, "RestrictProduct" },
	[6] = { en1543_ITEM, 16, "RestrcitLocation" },
      }},
      [13] = { en1543_BITMAP, 9, "ContractValidityInfoBitmap", {
	[0] = { en1543_DATE, 14, "StartDate" },
	[1] = { en1543_TIME, 11, "StartTime" },
	[2] = { en1543_DATE, 14, "EndDate" },
	[3] = { en1543_TIME, 11, "EndTime" },
	[4] = { en1543_ITEM, 8, "Duration" },
	[5] = { en1543_DATE, 14, "LimitDate" },
	[6] = { en1543_ITEM, 8, "Zones" },
	[7] = { en1543_ITEM, 16, "Journeys" },
	[8] = { en1543_ITEM, 16, "PeriodJourneys" },
      }},
      [14] = { en1543_BITMAP, 8, "ContractJourneyDataBitmap", {
	[0] = { en1543_ITEM, 16, "Origin" },
	[1] = { en1543_ITEM, 16, "Destination" },
	[2] = { en1543_ITEM, 16, "RouteNumbers" },
	[3] = { en1543_ITEM, 8, "RouteVariants" },
	[4] = { en1543_ITEM, 16, "Run" },
	[5] = { en1543_ITEM, 16, "Via" },
	[6] = { en1543_ITEM, 16, "Distance" },
	[7] = { en1543_ITEM, 8, "Interchange" },
      }},
      [15] = { en1543_BITMAP, 4, "ContractSaleDataBitmap", {
	[0] = { en1543_DATE, 14, "Date" },
	[1] = { en1543_TIME, 11, "Time" },
	[2] = { en1543_ITEM, 8, "Agent" },
	[3] = { en1543_ITEM, 16, "Device" },
      }},
      [16] = { en1543_ITEM, 8, "Status" },
      [17] = { en1543_ITEM, 16, "LoyalityPoints" },
      [18] = { en1543_ITEM, 16, "Authenticator" },
      [19] = { en1543_ITEM, 0, ""},
    }
  }
}

en1543_Event = {
  [0] = { en1543_DATE, 14, "Date" },
  [1] = { en1543_TIME, 11, "Time" },
  [2] = { en1543_BITMAP, 28, "EventBitmap",
    {
      [0] = { en1543_ITEM,  8, "DisplayData" },
      [1] = { en1543_ITEM, 24, "Network" },
      [2] = { en1543_ITEM,  8, "Code" },
      [3] = { en1543_ITEM,  8, "Result" },
      [4] = { en1543_ITEM,  8, "ServiceProvider" },
      [5] = { en1543_ITEM,  8, "NotOkCounter" },
      [6] = { en1543_ITEM, 24, "SerialNumber" },
      [7] = { en1543_ITEM, 16, "Destination" },
      [8] = { en1543_ITEM, 16, "LocationId" },
      [9] = { en1543_ITEM,  8, "LocationGate" },
      [10] = { en1543_ITEM, 16, "Device" },
      [11] = { en1543_NUMBER, 16, "RouteNumber" },
      [12] = { en1543_ITEM,  8, "RouteVariant" },
      [13] = { en1543_ITEM, 16, "JourneyRun" },
      [14] = { en1543_ITEM, 16, "VehiculeId" },
      [15] = { en1543_ITEM,  8, "VehiculeClass" },
      [16] = { en1543_ITEM,  5, "LocationType" },
      [17] = { en1543_ITEM,240, "Employee" },
      [18] = { en1543_ITEM, 16, "LocationReference" },
      [19] = { en1543_ITEM,  8, "JourneyInterchanges" },
      [20] = { en1543_ITEM, 16, "PeriodJourneys" },
      [21] = { en1543_ITEM, 16, "TotalJourneys" },
      [22] = { en1543_ITEM, 16, "JourneyDistance" },
      [23] = { en1543_NUMBER, 16, "PriceAmount" },
      [24] = { en1543_ITEM, 16, "PriceUnit" },
      [25] = { en1543_ITEM,  5, "ContractPointer" },
      [26] = { en1543_ITEM, 16, "Authenticator" },
      [27] = { en1543_ITEM,  5, "BitmapExtra" },
    }
  }
}
EPOCH = 852073200

function en1543_date(source, position)
	local date_days
	local part =  bytes.sub(source, position, position+13) -- 14 bits
	bytes.pad_left(part,32,0)
	date_days = EPOCH+bytes.to_number(part)*24*3600
	return os.date("%x",date_days)
end

function en1543_time(source, position)
	local date_minutes
	local part = bytes.sub(source, position, position+10) -- 11 bits
	bytes.pad_left(part,32,0)
	date_minutes = bytes.to_number(part)*60
	return os.date("%X",date_minutes)
end

function en1543_parse_item(ctx, format, data, position, reference_index)
	local parsed = 0
	local index
	local NODE
	local bitmap_size
	local hex_info
	local item

	if format == nil then
	   return 0
	end

	parsed = format[2]
	item = bytes.sub(data,position,position+parsed-1)
      	NODE = ui.tree_add_node(ctx,format[3],reference_index,nil,
     				string.format("%d bits: %s",parsed,tostring(item)))

	if format[1] == en1543_BITMAP then
	   bitmap_size = parsed
	   parsed = bitmap_size 
	   for index=0,format[2]-1 do
	       if data[position+bitmap_size-index-1]~=0 then
	          parsed = parsed + en1543_parse_item(NODE, format[4][index], data, position+parsed, index)
	       end
	   end
	elseif format[1] == en1543_DATE then
	   ui.tree_set_value(NODE,en1543_date(item,0))
	elseif format[1] == en1543_TIME then
	   ui.tree_set_value(NODE,en1543_time(item,0)) 
	elseif format[1] == en1543_NUMBER then
	   ui.tree_set_value(NODE,bytes.to_number(item))
        elseif format[1] == en1543_REPEAT then
	   ui.tree_set_value(NODE,bytes.to_number(item))
           for index=1,bytes.to_number(item) do
	       parsed = parsed + en1543_parse_item(ctx, format[4][0], data, position+parsed, reference_index+index)
	   end 
	else 
           hex_info = bytes.convert(8,item)
	   ui.tree_set_value(NODE,"0x"..tostring(hex_info))
	end
	return parsed
end

function en1543_parse(ctx, format, data)
	local index
	local parsed = 0
	for index=0,#format do
	    parsed = parsed + en1543_parse_item(ctx,format[index],data,parsed,index)
	end
	return parsed
end

function en1543_process(ctx)
	local DATA_REF
	local RECORD_REF
        local NODE_REF
	local data
	local bits
	local index

	NODE_REF = ui.tree_find_node(ctx,"Event logs")
	
	if NODE_REF then
	   for index=1,16 do
	      RECORD_REF=ui.tree_find_node(NODE_REF,"record",index)
	      if RECORD_REF==nil then break end
	      DATA_REF = ui.tree_find_node(RECORD_REF,"raw data")
	      data = bytes.new(8,ui.tree_get_value(DATA_REF))
	      bits = bytes.convert(1,data)
	      en1543_parse(RECORD_REF,en1543_Event,bits)
	   end
	else
	   log.print(LOG_DEBUG,"No event logs")
	end

	NODE_REF = ui.tree_find_node(ctx,"Special events")
	
	if NODE_REF then
	   for index=1,16 do
	      RECORD_REF=ui.tree_find_node(NODE_REF,"record",index)
	      if RECORD_REF==nil then break end
	      DATA_REF = ui.tree_find_node(RECORD_REF,"raw data")
	      data = bytes.new(8,ui.tree_get_value(DATA_REF))
	      bits = bytes.convert(1,data)
	      en1543_parse(RECORD_REF,en1543_Event,bits)
	   end
	else
	   log.print(LOG_DEBUG,"No special events")
	end

	NODE_REF = ui.tree_find_node(ctx,"Environment")

	if NODE_REF then
	   RECORD_REF=ui.tree_find_node(NODE_REF,"record",1)
	   DATA_REF = ui.tree_find_node(RECORD_REF,"raw data")
	   data = bytes.new(8,ui.tree_get_value(DATA_REF))
	   bits = bytes.convert(1,data)
	   en1543_parse(RECORD_REF,en1543_Env,bits)
	else
	   log.print(LOG_DEBUG,"No environment") 
	end

	NODE_REF = ui.tree_find_node(ctx,"Contract list")
	
	if NODE_REF then
	   RECORD_REF=ui.tree_find_node(NODE_REF,"record",1)
	   DATA_REF = ui.tree_find_node(RECORD_REF,"raw data")
	   data = bytes.new(8,ui.tree_get_value(DATA_REF))
	   bits = bytes.convert(1,data)
	   en1543_parse(RECORD_REF,en1543_BestContracts,bits)
	
           --
      	   -- FIXME: here we should parse the contracts according to "Tariff"
	   --        but for now we will assume that contract format is 'FF'
  
	else
	   log.print(LOG_DEBUG,"No contract list (BestContracts)") 
	end

	NODE_REF = ui.tree_find_node(ctx,"Contracts")
	
	if NODE_REF then
	   for index=1,16 do
	      RECORD_REF=ui.tree_find_node(NODE_REF,"record",index)
	      if RECORD_REF==nil then break end
	      DATA_REF = ui.tree_find_node(RECORD_REF,"raw data")
	      data = bytes.new(8,ui.tree_get_value(DATA_REF))
	      bits = bytes.convert(1,data)
	      en1543_parse(RECORD_REF,en1543_Contract,bits)
	   end
	else
	   log.print(LOG_DEBUG,"No contracts")
	end


end


function process_calypso(cardenv)
	local APP
	local DF_NAME = "1TIC.ICA" 
	local lfi
	local lfi_desc
	local LFI
	local REC
	local sw, resp
--
	local NODE


	APP = ui.tree_add_node(cardenv,"application",DF_NAME)
	-- note: no real DF select here
 
	for lfi,lfi_desc in pairs(LFI_LIST) do
	        sw,resp = card.select_file("2000",8)
		if sw~=0x9000 then 
		        break
		end
		sw,resp = card.select_file(lfi,8)
		if sw==0x9000 then
	                local r
			LFI = ui.tree_add_node(APP,lfi_desc,lfi)
			for r=1,255 do
				sw,resp=card.read_record(0,r,0x1D)
				if sw ~= 0x9000 then
					break
				end
				REC = ui.tree_add_node(LFI,"record",r,#resp)
--
                                NODE = ui.tree_add_node(REC,"raw data")
	                        ui.tree_set_value(NODE,tostring(resp))
				--en1543_analyze(REC,resp,lfi_desc)
			end
		end
	end
	en1543_process(APP)
end

card.connect()

CARD = card.tree_startup("CALYPSO")

atr = card.last_atr();
hex_card_num = bytes.sub(atr,-7,-4)
card_num     = (hex_card_num[0]*256*65536)+(hex_card_num[1]*65536)+(hex_card_num[2]*256)+hex_card_num[3]

ref=ui.tree_add_node(CARD,"Card number",nil,4,"hex: "..tostring(hex_card_num))
ui.tree_set_value(ref,card_num)

process_calypso(CARD)

card.disconnect()
