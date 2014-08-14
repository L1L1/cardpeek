--
-- This file is part of Cardpeek, the smartcard reader utility.
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
-- @name Tachograph 
-- @description Driver tachograph cards
-- @targets 0.8.2
--

require('lib.strict');

local Length_expected = "unset"
CLA = 0x00

log.print(log.INFO,"Debut de test");

function tacho_select(fileid)
    local sw

    if Length_expected=="unset" then
        sw = card.select(fileid,0x0C,"")
        if sw==0x9000 then
            Length_expected = ""
        else
            Length_expected = 0x00
            sw = card.select(fileid,0x0C,0x00)
        end
    else
        sw = card.select(fileid,0x0C,Length_expected)
    end
    log.print(log.DEBUG,"Selection du fichier " .. fileid);
    return sw
end

function tacho_perform_hash()
    return card.send(bytes.new(8, "80 2A 90 00"))
end

function tacho_compute_digital_signature()
    return card.send(bytes.new(8, "00 2A 9E 9A 80"))
end

function tacho_read_file(size)
    local pos=0
    local sw, resp
    local expected
    local data = bytes.new(8)
    local progress = 0

    if size > 500 then
        progress = 1
    end

    log.print(log.DEBUG,"Lecture de " .. size .. " octets");
    while pos<size do
        if size-pos<200 then
            expected = size-pos
        else
            expected = 200
        end

        sw, resp = card.read_binary(".",pos,expected);

        if sw~=0x9000 then return sw, data end

        if progress>0 then
            log.print(log.DEBUG,string.format("Lecture du fichier: %.0f%%",100*pos/size))
        end

        data = bytes.concat(data,resp)
        pos = pos + #resp
    end
    log.print(log.DEBUG,"Lecture reussie");
    return sw, data
end

local typeOfTachographCardId 
local cardStructureVersion 
local noOfEventsPerType  
local noOfFaultsPerType 
local activityStructureLength 
local noOfCardVehicleRecords 
local noOfCardPlaceRecords 

function Count_NoOfFaultsPerType()
    return noOfFaultsPerType
end

function Count_NoOfEventsPerType()
    return noOfEventsPerType
end

function Count_ActivityStructureLength()
    return activityStructureLength 
end

function Count_NoOfCardVehicleRecords()
    return noOfCardVehicleRecords
end
 
function Count_NoOfCardPlaceRecords()
    return noOfCardPlaceRecords
end 

TACHO_REGION_TABLE = {
    [0x00] = "None",
    [0x01] = "Andalucía",
    [0x02] = "Aragón",
    [0x03] = "Asturias",
    [0x04] = "Cantabria",
    [0x05] = "Cataluña",
    [0x06] = "Castilla-León",
    [0x07] = "Castilla-La-Mancha",
    [0x08] = "Valencia",
    [0x09] = "Extremadura", 
    [0x0A] = "Galicia", 
    [0x0B] = "Baleares", 
    [0x0C] = "Canarias", 
    [0x0D] = "La Rioja",
    [0x0E] = "Madrid",
    [0x0F] = "Murcia",
    [0x10] = "Navarra",
    [0x11] = "País Vasco"
} 

