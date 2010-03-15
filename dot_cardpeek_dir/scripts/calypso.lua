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

CARD = 0
card.CLA=0x94 -- Class for navigo cards

require('lib.strict')
require('lib.country_codes')

LFI_LIST = {
  ["/0002"] = "ICC",
  ["/0003"] = "ID",
  ["/2001"] = "Environment",
  ["/2010"] = "Event logs",
  ["/2020"] = "Contracts",
  ["/2040"] = "Special events",
  ["/2050"] = "Contract list",
  ["/2069"] = "Counters"
}

en1543_BITMAP = 1
en1543_ITEM = 2
en1543_DATE = 3
en1543_TIME = 4
en1543_NUMBER = 5
en1543_REPEAT = 6
en1543_NETWORKID = 7

en1543_BestContracts = {
  [0] = { en1543_REPEAT, 4, "BestContractCount", {
    [0] = { en1543_BITMAP, 3, "BestContractGeneralBitmap", {
      [0] = { en1543_NETWORKID, 24, "BestContractsNetworkId" },
      [1] = { en1543_ITEM, 16, "BestContractsTariff" },
      [2] = { en1543_ITEM, 5, "BestContractsPointer" }
    }}
  }}
}

en1543_Env = {
  [0] = { en1543_ITEM, 6, "EnvVersionNumber" },
  [1] = { en1543_BITMAP, 7, "EnvGeneralBitmap",{
    [0] = { en1543_NETWORKID, 24, "EnvNetworkId" },
    [1] = { en1543_ITEM, 8, "EnvApplicationIssuerId" },
    [2] = { en1543_DATE, 14, "EnvApplicationValidityEndDate" },
    [3] = { en1543_ITEM, 11, "EnvPayMethod" },
    [4] = { en1543_ITEM, 16, "EnvAuthenticator" },
    [5] = { en1543_ITEM, 32, "EnvSelectList" },
    [6] = { en1543_ITEM, 2, "EnvData",
      [0] = { en1543_ITEM, 1, "EnvCardStatus" },
      [1] = { en1543_ITEM, 0, "EnvExtra" },
    }}
  }
}

en1543_Holder = {
  [0] = { en1543_BITMAP, 8, "HolderGeneralBitmap", {
    [0] = { en1543_BITMAP, 2, "HolderNameBitmap", {
      [0] = { en1543_ITEM, 85, "HolderSurname" },
      [1] = { en1543_ITEM, 85, "HolderForename" }
    }},
    [1] = { en1543_BITMAP, 2, "HolderBirthBitmap", {
      [0] = { en1543_ITEM, 32, "HolderBirthDate" },
      [1] = { en1543_ITEM, 115, "HolderBirthPlace"}
    }},
    [2] = { en1543_ITEM, 85, "HolderBirthName" },
    [3] = { en1543_ITEM, 32, "HolderIdNumber" },
    [4] = { en1543_ITEM, 24, "HolderCountryAlpha" },
    [5] = { en1543_ITEM, 32, "HolderCompany" },
    [6] = { en1543_REPEAT, 4, "HolderProfiles(0..4)", {
      [0] = { en1543_BITMAP, 3, "ProfileBitmap", {
      	[0] = { en1543_ITEM, 24, "NetworkId" },
	[1] = { en1543_ITEM, 8, "ProfileNumber" },
	[2] = { en1543_DATE, 14, "ProfileDate" }
	}}
    }},
    [7] = { en1543_BITMAP, 12, "HolderDataBitmap", {
      [0] = { en1543_ITEM, 4, "HolderDataCardStatus" },
      [1] = { en1543_ITEM, 4, "HolderDataTeleReglement" },
      [2] = { en1543_ITEM, 17, "HolderDataResidence" },
      [3] = { en1543_ITEM, 6, "HolderDataCommercialID" },
      [4] = { en1543_ITEM, 17, "HolderDataWorkPlace" },
      [5] = { en1543_ITEM, 17, "HolderDataStudyPlace" },
      [6] = { en1543_ITEM, 16, "HolderDataSaleDevice" },
      [7] = { en1543_ITEM, 16, "HolderDataAuthenticator" },
      [8] = { en1543_ITEM, 14, "HolderDataProfileStartDate1" },
      [9] = { en1543_ITEM, 14, "HolderDataProfileStartDate2" },
      [10] = { en1543_ITEM, 14, "HolderDataProfileStartDate3" },
      [11] = { en1543_ITEM, 14, "HolderDataProfileStartDate4" }
    }}
  }}
}

en1543_Contract = {
  [0] = { en1543_BITMAP, 20, "ContractBitmap",
    {
      [0] = { en1543_NETWORKID, 24, "ContractNetworkId" },
      [1] = { en1543_ITEM,  8, "ContractProvider" },
      [2] = { en1543_ITEM, 16, "ContractTariff" },
      [3] = { en1543_ITEM, 32, "ContractSerialNumber" },
      [4] = { en1543_BITMAP,  2, "ContractCustomerInfoBitmap", {
	[0] = { en1543_ITEM,  6, "ContractCustomerProfile" },
	[1] = { en1543_ITEM, 32, "ContractCustomerNumber" },
      }},
      [5] = { en1543_BITMAP,  2, "ContractPassengerInfoBitmap", {
	[0] = { en1543_ITEM,  6, "ContractPassengerClass" },
	[1] = { en1543_ITEM, 32, "ContractPassengerTotal" },
      }},
      [6] = { en1543_ITEM, 6, "ContractVehiculeClassAllowed" },
      [7] = { en1543_ITEM, 32, "ContractPaymentPointer" },
      [8] = { en1543_ITEM, 11, "ContractPayMethod" },
      [9] = { en1543_ITEM, 16, "ContractServices" },
      [10] = { en1543_NUMBER, 16, "ContractPriceAmount" },
      [11] = { en1543_ITEM, 16, "ContractPriceUnit" },
      [12] = { en1543_BITMAP, 7, "ContractContractRestrictionBitmap", {
	[0] = { en1543_TIME, 11, "ContractStartTime" },
	[1] = { en1543_TIME, 11, "ContractEndTime" },
	[2] = { en1543_ITEM, 8, "ContractRestrictDay" },
	[3] = { en1543_ITEM, 8, "ContractRestrictTimeCode" },
	[4] = { en1543_ITEM, 8, "ContractRestrictCode" },
	[5] = { en1543_ITEM, 16, "ContractRestrictProduct" },
	[6] = { en1543_ITEM, 16, "ContractRestrcitLocation" },
      }},
      [13] = { en1543_BITMAP, 9, "ContractContractValidityInfoBitmap", {
	[0] = { en1543_DATE, 14, "ContractStartDate" },
	[1] = { en1543_TIME, 11, "ContractStartTime" },
	[2] = { en1543_DATE, 14, "ContractEndDate" },
	[3] = { en1543_TIME, 11, "ContractEndTime" },
	[4] = { en1543_ITEM, 8, "ContractDuration" },
	[5] = { en1543_DATE, 14, "ContractLimitDate" },
	[6] = { en1543_ITEM, 8, "ContractZones" },
	[7] = { en1543_ITEM, 16, "ContractJourneys" },
	[8] = { en1543_ITEM, 16, "ContractPeriodJourneys" },
      }},
      [14] = { en1543_BITMAP, 8, "ContractContractJourneyDataBitmap", {
	[0] = { en1543_ITEM, 16, "ContractOrigin" },
	[1] = { en1543_ITEM, 16, "ContractDestination" },
	[2] = { en1543_ITEM, 16, "ContractRouteNumbers" },
	[3] = { en1543_ITEM, 8, "ContractRouteVariants" },
	[4] = { en1543_ITEM, 16, "ContractRun" },
	[5] = { en1543_ITEM, 16, "ContractVia" },
	[6] = { en1543_ITEM, 16, "ContractDistance" },
	[7] = { en1543_ITEM, 8, "ContractInterchange" },
      }},
      [15] = { en1543_BITMAP, 4, "ContractContractSaleDataBitmap", {
	[0] = { en1543_DATE, 14, "ContractDate" },
	[1] = { en1543_TIME, 11, "ContractTime" },
	[2] = { en1543_ITEM, 8, "ContractAgent" },
	[3] = { en1543_ITEM, 16, "ContractDevice" },
      }},
      [16] = { en1543_ITEM, 8, "ContractStatus" },
      [17] = { en1543_ITEM, 16, "ContractLoyalityPoints" },
      [18] = { en1543_ITEM, 16, "ContractAuthenticator" },
      [19] = { en1543_ITEM, 0, "Contract"},
    }
  }
}