TACHO_EVENT_TABLE = {
    [0x00] = "No further details",
    [0x01] = "Insertion of a non-valid card",
    [0x02] = "Card conflict",
    [0x03] = "Time overlap",
    [0x04] = "Driving without an appropriate card",
    [0x05] = "Card insertion while driving",
    [0x06] = "Last card session not correctly closed", 
    [0x07] = "Over speeding",
    [0x08] = "Power supply interruption",
    [0x09] = "Motion data error",
    [0x10] = "Vehicle unit related security breach attempt event, No further details",
    [0x11] = "Vehicle unit, Motion sensor authentication failure", 
    [0x12] = "Vehicle unit, Tachograph card authentication failure", 
    [0x13] = "Vehicle unit, Unauthorised change of motion sensor", 
    [0x14] = "Vehicle unit, Card data input integrity error",
    [0x15] = "Vehicle unit, Stored user data integrity error", 
    [0x16] = "Vehicle unit, Internal data transfer error", 
    [0x17] = "Vehicle unit, Unauthorised case opening", 
    [0x18] = "Vehicle unit, Hardware sabotage",
    [0x20] = "Sensor related security breach attempt events, No further details",
    [0x21] = "Sensor, Authentication failure",
    [0x22] = "Sensor, Stored data integrity error",
    [0x23] = "Sensor, Internal data transfer error",
    [0x24] = "Sensor, Unauthorised case opening",
    [0x18] = "Sensor, Hardware sabotage",
    [0x30] = "Recording equipment fault, No further details",
    [0x31] = "Recording equipment, VU internal fault",
    [0x32] = "Recording equipment, Printer fault",
    [0x33] = "Recording equipment, Display fault",
    [0x34] = "Recording equipment, Downloading fault",
    [0x35] = "Recording equipment, Sensor fault",
    [0x40] = "Card faults, No further details",
}

TACHO_NATION_TABLE = {
    [0x00] = "No information available",
    [0x01] = "Austria",
    [0x02] = "Albania",
    [0x03] = "Andorra",
    [0x04] = "Armenia",
    [0x05] = "Azerbaijan",
    [0x06] = "Belgium",
    [0x07] = "Bulgaria",
    [0x08] = "Bosnia and Herzegovina",
    [0x09] = "Belarus",
    [0x0A] = "Switzerland",
    [0x0B] = "Cyprus",
    [0x0C] = "Czech Republic",
    [0x0D] = "Germany",
    [0x0E] = "Denmark",
    [0x0F] = "Spain",
    [0x10] = "Estonia",
    [0x11] = "France",
    [0x12] = "Finland",
    [0x13] = "Liechtenstein",
    [0x14] = "Faeroe Islands",
    [0x15] = "United Kingdom",
    [0x16] = "Georgia",
    [0x17] = "Greece",
    [0x18] = "Hungary",
    [0x19] = "Croatia",
    [0x1A] = "Italy",
    [0x1B] = "Ireland",
    [0x1C] = "Iceland",
    [0x1D] = "Kazakhstan",
    [0x1E] = "Luxembourg",
    [0x1F] = "Lithuania",
    [0x20] = "Latvia",
    [0x21] = "Malta",
    [0x22] = "Monaco",
    [0x23] = "Republic of Moldova",
    [0x24] = "Macedonia",
    [0x25] = "Norway",
    [0x26] = "Netherlands",
    [0x27] = "Portugal",
    [0x28] = "Poland",
    [0x29] = "Romania",
    [0x2A] = "San Marino",
    [0x2B] = "Russian Federation",
    [0x2C] = "Sweden",
    [0x2D] = "Slovakia",
    [0x2E] = "Slovenia",
    [0x2F] = "Turkmenistan",
    [0x30] = "Turkey",
    [0x31] = "Ukraine",
    [0x32] = "Vatican City",
    [0x33] = "Yugoslavia",
    [0xFD] = "European Community",
    [0xFE] = "Rest of Europe",
    [0xFF] = "Rest of the world",
}

TACHO_EQUIPMENT_TYPE = {
    "Driver card",
    "Workshop card",
    "Control card",
    "Company card",
    "Manufacturing card",
    "Vehicule unit",
    "Motion sensor"
}

TACHO_WORK_PERIOD = {
    [0] = "Begin, related time = card insertion time or time of entry",
    [1] = "End, related time = card withdrawal time or time of entry",
    [2] = "Begin, related time manually entered (start time)",
    [3] = "End, related time manually entered (end of work period)",
    [4] = "Begin, related time assumed by VU",
    [5] = "End, related time assumed by VU"
}

function Tacho_WORK_PERIOD(data,node)
    node:set_attribute("alt",TACHO_WORK_PERIOD[data[0]])
end

function Tacho_EQUIPMENT_TYPE(data,node)
    node:set_attribute("alt",TACHO_EQUIPMENT_TYPE[data[0]])
end