en1543_Event = {
  [0] = { en1543_DATE, 14, "EventDate" },
  [1] = { en1543_TIME, 11, "EventTime" },
  [2] = { en1543_BITMAP, 28, "EventEventBitmap",
    {
      [0] = { en1543_ITEM,  8, "EventDisplayData" },
      [1] = { en1543_NETWORKID, 24, "EventNetworkId" },
      [2] = { en1543_ITEM,  8, "EventCode" },
      [3] = { en1543_ITEM,  8, "EventResult" },
      [4] = { en1543_ITEM,  8, "EventServiceProvider" },
      [5] = { en1543_ITEM,  8, "EventNotOkCounter" },
      [6] = { en1543_ITEM, 24, "EventSerialNumber" },
      [7] = { en1543_ITEM, 16, "EventDestination" },
      [8] = { en1543_ITEM, 16, "EventLocationId" },
      [9] = { en1543_ITEM,  8, "EventLocationGate" },
      [10] = { en1543_ITEM, 16, "EventDevice" },
      [11] = { en1543_NUMBER, 16, "EventRouteNumber" },
      [12] = { en1543_ITEM,  8, "EventRouteVariant" },
      [13] = { en1543_ITEM, 16, "EventJourneyRun" },
      [14] = { en1543_ITEM, 16, "EventVehiculeId" },
      [15] = { en1543_ITEM,  8, "EventVehiculeClass" },
      [16] = { en1543_ITEM,  5, "EventLocationType" },
      [17] = { en1543_ITEM,240, "EventEmployee" },
      [18] = { en1543_ITEM, 16, "EventLocationReference" },
      [19] = { en1543_ITEM,  8, "EventJourneyInterchanges" },
      [20] = { en1543_ITEM, 16, "EventPeriodJourneys" },
      [21] = { en1543_ITEM, 16, "EventTotalJourneys" },
      [22] = { en1543_ITEM, 16, "EventJourneyDistance" },
      [23] = { en1543_NUMBER, 16, "EventPriceAmount" },
      [24] = { en1543_ITEM, 16, "EventPriceUnit" },
      [25] = { en1543_ITEM,  5, "EventContractPointer" },
      [26] = { en1543_ITEM, 16, "EventAuthenticator" },
      [27] = { en1543_ITEM,  5, "EventBitmapExtra" },
    }
  }
}
EPOCH = 852073200

date_days = 0
function en1543_date(source, position)
	local part =  bytes.sub(source, position, position+13) -- 14 bits
	bytes.pad_left(part,32,0)
	date_days = EPOCH+bytes.tonumber(part)*24*3600
	return os.date("%x",date_days)
end

function en1543_time(source, position)
	local date_minutes
	local part = bytes.sub(source, position, position+10) -- 11 bits
	bytes.pad_left(part,32,0)
	date_minutes = date_days + bytes.tonumber(part)*60
	date_days = 0
	return os.date("%X",date_minutes)
end