function Tacho_EVENT_TYPE(data,node)
    node:set_attribute("alt",TACHO_EVENT_TABLE[data[0]])
end

function Tacho_NATION_NUMERIC(data,node)
    node:set_attribute("alt",TACHO_NATION_TABLE[data[0]])
end

function Tacho_DATE(data,node)
    local time=data:tonumber()
    node:set_attribute("alt",os.date("!%d/%m/%Y",time))
end

function Tacho_TIME(data,node)
    local time=data:tonumber()
    node:set_attribute("alt",os.date("!%H:%M:%S %d/%m/%Y",time))
end

function Tacho_TEXT(data,node)
    node:set_attribute("alt",data:format("%P"))
end 

function Tacho_TEXT_8859(data,node)
    global('iconv')
    
    if iconv and data[0]>0 then
        local format = "ISO-8859-"..data[0]
        local conversion = iconv.open(format,"UTF-8")
        local converted = conversion:iconv(data:sub(1):format("%C"))
        if converted then
            node:set_attribute("alt",converted)
            return
        end
    end
    
    if data[0]==1 then
        node:set_attribute("alt",data:sub(1):format("%P"))
    elseif data[0]==0 then
        node:set_attribute("alt","(empty)")
    end
end 

function Tacho_DATEF(data,node)
    local digits= data:convert(4)
    node:set_attribute("alt",digits:sub(6,7):format("%D").."/"..digits:sub(4,5):format("%D").."/"..digits:sub(0,3):format("%D"))
end

function Tacho_NUMERIC(data,node)
    node:set_attribute("alt",data:tonumber())
end

function Tacho_REGION_NUMERIC(data,node)
    node:set_attribute("alt",TACHO_REGION_TABLE[data[0]])
end

function Tacho_VEHICLE_RECORD_ADJUST(data,node)
    local parent =  node:parent()
    local o_begin = parent:find_first({label='vehicleOdometerBegin'}):get_attribute("val"):tonumber()
    local o_end =   parent:find_first({label='vehicleOdometerEnd'}):get_attribute("val"):tonumber()
    local v_date =  parent:find_first({label='vehicleFirstUse'}):get_attribute("val"):tonumber()
    local v_reg =   parent:find_first({label='vehicleRegistrationNumber'}):get_attribute("alt")
    
    parent:set_attribute("val", string.format("%s: %d km, %s", os.date("!%d/%m/%Y",v_date), o_end-o_begin, v_reg))
end

function Tacho_ActivityChangeInfo(data,node)
    local summary
    local time = bit.AND(data:tonumber(),0x07FF)
    local sub_node = node:append({  classname='record',
                                    label='Change', 
                                    id=string.format("%02u:%02u",time/60,time%60),
                                    val=data,
                                    size=2 })
    local activity = bit.SHR(bit.AND(data[0],0x18),3)
    
    node:set_attribute("val",nil)

    if activity==00 then
        sub_node:append({label='activity', val='0', alt='break/rest'})
        summary = "break of "
    elseif activity==01 then
        sub_node:append({label='activity', val='1', alt='availabilty'})
        summary = "availability of "
    elseif activity==02 then
        sub_node:append({label='activity', val='2', alt='work'})
        summary = "work of "
    else
        sub_node:append({label='activity', val='3', alt='driving'}) 
        summary = "driving of "
    end

    if bit.AND(data[0],0x80)==0 then
        sub_node:append({label='slot', val='0', alt='driver'})
        summary = summary .. "driver"
    else
        sub_node:append({label='slot', val='1', alt='co-driver'})
        summary = summary .. "co-driver"
    end

    if bit.AND(data[0],0x40)==0 then
        sub_node:append({label='driving status', val='0', alt='single'})
    else
        sub_node:append({label='driving status', val='1', alt='crew'})
    end

    if bit.AND(data[0],0x20)==0 then
        sub_node:append({label='card status', val='0', alt='inserted'})
    else
        sub_node:append({label='card status', val='1', alt='not inserted'})
    end
    
    if time==0 then
        sub_node:set_attribute("alt","(Initial state: " .. summary .. ")")
    else
        sub_node:set_attribute("alt","(Update:" .. summary .. ")")
    end

    return time, 1+activity
end

function subpart(data,first,last)
    if first>=#data then
        return data:sub(first-#data,last-#data)
    end
    if last<#data then
        return data:sub(first,last)
    else
        return data:sub(first,#data-1) .. data:sub(0,last-#data)
    end
end

function Tacho_ACTIVITY_RECORDS(data,node)
    local subnode
    local newest = node:parent():find_first({label="activityPointerNewestRecord"}):get_attribute("val"):tonumber()
    local oldest = node:parent():find_first({label="activityPointerOldestDayRecord"}):get_attribute("val"):tonumber()
    local ptr = oldest
    local counter = 1
   
    node:set_attribute("classname","record")
    node:set_attribute("val",nil)

    --print("newest: "..newest);
    --print("oldest: "..oldest);

    while true do
        local rec_len = subpart(data,ptr+2,ptr+3):tonumber()
        local activity = subpart(data,ptr+12,ptr+rec_len-1)
        local subsub_node
        local cat_total = { 0, 0, 0, 0 }
        local cat_ptr = 0
        local cur_time = 0 
        local old_time = 0
        local cat_cur = 0
        local rec_date = os.date("!%d/%m/%Y", subpart(data,ptr+4,ptr+7):tonumber())

        subnode = node:append({classname='record',label='CardActivityDailyRecord', size=rec_len, id=counter})
        subnode:append({classname='item',
                        label='activityRecordLength',
                        val=subpart(data,ptr+2,ptr+3),
                        alt=string.format("%d (address:%s)",rec_len,ptr)})
        subnode:append({classname='item',
                        label='activityPreviousRecordLength',
                        val=subpart(data,ptr,ptr+1),
                        alt=subpart(data,ptr,ptr+1):tonumber()})
        subnode:append({classname='item',
                        label='activityRecordDate',
                        val=subpart(data,ptr+4,ptr+7),
                        alt=rec_date})
        subnode:append({classname='item',
                        label='dailyPresenceCounter',
                        val=subpart(data,ptr+8,ptr+9),
                        alt=subpart(data,ptr+8,ptr+9):tonumber() })
        subnode:append({classname='item',
                        label='activityDayDistance',
                        val=subpart(data,ptr+10,ptr+11),
                        alt=subpart(data,ptr+10,ptr+11):tonumber() .. " km"})
        subsub_node = subnode:append({classname='record',
                        label='activityChangeInfo',
                        size=#activity,
                        val=activity})

        if #activity>0 then
            for i=0,#activity-1,2 do
                cur_time, cat_cur = Tacho_ActivityChangeInfo(activity:sub(i,i+1),subsub_node)
                if cat_ptr~=0 then
                    cat_total[cat_ptr] = cat_total[cat_ptr]+(cur_time-old_time)
                end 
                cat_ptr = cat_cur
                old_time = cur_time
            end

            cat_total[cat_ptr]=cat_total[cat_ptr]+1440-cur_time
            subnode:set_attribute("alt",string.format("%s: %d km\n  %dh%02d break, %dh%02d availability, %dh%02d work, %dh%02d drive",
                    rec_date,
                    subpart(data,ptr+10,ptr+11):tonumber(),
                    cat_total[1]/60,  cat_total[1]%60,
                    cat_total[2]/60,  cat_total[2]%60,
                    cat_total[3]/60,  cat_total[3]%60,
                    cat_total[4]/60,  cat_total[4]%60))
        else
             subnode:set_attribute("alt","(no activity)")
        end         

        --print(string.format("current ptr=%d, next ptr=%d, rec_len=%d, act_len=%d",ptr,ptr+rec_len,rec_len,#activity))

        if ptr==newest then
            break
        end
        ptr = (ptr + rec_len)%#data
        counter=counter+1
    end
end

local TACHO_MAP = 
{
    ["EF_ICC"] = 
    { { "CardIccIdentification", "record", 1, {
            { "clockStop", "item", 1, 0 },
            { "cardExtendedSerialNumber", "item", 8, 0 },
            { "cardApprovalNumber", "item", 8, Tacho_TEXT },
            { "cardPersonaliserID", "item", 1, 0 },
            { "embedderIcAssemblerId", "item", 5, 0 },
            { "icIdentifier", "item", 2, 0 } }
    } },
    ["EF_IC"] =
    { { "CardChipIdentification", "record", 1, {
            { "icSerialNumber", "item", 4, 0 },
            { "icManufacturingReferences", "item", 4, 0 } }
    } },
    ["EF_Application_Identification"] = 
    { { "DriverCardApplicationIdentification", "record", 1, {
            { "typeOfTachographCardId", "item", 1, Tacho_EQUIPMENT_TYPE },
            { "cardStructureVersion", "item", 2, 0 },
            { "noOfEventsPerType", "item", 1, Tacho_NUMERIC  },
            { "noOfFaultsPerType", "item", 1, Tacho_NUMERIC  },
            { "activityStructureLength", "item", 2, Tacho_NUMERIC  },
            { "noOfCardVehicleRecords", "item", 2, Tacho_NUMERIC  },
            { "noOfCardPlaceRecords", "item", 1, Tacho_NUMERIC  } }
    } },
    ["EF_Card_Certificate"] = 
    { { "cardCertificate", "item", 194, 0 
    } },
    ["EF_CA_Certificate"] = 
    { { "memberStateCertificate", "item", 194, 0 
    } },
    ["EF_Identification"] = 
    { { "CardIdentification", "record", 1, {
            { "cardIssuingMemberState", "item", 1, Tacho_NATION_NUMERIC },
            { "cardNumber", "item", 16, Tacho_TEXT },
            { "cardIssuingAuthorityName", "item", 36, Tacho_TEXT_8859 },
            { "cardIssueDate", "item", 4,  Tacho_TIME  },
            { "cardValidityBegin", "item", 4, Tacho_TIME  },
            { "cardExpiryDate", "item", 4, Tacho_TIME  } }
       },
       { "DriverCardHolderIdentification", "record", 1, {
            { "CardHolderName", "record", 1, {
                { "hoderSurname", "item", 36, Tacho_TEXT_8859 },
                { "hoderFirstNames", "item", 36, Tacho_TEXT_8859 } }
            },
            { "cardHolderBirthDate", "item", 4, Tacho_DATEF },
            { "cardHolderPreferredLanguage", "item", 2, Tacho_TEXT } }
    } },
    ["EF_Card_Download"] = 
    { { "lastCardDownload", "item", 4, Tacho_TIME 
    } }, 
    ["EF_Driving_Licence_info"] = 
    { { "CardDrivingLicenceInformation", "record", 1, {
            { "drivingLicenceIssuingAuthority", "item", 36, Tacho_TEXT_8859 },
            { "drivingLicenceIssuingNation", "item", 1, Tacho_NATION_NUMERIC },
            { "drivingLicenceNumber", "item", 16, Tacho_TEXT } } 
    } },
    ["EF_Events_Data"] = 
    { { "CardEventData", "record", 1, {
            { "CardEventRecords", "record", 6, {
                { "CardEventRecord", "record", Count_NoOfEventsPerType, {
                    { "eventType", "item", 1, Tacho_EVENT_TYPE },
                    { "eventBeginTime", "item", 4, Tacho_TIME },
                    { "eventEndtime", "item", 4, Tacho_TIME },
                    { "EventVehicleRegistration", "record", 1, {
                        { "vehiculeRegistrationNation", "item", 1, Tacho_NATION_NUMERIC },
                        { "vehiculeRegistrationNumber", "item", 14, Tacho_TEXT_8859 } } 
                    } }
                } }
            } }
    } },
    ["EF_Faults_Data"] = 
    { { "CardFaultData", "record", 1, {
            { "CardFaultsRecords", "record", 2, {
                { "CardFaultRecord", "record", Count_NoOfFaultsPerType, {
                    { "faultType", "item", 1, Tacho_EVENT_TYPE },
                    { "faultBeginTime", "item", 4, Tacho_TIME },
                    { "faultEndtime", "item", 4, Tacho_TIME },
                    { "FaultVehicleRegistration", "record", 1, {
                        { "vehiculeRegistrationNation", "item", 1, Tacho_NATION_NUMERIC },
                        { "vehiculeRegistrationNumber", "item", 14, Tacho_TEXT_8859 } } 
                    } }
                } }
            } }        
    } },
    ["EF_Driver_Activity_Data"] = 
    { { "CardDriverActivity", "record", 1, {
            { "activityPointerOldestDayRecord", "item", 2, Tacho_NUMERIC },
            { "activityPointerNewestRecord", "item", 2, Tacho_NUMERIC },
            { "activityDailyRecords", "item", Count_ActivityStructureLength, Tacho_ACTIVITY_RECORDS } 
        }
    } },
    ["EF_Vehicles_Used"] = 
    { { "CardVehiclesUsed", "record", 1, {
            { "vehiclePointerNewestRecord", "item", 2, 0 },
            { "CardVehicleRecords", "record", 1, {
                { "CardVehicleRecord", "record", Count_NoOfCardVehicleRecords, {
                    { "vehicleOdometerBegin", "item", 3, Tacho_NUMERIC }, 
                    { "vehicleOdometerEnd", "item", 3, Tacho_NUMERIC }, 
                    { "vehicleFirstUse", "item", 4, Tacho_TIME }, 
                    { "vehicleLastUse", "item", 4, Tacho_TIME }, 
                    { "vehicleRegistration", "record", 1, {
                        { "vehicleRegistrationNation", "item", 1, Tacho_NATION_NUMERIC }, 
                        { "vehicleRegistrationNumber", "item", 14, Tacho_TEXT_8859 } 
                    } },
                    { "vehicleDataBlockCounter", "item", 2, Tacho_VEHICLE_RECORD_ADJUST } 
                } }  
            } }
        }
    } },
    ["EF_Places"] = 
    { { "CardPlaceDailyWorkPeriod", "record", 1, {
            { "placePointerNewestRecord", "item", 1, 0 },
            { "PlaceRecords", "record", 1, {
                { "PlaceRecord", "record", Count_NoOfCardPlaceRecords, {
                    { "entryTime", "item", 4, Tacho_TIME },
                    { "entryDailyWorkPeriod", "item", 1, Tacho_WORK_PERIOD }, 
                    { "dailyWorkPeriodCountry", "item", 1, Tacho_NATION_NUMERIC }, 
                    { "dailyWorkPeriodRegion", "item", 1, Tacho_REGION_NUMERIC }, 
                    { "vehiculeOdometerValue", "item", 3, Tacho_NUMERIC }
                } }
            } }
        }
    } },
    ["EF_Current_Usage"] = 
    { { "CardCurrentUse", "record", 1, {
            { "sessionOpenTime", "item", 4, Tacho_TIME },
            { "SessionOpenVehicle", "record", 1, {
                 { "vehicleRegistrationNation", "item", 1, Tacho_NATION_NUMERIC }, 
                 { "vehicleRegistrationNumber", "item", 14, Tacho_TEXT_8859 } 
            } }
        } 
    } },
    ["EF_Control_Activity_Data"] = 
    { { "CardControlActivityDataRecord", "record", 1, {
            { "controlType", "item", 1, 1 },
            { "controlTime", "item", 4, Tacho_TIME },
            { "ControlCardNumber", "record", 1, {
                { "cardType", "item", 1, 1 },
                { "cardIssuingMemberState", "item", 1, Tacho_NATION_NUMERIC },
                { "cardNumber", "item", 16, Tacho_TEXT }
            } },  
            { "ControlVehicleRegistration", "record", 1, {
                 { "vehicleRegistrationNation", "item", 1, Tacho_NATION_NUMERIC }, 
                 { "vehicleRegistrationNumber", "item", 14, Tacho_TEXT_8859 } 
            } },
            { "controlDownloadPeriodBegin", "item", 4, Tacho_TIME },
            { "controlDownloadPeriodEnd", "item", 4, Tacho_TIME }
        }
    } },
    ["EF_Specific_Conditions"] = 
    { { "SepcificConditionRecord", "record", 1, {
            { "SpecificConditionRecord", "record", 56, {
                { "entryTime", "item", 4, Tacho_TIME },
                { "specificConditionType", "item", 1, 1 } 
            } }
        }
    } }
}

function tacho_map_ex(map, data, node)

    for i,v in ipairs(map) do
        local item 
        local count        

        if #v~=4 then
            log.print(log.ERROR,"The mapping for " .. v[1] .. " is incomplete")
            return nil
        end

        if not data or #data==0 then
            return data
        end

        if type(v[3])=='function' then
            count = v[3]()
        else
            count = v[3]
        end

        if v[2]=='record' then
           for j=1,count do
                item = node:append({label=v[1], classname=v[2]})
                data = tacho_map_ex(v[4], data, item)
                
                if count>1 then
                    item:set_attribute("id",j)
                end
             end 
        else
            local data_sub = data:sub(0,count-1)
            
            item = node:append({label=v[1], classname=v[2], val=data_sub})
            data = data:sub(count)

            if type(v[4])=="function" then
                v[4](data_sub,item)
            end
            
            item:set_attribute("size",count)
        end
    end
    return data
end

function tacho_map(efname, data, node)
    local map = TACHO_MAP[efname]

    if not map then
        log.print(log.ERROR,efname .. " is not a recognized EF name")
        return file_node
    end

    local remains = tacho_map_ex(map,data,node)
    
    if remains and #remains>0 then
        log.print(log.ERROR,"Unparsed data remains in " .. efname .. ": " .. #remains)
    end
end

local ddd = bytes.new(8)

function ddd_append_data(ef, data)
    ddd = bytes.concat(ddd,ef,00,bit.SHR(#data,8),bit.AND(#data,0xFF),data)
end

function ddd_append_signature(ef, signature)
    ddd = bytes.concat(ddd,ef,01,bit.SHR(#signature,8),bit.AND(#signature,0xFF),signature)
end

function ddd_save(card)
    local user_name
    local user_surname
    local current_date
    local fpath, fname

    user_name = card:find_first({label='hoderSurname'}):get_attribute("alt")
    user_surnames = card:find_first({label='hoderFirstNames'}):get_attribute("alt")
    current_date = os.date('%Y-%m-%d')

    fname = user_name .. user_surnames .. current_date .. ".ddd" 

    fname = fname:gsub("%s%s*","_"):lower()

    fpath, fname = ui.select_file("Save ESM file as",nil,fname)

    if fname then
        file = io.open(fname,"wb")
        for i=0,#ddd-1 do
            file:write(string.format("%c",ddd[i]))
        end
        file:close()
        log.print(log.INFO,"Saved card data in " .. fname);
        return true
    end
    log.print(log.INFO,"Card data not saved");
    return false
end

local MAP_TABLE = {}

function tacho_read_file_and_store(node,fid,length,fname,cname, with_storage, with_signature)
    local h_sw
    local sw, resp
    local s_sw, s_resp
    local sub_node

    sw = tacho_select(fid)
    sub_node = node:append({classname=cname,label=fname,id=fid})

    if with_signature==true then
        h_sw = tacho_perform_hash()
        if h_sw~=0x9000 then
            log.print(log.WARNING,"Perform hash failed for file " .. fid)
        end
    end

    if sw==0x9000 and length>0 then
        sw, resp = tacho_read_file(length)
        if sw==0x9000 then
            table.insert(MAP_TABLE,{ sub_node, fid, length, fname, cname, resp })

            if with_storage==true then
                ddd_append_data(fid:sub(2),resp)
            end

            if with_signature==true and h_sw==0x9000 then
                s_sw, s_resp = tacho_compute_digital_signature()
                if s_sw==0x9000 then
                    ddd_append_signature(fid:sub(2),s_resp)
                else
                    log.print(log.WARNING,"Compute digital signature failed for file " .. fid)
                end
            end
        else
            sub_node:set_attribute("alt",string.format("File read error: %x",sw)) 
        end
    end

    return sw, resp, sub_node
end

if card.connect() then
    local resp
    local sw 
    local CARD = card.tree_startup("Tachograph") 
    local TACHO
    local answer
    local export_signed
    local export_ddd

    answer = ui.question("Do you whish to export content of this card in an ESM file\n(also called DDD or C1B)?", 
                         {"Export signed data file", "Export unsigned data file", "No, don't export"})

    if answer==1 then
        export_signed = true
        export_ddd = true
    elseif answer==2 then   
        export_signed = false
        export_ddd = true
    else
        export_signed = false
        export_ddd = false
    end

    tacho_read_file_and_store(CARD,".0002",25,"EF_ICC","file",true,false)

    tacho_read_file_and_store(CARD,".0005",8,"EF_IC","file",true,false)

    sw,resp,TACHO = tacho_read_file_and_store(CARD,"#FF544143484F",0,"DF_Tachograph","application",false,false)

    sw,resp = tacho_read_file_and_store(TACHO,".0501",10,"EF_Application_Identification","file",true,export_signed)

    if sw==0x9000 then
        typeOfTachographCardId = resp[0]
        noOfEventsPerType = resp[3]               
        noOfFaultsPerType = resp[4]
        activityStructureLength = resp[5]*256 + resp[6]
        noOfCardVehicleRecords = resp[7]*256 + resp[8]
        noOfCardPlaceRecords = resp[9]

        if typeOfTachographCardId~=1 then
            ui.question("This is not a tachograph DRIVER card.\nThis script may not work on this card.",{"OK"})
        end

        tacho_read_file_and_store(TACHO,".C100",194,"EF_Card_Certificate","file", true, false)

        tacho_read_file_and_store(TACHO,".C108",194, "EF_CA_Certificate", "file", true, false)
        
        tacho_read_file_and_store(TACHO,".0520",143,"EF_Identification", "file", true, export_signed)
    
        tacho_read_file_and_store(TACHO,".050E",4,"EF_Card_Download","file", true, false)

        tacho_read_file_and_store(TACHO,".0521",53,"EF_Driving_Licence_info","file", true, export_signed)

        tacho_read_file_and_store(TACHO,".0502",noOfEventsPerType*24*6,"EF_Events_Data","file", true, export_signed)

        tacho_read_file_and_store(TACHO,".0503",noOfFaultsPerType*24*2,"EF_Faults_Data","file", true, export_signed)

        tacho_read_file_and_store(TACHO,".0504",activityStructureLength+4,"EF_Driver_Activity_Data","file", true, export_signed)

        tacho_read_file_and_store(TACHO,".0505",noOfCardVehicleRecords*31+2,"EF_Vehicles_Used","file", true, export_signed)

        tacho_read_file_and_store(TACHO,".0506",noOfCardPlaceRecords*10+1,"EF_Places","file", true, export_signed)

        tacho_read_file_and_store(TACHO,".0507",19,"EF_Current_Usage","file", true, export_signed)

        tacho_read_file_and_store(TACHO,".0508",46,"EF_Control_Activity_Data","file", true, export_signed)

        tacho_read_file_and_store(TACHO,".0522",280,"EF_Specific_Conditions","file", true, export_signed)

        for i,v in ipairs(MAP_TABLE) do
            if v[3]>0 then
                tacho_map(v[4],v[6],v[1])
            end           
        end

        -- cleanup empty events:
        for node in TACHO:find({label='eventType'}) do
            if node:get_attribute("val"):get(0)==0 then
                node:parent():remove()
            end
        end
        for node in TACHO:find({label='faultType'}) do
            if node:get_attribute("val"):get(0)==0 then
                node:parent():remove()
            end
        end
        for node in TACHO:find({label='specificConditionType'}) do
            if node:get_attribute("val"):get(0)==0 then
                node:parent():remove()
            end
        end

        if export_ddd==true then
            ddd_save(CARD)
        end

    end

    card.disconnect();
end

log.print(log.INFO,"End of script");