function en1543_networkid(source, position)
	local country = bytes.sub(source, position, position+11)
	local region  = bytes.sub(source, position+12, position+23)
	local country_code
	local region_code
	
	country_code = iso_country_code_name(tonumber(tostring(bytes.convert(4,country))))
	region_code  = tonumber(tostring(bytes.convert(4,region)))
	
	return "country "..country_code.." / region "..region_code
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
      	NODE = ui.tree_add_node(ctx,format[3],reference_index)

	if format[1] == en1543_BITMAP then
	   bitmap_size = parsed
	   parsed = bitmap_size 
	   for index=0,format[2]-1 do
	       if data[position+bitmap_size-index-1]~=0 then
	          parsed = parsed + en1543_parse_item(NODE, format[4][index], data, position+parsed, index)
	       end
	   end
	elseif format[1] == en1543_DATE then
	   ui.tree_set_value(NODE,item);
	   ui.tree_set_alt_value(NODE,en1543_date(item,0))
	elseif format[1] == en1543_TIME then
	   ui.tree_set_value(NODE,item);
	   ui.tree_set_alt_value(NODE,en1543_time(item,0)) 
	elseif format[1] == en1543_NUMBER then
	   ui.tree_set_value(NODE,item);
	   ui.tree_set_alt_value(NODE,bytes.tonumber(item))
	elseif format[1] == en1543_NETWORKID then
	   ui.tree_set_value(NODE,item);
	   ui.tree_set_alt_value(NODE,en1543_networkid(item,0))
        elseif format[1] == en1543_REPEAT then
	   ui.tree_set_value(NODE,item);
	   ui.tree_set_alt_value(NODE,bytes.tonumber(item))
           for index=1,bytes.tonumber(item) do
	       parsed = parsed + en1543_parse_item(ctx, format[4][0], data, position+parsed, reference_index+index)
	   end 
	else 
	   ui.tree_set_value(NODE,item);
           hex_info = bytes.convert(8,item)
	   ui.tree_set_alt_value(NODE,"0x"..tostring(hex_info))
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
	local parsed

	NODE_REF = ui.tree_find_node(ctx,"File Event logs")
	
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
	   log.print(log.DEBUG,"No event logs")
	end

	NODE_REF = ui.tree_find_node(ctx,"File Special events")
	
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
	   log.print(log.DEBUG,"No special events")
	end

	NODE_REF = ui.tree_find_node(ctx,"File Environment")

	if NODE_REF then
	   RECORD_REF=ui.tree_find_node(NODE_REF,"record",1)
	   DATA_REF = ui.tree_find_node(RECORD_REF,"raw data")
	   data = bytes.new(8,ui.tree_get_value(DATA_REF))
	   bits = bytes.convert(1,data)
	   parsed = en1543_parse(RECORD_REF,en1543_Env,bits)
	   parsed = en1543_parse(RECORD_REF,en1543_Holder,bytes.sub(bits,parsed))
	else
	   log.print(log.DEBUG,"No environment") 
	end

	NODE_REF = ui.tree_find_node(ctx,"File Contract list")
	
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
	   log.print(log.DEBUG,"No contract list (BestContracts)") 
	end

	NODE_REF = ui.tree_find_node(ctx,"File Contracts")
	
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
	   log.print(log.DEBUG,"No contracts")
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
	local NODE


	sw, resp = card.select("#315449432e494341")

	APP = ui.tree_add_node(cardenv,"Application",DF_NAME,nil,"application")
	-- note: no real DF select here
 
	for lfi,lfi_desc in pairs(LFI_LIST) do
	        sw,resp = card.select("/2000")
		if sw~=0x9000 then 
		        break
		end
		sw,resp = card.select(lfi)
		if sw==0x9000 then
	                local r
			LFI = ui.tree_add_node(APP,"File "..lfi_desc,lfi,nil,"file")
			for r=1,255 do
				sw,resp=card.read_record(0,r,0x1D)
				if sw ~= 0x9000 then
					break
				end
				REC = ui.tree_add_node(LFI,"record",r,#resp,"record")
                                NODE = ui.tree_add_node(REC,"raw data")
	                        ui.tree_set_value(NODE,resp)
			end
		end
	end
	en1543_process(APP)
end

local atr, hex_card_num, card_num

card.connect()

CARD = card.tree_startup("CALYPSO")

atr = card.last_atr();
hex_card_num = bytes.sub(atr,-7,-4)
card_num     = (hex_card_num[0]*256*65536)+(hex_card_num[1]*65536)+(hex_card_num[2]*256)+hex_card_num[3]

local ref = ui.tree_add_node(CARD,"Card number",nil,4,"block")
ui.tree_set_value(ref,hex_card_num)
ui.tree_set_alt_value(ref,card_num)

process_calypso(CARD)

card.disconnect()
