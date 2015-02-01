--
-- This file is part of Cardpeek, the smart card reader utility.
--
-- Copyright 2009-2013 by Alain Pannetrat <L1L1@gmx.com>
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
-- This file is based on c376n2.lua, which was authored and contributed 
-- to cardpeek by Anthony Berkow (C) 2013
-- 
-- Some minor alterations of the original file were performed for 
-- integration with calypso.lua
-- 
-- Jan 2015:
-- This file c376n3.lua was created to take into account some specific
-- isses discovered with a new type of RavKav card with network id = 3
-- and which does not return a FCI. 
-- This induced a change in RavKav_getRecordInfo()
-- 
--********************************************************************--

require('lib.apdu')
require('lib.tlv')
require('lib.en1545')
require('lib.country_codes')
require('etc.ravkav-strings')

--Classes
ISO_CLS_STD     = 0x00  --Structure and coding of command and response according to ISO/IEC 7816 5.4.1 (plus optional secure messaging and logical channel)
ISO_CLS_PRO9    = 0x90  --As for ISO_CLS_STD but the coding and meaning of command and response are proprietary
--Class modifiers (SM)
ISO_CLS_SM_PRO  = 0x04  --Proprietary secure messaging format

--Application ID
AID_RavKav = "#315449432e494341"    --"1TIC.ICA"

--LIDs
CALYPSO_LID_ENVIRONMENT = "2001"    --SFI=0x07, linear, 1 record
CALYPSO_LID_EVENTS_LOG  = "2010"    --SFI=0x08, cyclic, 3 records
CALYPSO_LID_CONTRACTS   = "2020"    --SFI=0x09, linear, 4 records
CALYPSO_LID_COUNTERS    = "2069"    --SFI=0x19, counters, 9 counters

LID_LIST = {
    {"General Information", CALYPSO_LID_ENVIRONMENT,    "environment"},
    {"Counters",            CALYPSO_LID_COUNTERS,       "counters"},
    {"Latest Travel",       CALYPSO_LID_EVENTS_LOG,     "event"},
    {"Contracts",           CALYPSO_LID_CONTRACTS,      "contract"}
}

function ravkav_parse_serial_number(node,data)
	nodes.set_attribute(node,"val", data)
	nodes.set_attribute(node,"alt", string.format("%010d",bytes.tonumber(data)))
end

function ravkav_parse_ats(node,data)
    if data and 5 <= #data then
        local byteOffset = 1
        byteOffset = RavKav_parseBits(data, byteOffset,             1, node, "File type",    en1545_NUMBER)
        byteOffset = RavKav_parseBits(data, byteOffset,             1, node, "EF type",      en1545_NUMBER)
        byteOffset, recordLen = RavKav_parseBits(data,byteOffset,   1, node, "Record size",  en1545_NUMBER)
        byteOffset, numRecords = RavKav_parseBits(data,byteOffset,  1, node, "Record count", en1545_NUMBER)
        byteOffset = RavKav_parseBits(data, byteOffset,             4, node, "Access",       en1545_UNDEFINED)
    end
end

--Tags
RAVKAV_IDO = {
    ['A5/BF0C'] = {"Secure messaging data"},
    ['BF0C/C7'] = {"Serial number", ravkav_parse_serial_number}, 
    ['85']      = {"Record Info", ravkav_parse_ats},
}

ValidUntilOption = {
    None = 0,
    Date = 1,
    EndOfService = 2,
    Cancelled = 3
}

function bytes.is_all(bs, byte)
	if 0 == #bs then return false end
	local i
	for i = 0, #bs - 1 do
		if bs[i] ~= byte then return false end
	end
	return true
end

function RavKav_addTextNode(ctx, a_label, text)
    local REF = nodes.append(ctx, { classname="item", label=a_label })
    nodes.set_attribute(REF,"alt", text)
end

function RavKav_selectApplication(root)
    card.CLA = ISO_CLS_STD
	local sw, resp = card.select(AID_RavKav, card.SELECT_RETURN_FIRST + card.SELECT_RETURN_FCI, 0)
    if 0x9000 ~= sw then
		log.print(log.ERROR, "Failed to select Calypso application.")
        return nil
    end

    local APP_REF = nodes.append(root, { classname="application", label="RavKav Card", size=#resp })
    nodes.set_attribute(APP_REF,"val", resp)
    return APP_REF
end

function RavKav_readCalypsoEf(lid, ctx)
    card.CLA = ISO_CLS_STD
	local sw, resp = card.select("."..lid)
    if 0x9000 ~= sw then
		log.print(log.ERROR, "Failed to select EF "..lid)
    else
        tlv_parse(ctx, resp)
    end
end

function RavKav_parseEf(ctx)
    local numRecords, recordLen
    local RECINFO_REF, ri_data = RavKav_getField(ctx, "Record Info")
    if ri_data and 5 <= #ri_data then
        --recordLen = ri_data[3]
        --numRecords = ri_data[4]
        local byteOffset = 1
        byteOffset = RavKav_parseBits(ri_data, byteOffset,             1, RECINFO_REF, "File type",    en1545_NUMBER)
        byteOffset = RavKav_parseBits(ri_data, byteOffset,             1, RECINFO_REF, "EF type",      en1545_NUMBER)
        byteOffset, recordLen = RavKav_parseBits(ri_data,byteOffset,   1, RECINFO_REF, "Record size",  en1545_NUMBER)
        byteOffset, numRecords = RavKav_parseBits(ri_data,byteOffset,  1, RECINFO_REF, "Record count", en1545_NUMBER)
        byteOffset = RavKav_parseBits(ri_data, byteOffset,             4, RECINFO_REF, "Access",       en1545_UNDEFINED)
        log.print(log.DBG, string.format("numRecords: %d, recordLen: %d", numRecords, recordLen))
    end
    return numRecords, recordLen
end

function RavKav_getRecordInfo(ctx)
    local numRecords = 0
    local recordLen = 0
    local RECINFO_REF = nodes.find_first(ctx, {label="Record Info"})
    if RECINFO_REF then
        numRecords = RavKav_getFieldAsNumber(RECINFO_REF, "Record count")
        recordLen = RavKav_getFieldAsNumber(RECINFO_REF, "Record size")
    else
        local iter
        for iter in nodes.find(ctx,{label="record"}) do
            numRecords = numRecords + 1
        end
        recordLen = 29
    end
    return numRecords, recordLen
end

function RavKav_readRecord(nRec, recordLen)
	card.CLA = ISO_CLS_PRO9 + ISO_CLS_SM_PRO
	local sw, resp = card.read_record(0, nRec, recordLen)
    if 0x9000 ~= sw then
		log.print(log.ERROR, string.format("Failed to read record %d", nRec))
        return nil
    end
    return resp
end

-- Note: this function can work with either bits or bytes depending on the data format
function RavKav_parseBits(data, bitOffset, nBits, REC, a_label, func, a_id)
	local valData = bytes.sub(data, bitOffset, bitOffset + nBits - 1)
	local value = func(valData)
	local intVal = bytes.tonumber(valData)
	local NEW_REF = nil
	if REC then
	    NEW_REF = nodes.append(REC, {classname="item", label=a_label, id=a_id, size=nBits })
        nodes.set_attribute(NEW_REF,"val", bytes.convert(valData,8))
        nodes.set_attribute(NEW_REF,"alt", value)
    end
	return bitOffset + nBits, value, NEW_REF, intVal
end

function RavKav_rkDaysToSeconds(dateDaysPart)
    bytes.pad_left(dateDaysPart, 32, 0)
    return EPOCH + bytes.tonumber(dateDaysPart) * 24 * 3600
end

-- The bits in dateDaysPart have been inverted.
function RavKav_rkInvertedDaysToSeconds(dateDaysPart)
    bytes.pad_left(dateDaysPart, 32, 0)
    return EPOCH + bit.XOR(bytes.tonumber(dateDaysPart), 0x3fff) * 24 * 3600
end

-- Return the date (in seconds) corresponding to the last day of the month for the given date (in seconds).
-- If addMonths > 0 then the month is incremented by addMonths months.
function RavKav_calculateMonthEnd(dateSeconds, addMonths)
    local dt = os.date("*t", dateSeconds)

    if 0 < addMonths then
        dt.month = dt.month + addMonths
        local addYears = math.floor(dt.month / 12)
        dt.year = dt.year + addYears
        dt.month = dt.month - addYears * 12
    end

    dt.day = 28
    local tmp_dt = dt
    repeat
        local date_s = os.time(tmp_dt) + 24 * 3600 --add 1 day
        tmp_dt = os.date("*t",date_s)
        if 1 ~= tmp_dt.day then --did not wrap round to a new month
            dt.day = dt.day + 1
        end
    until 1 == tmp_dt.day

    return os.time(dt)
end

function RavKav_INVERTED_DATE(source)
    return os.date("%x", RavKav_rkInvertedDaysToSeconds(source))
end

function RavKav_PERCENT(source)
    return string.format("%u %%", bytes.tonumber(source))
end

-- source is in BCD!
function RavKav_COUNTRY(source)
    local countryId = bytes.tonumber(source)
    countryId = tonumber(string.format("%X", countryId))
    return iso_country_code_name(countryId)
end

function RavKav_ISSUER(source)
    local issuerId = bytes.tonumber(source)
    local issuer = RAVKAV_ISSUERS[issuerId]
    if issuer then return issuer end
    return tostring(issuerId)
end

function RavKav_PROFILE(source)
    local profileId = bytes.tonumber(source)
    local profile = RAVKAV_PROFILES[profileId]
    if profile then return profile end
    return tostring(profileId)
end

function RavKav_ROUTE(source)
    local routeSystemId = bytes.tonumber(source)
    local route = RAVKAV_ROUTES[routeSystemId]
    if route then return route end
    return tostring(routeSystemId)
end

function RavKav_LOCATION(source)
    local locationId = bytes.tonumber(source)
    local location = RAVKAV_LOCATIONS[locationId]
    if location then return location end
    return tostring(locationId)
end

function RavKav_VALIDITY_TYPE(source)
    local validityType = bytes.tonumber(source)
    local validity = RAVKAV_VALIDITY_TYPES[validityType]
    if validity then return validity end
    return tostring(validityType)
end

function RavKav_CONTRACT_TYPE(source)
    local contractType = bytes.tonumber(source)
    local contract = RAVKAV_CONTRACT_TYPES[contractType]
    if contract then return contract end
    return tostring(contractType)
end

-- Returns CSV list of which bits are set
function RavKav_ZONES(source)
    local zonesPart = bytes.sub(source, 0, 11) --12 bits
    local zones = bytes.tonumber(zonesPart)
    local haveZones = false
    local zoneList = ""
    local i
    for i = 1, 12 do
        if 1 == bit.AND(zones, 1) then
            if haveZones then zoneList = zoneList.."," end
            zoneList = zoneList..tostring(i)
            haveZones = true
        end
        zones = bit.SHR(zones, 1)
    end
    return zoneList
end

function RavKav_readRecords(EF_REF, numRecords, recordLen, rec_desc)
    local nRec
    for nRec = 1, numRecords do
        local record = RavKav_readRecord(nRec, recordLen)
        if record then
            local REC_REF = nodes.append(EF_REF, {classname="record", label=rec_desc, id=nRec, size=#record})
            nodes.set_attribute(REC_REF,"val", record)
        end
    end
end

function RavKav_readFile(APP_REF, desc, lid, rec_desc)
	log.print(log.DBG, "Reading "..desc.."...")
    local EF_REF = nodes.append(APP_REF, {classname="file", label=desc})
    RavKav_readCalypsoEf(lid, EF_REF)
    local numRecords, recordLen = RavKav_parseEf(EF_REF)
    if numRecords then
        RavKav_readRecords(EF_REF, numRecords, recordLen, rec_desc)
    end
end

function RavKav_getField(SRC_REF, a_label, a_id)
    local data, text
    local REF = nodes.find_first(SRC_REF, {label=a_label, id=a_id})
    if REF then
        data = nodes.get_attribute(REF,"val")
        text = nodes.get_attribute(REF,"alt")
    end
    return REF, data, text
end

function RavKav_getFieldAsNumber(SRC_REF, label, id)
    local REF, data, text = RavKav_getField(SRC_REF, label, id)
    if data then return bytes.tonumber(data) end
    return 0
end

function RavKav_getFieldsAsNumberAry(SRC_REF, a_label, a_id)
    local node
    local ary = {}

    for node in nodes.find(SRC_REF, { label=a_label, id=a_id }) do
        local data = nodes.get_attribute(node,"val")
        if data then table.insert(ary, bytes.tonumber(data)) end
    end
    return ary
end

-- if ary is supplied then only create new node if the field is a pointer to a valid entry in ary
function RavKav_copyField(SRC_REF, DEST_REF, a_label, a_id, ary)
    local SRC_REF2, data, text = RavKav_getField(SRC_REF, a_label, a_id)
    if nil == data then return nil, nil end

    if ary then
        local dataId = en1545_NUMBER(data)
        if nil == ary[dataId] then return nil, nil end
    end

    NEW_REF = nodes.append(DEST_REF, {classname="item", label=a_label, id=a_id, size=#data })
    nodes.set_attribute(NEW_REF,"val", data)
    nodes.set_attribute(NEW_REF,"alt", text)
    return SRC_REF2, NEW_REF
end

function RavKav_parseEnvironment(ENV_REF, nRec)
    local ENV_REC_REF = nodes.find_first(ENV_REF, {label="record", id=nRec})
    if nil == ENV_REC_REF then return false end

    local record = nodes.get_attribute(ENV_REC_REF,"val")
    if nil == record then return false end

    local data = bytes.convert(record,1)

    local bitOffset = 0
    bitOffset = RavKav_parseBits(data, bitOffset,  3, ENV_REC_REF, "Version number",        en1545_NUMBER)
    bitOffset = RavKav_parseBits(data, bitOffset, 12, ENV_REC_REF, "Country",               RavKav_COUNTRY)
    bitOffset = RavKav_parseBits(data, bitOffset,  8, ENV_REC_REF, "Issuer",                RavKav_ISSUER)
    bitOffset = RavKav_parseBits(data, bitOffset, 26, ENV_REC_REF, "Application number",    en1545_NUMBER)
    bitOffset = RavKav_parseBits(data, bitOffset, 14, ENV_REC_REF, "Date of issue",         en1545_DATE)
    bitOffset = RavKav_parseBits(data, bitOffset, 14, ENV_REC_REF, "End date",              en1545_DATE)
    bitOffset = RavKav_parseBits(data, bitOffset,  3, ENV_REC_REF, "Pay method",            en1545_NUMBER)
    bitOffset = RavKav_parseBits(data, bitOffset, 32, ENV_REC_REF, "Date of birth",         en1545_BCD_DATE)
    bitOffset = RavKav_parseBits(data, bitOffset, 14, ENV_REC_REF, "Company (not set)",     en1545_NUMBER)
    bitOffset = RavKav_parseBits(data, bitOffset, 30, ENV_REC_REF, "Company ID (not set)",  en1545_NUMBER)
    bitOffset = RavKav_parseBits(data, bitOffset, 30, ENV_REC_REF, "ID number",             en1545_NUMBER)

    local PROFILES_REF = nodes.append(ENV_REC_REF, {classname="item", label="Profiles"})
    local profile, ref
    bitOffset, profile, ref = RavKav_parseBits(data, bitOffset,  6, PROFILES_REF,   "Profile",      RavKav_PROFILE, 1)
    bitOffset               = RavKav_parseBits(data, bitOffset, 14, ref,            "Valid until",  en1545_DATE)
    bitOffset, profile, ref = RavKav_parseBits(data, bitOffset,  6, PROFILES_REF,   "Profile",      RavKav_PROFILE, 2)
                              RavKav_parseBits(data, bitOffset, 14, ref,            "Valid until",  en1545_DATE)
    return true
end

function RavKav_parseGeneralInfo(APP_REF)
	log.print(log.DBG, "Parsing general info (environment)...")
    local ENV_REF = nodes.find_first(APP_REF, {label="Environment"})
    if ENV_REF then
        local numRecords = 2 -- RavKav_getRecordInfo(ENV_REF)
        local nRec
        for nRec = 1, numRecords do
            -- only the first record contains valid data
            if RavKav_parseEnvironment(ENV_REF, nRec) then break end
        end
    end
end

function RavKav_parseCounter(CNTRS_REF, nRec)
    local CNTR_REC_REF = nodes.find_first(CNTRS_REF, {label="record", id=nRec})
    if CNTR_REC_REF then
        local record = nodes.get_attribute(CNTR_REC_REF,"val")
        if record then
            local data = bytes.convert(record,1)
            local bitOffset = 0
            local maxCounters = math.floor(#data / 24)
            local nRec, nCnt
            local cntrAry = {}
            for nRec = 1, maxCounters do
                bitOffset, nCnt = RavKav_parseBits(data, bitOffset, 24, CNTR_REC_REF, "Counter", en1545_NUMBER, nRec)
                cntrAry[nRec] = nCnt
            end
            return cntrAry
        end
    end
    return nil
end

function RavKav_parseCounters(APP_REF)
	log.print(log.DBG, "Parsing counters...")
    local CNTRS_REF = nodes.find_first(APP_REF, {id=CALYPSO_LID_COUNTERS})
    if CNTRS_REF then
        local numRecords = RavKav_getRecordInfo(CNTRS_REF)
        if 0 < numRecords then
            -- there should only be one counter record
            return RavKav_parseCounter(CNTRS_REF, 1)
        end
    end
    return nil
end

function RavKav_parseEvent(EVENTS_REF, nRec)
    local EVENT_REC_REF = nodes.find_first(EVENTS_REF, {label="record", id=nRec})
    if nil == EVENT_REC_REF then return end

    local record = nodes.get_attribute(EVENT_REC_REF,"val")
    if nil == record then return end

    local data = bytes.convert(record,1)

	if bytes.is_all(data, 0) then return end

    local bitOffset = 0

    local text, ref, issuerId

    bitOffset                       = RavKav_parseBits(data, bitOffset, 3, EVENT_REC_REF, "Version number", en1545_NUMBER)
    bitOffset, text, ref, issuerId  = RavKav_parseBits(data, bitOffset, 8, EVENT_REC_REF, "Issuer",         RavKav_ISSUER)

    if issuerId == 0 then return end

    local contractId, eventType

    bitOffset, contractId   = RavKav_parseBits(data, bitOffset,  4, EVENT_REC_REF, "Contract ID",               en1545_NUMBER)
    bitOffset               = RavKav_parseBits(data, bitOffset,  4, EVENT_REC_REF, "Area ID",                   en1545_NUMBER)      --1 == urban, 2 == intercity
    bitOffset, eventType    = RavKav_parseBits(data, bitOffset,  4, EVENT_REC_REF, "Event type",                en1545_NUMBER)
    bitOffset               = RavKav_parseBits(data, bitOffset, 30, EVENT_REC_REF, "Event time",                en1545_DATE_TIME)
    bitOffset               = RavKav_parseBits(data, bitOffset,  1, EVENT_REC_REF, "Journey interchanges flag", en1545_NUMBER)      --includes switching/continuing beyond
    bitOffset               = RavKav_parseBits(data, bitOffset, 30, EVENT_REC_REF, "First event time",          en1545_DATE_TIME)   --identical to 'Event time'

    local PRIORITIES_REF = nodes.append(EVENT_REC_REF, {classname="item", label="Best contract priorities"})
    local nContract
    for nContract = 1, 8 do
        bitOffset = RavKav_parseBits(data, bitOffset, 4, PRIORITIES_REF, "Contract", RavKav_PERCENT, nContract)
    end

    local locationBitmap
    local ln_ref = nil

	bitOffset, text, ref, locationBitmap = RavKav_parseBits(data, bitOffset, 7, EVENT_REC_REF, "Location bitmap", en1545_UNDEFINED)    --defined per issuer

    if 0 < bit.AND(locationBitmap, 1) then
        if 2 == issuerId then   --Israel Rail
            bitOffset = RavKav_parseBits(data, bitOffset, 16, EVENT_REC_REF, "Location", RavKav_LOCATION)   --station
        else
            bitOffset = RavKav_parseBits(data, bitOffset, 16, EVENT_REC_REF, "Location ID", en1545_NUMBER)  --place
        end
    end

    if 0 < bit.AND(locationBitmap, 2) then
        local lineNumber
        bitOffset, lineNumber, ln_ref = RavKav_parseBits(data, bitOffset, 16, EVENT_REC_REF, "Line number", en1545_NUMBER)  --number of line on route
    end

    if 0 < bit.AND(locationBitmap, 4) then
        bitOffset = RavKav_parseBits(data, bitOffset, 8, EVENT_REC_REF, "Stop en route", en1545_NUMBER)
    end

    if 0 < bit.AND(locationBitmap, 8) then  --12 unknown bits
        bitOffset = RavKav_parseBits(data, bitOffset, 12, EVENT_REC_REF, "Location[3]", en1545_UNDEFINED)
    end

    if 0 < bit.AND(locationBitmap, 0x10) then
        bitOffset = RavKav_parseBits(data, bitOffset, 14, EVENT_REC_REF, "Vehicle", en1545_UNDEFINED)
    end

    if 0 < bit.AND(locationBitmap, 0x20) then   --how many bits??
	    log.print(log.DBG, string.format("Event[%d]: Location[5]: unknown value", nRec))
    end

    if 0 < bit.AND(locationBitmap, 0x40) then   --8 unknown bits
        if 0 < bit.AND(locationBitmap, 0x20) then   -- we are lost due to previous unknown field
	        log.print(log.DBG, string.format("Event[%d]: Location[6]: unknown value", nRec))
        else
            bitOffset = RavKav_parseBits(data, bitOffset, 8, EVENT_REC_REF, "Location[6]", en1545_UNDEFINED)
        end
    end

    if 0 == bit.AND(locationBitmap, 0x20) then
        local eventExtension
        bitOffset, text, ref, eventExtension = RavKav_parseBits(data, bitOffset, 3, EVENT_REC_REF, "Event extension bitmap", en1545_UNDEFINED)

        if 0 < bit.AND(eventExtension, 1) then
            bitOffset = RavKav_parseBits(data, bitOffset, 10, EVENT_REC_REF, "Route system",    RavKav_ROUTE)
            bitOffset = RavKav_parseBits(data, bitOffset,  8, EVENT_REC_REF, "Fare code",       en1545_NUMBER)
            local debitAmount, dr_ref
            bitOffset, debitAmount, dr_ref = RavKav_parseBits(data, bitOffset, 16, EVENT_REC_REF, "Debit amount", en1545_NUMBER)
            if 0 < debitAmount and 6 ~= eventType and 0 < contractId and 9 > contractId then    --not a transit trip
                if 21 > debitAmount then
                    dr_ref:set_attribute("alt",string.format("%u trip(s)", debitAmount))
                else
                    dr_ref:set_attribute("alt",string.format("NIS %0.2f", debitAmount / 100.0))
                end
            end
        end

        if 0 < bit.AND(eventExtension, 2) then
            bitOffset = RavKav_parseBits(data, bitOffset, 32, EVENT_REC_REF, "Event extension[2]", en1545_UNDEFINED)
        end

        if 0 < bit.AND(eventExtension, 4) then
            bitOffset = RavKav_parseBits(data, bitOffset, 32, EVENT_REC_REF, "Event extension[3]", en1545_UNDEFINED)
        end
    end

    -- fix 'Line number' field
    local lnBitsize = 0
    if 3 == issuerId then       --Egged
        bitOffset = 155
        lnBitsize = 10
        if 0 < bit.AND(locationBitmap, 8) then bitOffset = bitOffset + 12 end
    elseif 15 == issuerId then  --Metropolis
        bitOffset = 123
        lnBitsize = 32
    elseif 2 ~= issuerId and 14 ~= issuerId and 16 ~= issuerId then --not Israel Rail, Nativ Express nor Superbus
        bitOffset = 145
        lnBitsize = 10
    end
    if 0 < lnBitsize then
        if ln_ref then ln_ref:remove() end
        RavKav_parseBits(data, bitOffset, lnBitsize, EVENT_REC_REF, "Line number", en1545_NUMBER)
    end
end

function RavKav_parseEventsLog(APP_REF)
	log.print(log.DBG, "Parsing events...")
    local EVENTS_REF = nodes.find_first(APP_REF, {label="Event logs"} )
    if EVENTS_REF then
        local numRecords = RavKav_getRecordInfo(EVENTS_REF)
        local nRec
        for nRec = 1, numRecords do
            RavKav_parseEvent(EVENTS_REF, nRec)
        end
    end
end

function RavKav_parseContract(CONTRACTS_REF, nRec, counter)
    local CONTRACT_REC_REF = nodes.find_first(CONTRACTS_REF, {label="record", id=nRec})
    if nil == CONTRACT_REC_REF then return end

    local record = nodes.get_attribute(CONTRACT_REC_REF,"val")
    if nil == record then return end

    local data = bytes.convert(record,1)
	if bytes.is_all(data, 0) then 
        nodes.set_attribute(CONTRACT_REC_REF,"alt","empty")
        return 
    end

    local bitOffset = 0

    local text, ref, vfd_ref, issuerId

    bitOffset                   = RavKav_parseBits(data, bitOffset,  3, CONTRACT_REC_REF, "Version number", en1545_NUMBER)
    bitOffset, text, vfd_ref    = RavKav_parseBits(data, bitOffset, 14, CONTRACT_REC_REF, "Valid from",     RavKav_INVERTED_DATE)   --validity start date

    local validFromSeconds = RavKav_rkInvertedDaysToSeconds(nodes.get_attribute(vfd_ref,"val"))

    bitOffset, text, ref, issuerId = RavKav_parseBits(data, bitOffset, 8, CONTRACT_REC_REF, "Issuer", RavKav_ISSUER)
    if issuerId == 0 then return end

    local ticketType, validityBitmap
    local contractValid = true

    bitOffset               = RavKav_parseBits(data, bitOffset, 2, CONTRACT_REC_REF, "Tariff transport access", en1545_NUMBER)
    bitOffset               = RavKav_parseBits(data, bitOffset, 3, CONTRACT_REC_REF, "Tariff counter use",      en1545_NUMBER)  --0 == not used, 2 == number of tokens, 3 == monetary amount
    bitOffset, ticketType   = RavKav_parseBits(data, bitOffset, 6, CONTRACT_REC_REF, "Ticket type",             en1545_NUMBER)

    if 1 == ticketType or 6 == ticketType or 7 == ticketType then   --Single or multiple, Aggregate value or Single or multiple
        contractValid = 0 < counter
        local balance        
        if 6 == ticketType then --Aggregate value
    	    balance = string.format("NIS %0.2f", counter / 100.0)   --balanceRemaining
        else
            balance = string.format("%d trip(s)", counter)          --tripsRemaining
        end
	    local NEW_REF = nodes.append(CONTRACT_REC_REF, {classname="item", label="Balance", size=24})
        nodes.set_attribute(NEW_REF,"val", bytes.new(8, string.format("%06X", counter)))
        nodes.set_attribute(NEW_REF,"alt", balance)
    end

    bitOffset                               = RavKav_parseBits(data, bitOffset, 14, CONTRACT_REC_REF, "Date of purchase",           en1545_DATE)    --sale date
    bitOffset                               = RavKav_parseBits(data, bitOffset, 12, CONTRACT_REC_REF, "Sale device",                en1545_NUMBER)  --serial number of device that made the sale
    bitOffset                               = RavKav_parseBits(data, bitOffset, 10, CONTRACT_REC_REF, "Sale number",                en1545_NUMBER)  --runnning sale number on the day of the sale
    bitOffset                               = RavKav_parseBits(data, bitOffset,  1, CONTRACT_REC_REF, "Journey interchanges flag",  en1545_NUMBER)  --includes switching/continuing beyond
	bitOffset, text, ref, validityBitmap    = RavKav_parseBits(data, bitOffset,  9, CONTRACT_REC_REF, "Validity bitmap",            en1545_UNDEFINED)

    if 0 < bit.AND(validityBitmap, 1) then
        bitOffset = RavKav_parseBits(data, bitOffset, 5, CONTRACT_REC_REF, "Validity[1]", en1545_UNDEFINED)
    end

    local restrictionCode = 0
    if 0 < bit.AND(validityBitmap, 2) then
        bitOffset, restrictionCode = RavKav_parseBits(data, bitOffset, 5, CONTRACT_REC_REF, "Restriction code", en1545_NUMBER)
    end

    if 0 < bit.AND(validityBitmap, 4) then
        local iDuration, rd_ref
        bitOffset, iDuration, rd_ref = RavKav_parseBits(data, bitOffset, 6, CONTRACT_REC_REF, "Restriction duration", en1545_NUMBER)
        if 16 == restrictionCode then
            iDuration = iDuration * 5
        else
            iDuration = iDuration * 30
        end
        rd_ref:set_attribute("alt",string.format("%d minute(s)", iDuration))
    end

    if 0 < bit.AND(validityBitmap,8) then   --validity end date
        local vtd_ref
        bitOffset, text, vtd_ref = RavKav_parseBits(data, bitOffset, 14, CONTRACT_REC_REF, "Valid until", en1545_DATE)
        if contractValid then
            contractValid = RavKav_rkDaysToSeconds(vtd_ref:val()) > os.time()
        end
    end

    local validUntilOption = ValidUntilOption.None
    if 0 < bit.AND(validityBitmap, 0x10) then
        local validMonths
        bitOffset, validMonths = RavKav_parseBits(data, bitOffset, 8, CONTRACT_REC_REF, "Valid months", en1545_NUMBER)
        if 0 < validMonths then
            if 1 == validMonths then
                validUntilOption = ValidUntilOption.Date
                local validUntilSeconds = RavKav_calculateMonthEnd(validFromSeconds, validMonths - 1)   --month end date

        	    local NEW_REF = nodes.append(CONTRACT_REC_REF, {classname="item", label="Valid to", size=14}) --month end date
                nodes.set_attribute(NEW_REF,"val", bytes.new(8, string.format("%04X", validUntilSeconds)))
                nodes.set_attribute(NEW_REF,"alt", os.date("%x", validUntilSeconds))

                if contractValid then
                    contractValid = validUntilSeconds > os.time()
                end
            elseif 129 == validMonths then
                validUntilOption = ValidUntilOption.EndOfService
	            log.print(log.DBG, string.format("Contract[%d]: Valid until the end of the service", nRec))

                local NEW_REF = nodes.append(CONTRACT_REC_REF, {classname="item", label="Valid to", size=2})
                nodes.set_attribute(NEW_REF,"val", bytes.new(8, string.format("%01X", validUntilOption)))
                nodes.set_attribute(NEW_REF,"alt", "The end of the service")

                if contractValid then
                    contractValid = validFromSeconds > os.time()
                end
            else
                log.print(log.DBG, string.format("Contract[%d]: Valid months unknown value: %d", nRec, validMonths))
            end
        else
            validUntilOption = ValidUntilOption.Cancelled
            log.print(log.DBG, string.format("Contract[%d]: Validity cancelled", nRec))

            local NEW_REF = nodes.append(CONTRACT_REC_REF, {classname="item", label="Valid to", size=2})
            nodes.set_attribute(NEW_REF,"val", bytes.new(8, string.format("%01X", validUntilOption)))
            nodes.set_attribute(NEW_REF,"alt", "Validity cancelled")
        end
    end

    if 0 < bit.AND(validityBitmap, 0x20) then
        bitOffset = RavKav_parseBits(data, bitOffset, 32, CONTRACT_REC_REF, "Validity[5]", en1545_UNDEFINED)
    end

    if 0 < bit.AND(validityBitmap, 0x40) then
        bitOffset = RavKav_parseBits(data, bitOffset, 6, CONTRACT_REC_REF, "Validity[6]", en1545_UNDEFINED)
    end

    if 0 < bit.AND(validityBitmap, 0x80) then
        bitOffset = RavKav_parseBits(data, bitOffset, 32, CONTRACT_REC_REF, "Validity[7]", en1545_UNDEFINED)
    end

    if 0 < bit.AND(validityBitmap, 0x100) then
        bitOffset = RavKav_parseBits(data, bitOffset, 32, CONTRACT_REC_REF, "Validity[8]", en1545_UNDEFINED)
    end

    -- read validity locations
    local VALID_LOCS_REF = nodes.append(CONTRACT_REC_REF, {classname="item", label="Validity locations"})
    local nLoc
    for nLoc = 1, 7 do  --limit to 7 just in case the end marker is missing?
        local LOC_REF = nodes.append(VALID_LOCS_REF, {classname="item", label="Validity", id=nLoc})

        local validity, ref, validityType
        bitOffset, validity, ref, validityType = RavKav_parseBits(data, bitOffset, 4, LOC_REF, "Validity type", RavKav_VALIDITY_TYPE)

        if 15 == validityType then  --end of validity locations
            log.print(log.DBG, string.format("Contract %d: Validity location[%d]: validityType: 15 (end of validity locations)", nRec, nLoc))
            nodes.remove(LOC_REF)
            break
        end

        if 0 == validityType then
            bitOffset = RavKav_parseBits(data, bitOffset, 10, LOC_REF, "Route ID",      RavKav_ROUTE, validityType)
            bitOffset = RavKav_parseBits(data, bitOffset, 12, LOC_REF, "Spatial zones", RavKav_ZONES, validityType)
        elseif 1 == validityType then
            bitOffset = RavKav_parseBits(data, bitOffset, 10, LOC_REF, "Route ID",      RavKav_ROUTE, validityType)
            bitOffset = RavKav_parseBits(data, bitOffset,  8, LOC_REF, "Tariff code",   en1545_NUMBER, validityType)
        elseif 3 == validityType then
            bitOffset = RavKav_parseBits(data, bitOffset, 32, LOC_REF, "Validity",      en1545_UNDEFINED, validityType)
        elseif 7 == validityType then
            bitOffset = RavKav_parseBits(data, bitOffset, 10, LOC_REF, "Route ID",      RavKav_ROUTE, validityType)
            bitOffset = RavKav_parseBits(data, bitOffset,  8, LOC_REF, "Tariff code",   en1545_NUMBER, validityType)
        elseif 8 == validityType then   --unknown validity type except for Israel Rail?
            bitOffset = RavKav_parseBits(data, bitOffset, 10, LOC_REF, "Route ID",      RavKav_ROUTE, validityType)
            bitOffset = RavKav_parseBits(data, bitOffset,  8, LOC_REF, "Tariff code",   en1545_NUMBER, validityType)    --?
            bitOffset = RavKav_parseBits(data, bitOffset, 14, LOC_REF, "Unknown",       en1545_UNDEFINED, validityType)
        elseif 9 == validityType then
            bitOffset = RavKav_parseBits(data, bitOffset,  3, LOC_REF, "Extended ticket type (ETT)", en1545_NUMBER, validityType)
            bitOffset = RavKav_parseBits(data, bitOffset, 11, LOC_REF, "Contract type", RavKav_CONTRACT_TYPE, validityType)
        elseif 11 == validityType then
            bitOffset = RavKav_parseBits(data, bitOffset, 21, LOC_REF, "Validity",      en1545_UNDEFINED, validityType)
        elseif 14 == validityType then
            bitOffset = RavKav_parseBits(data, bitOffset, 10, LOC_REF, "Route ID",      en1545_NUMBER, validityType)
            bitOffset = RavKav_parseBits(data, bitOffset, 12, LOC_REF, "Spatial zones", RavKav_ZONES, validityType)
        else
            log.print(log.DBG, string.format("Contract %d: Validity location[%d]: unrecognised validityType: %d", nRec, nLoc, validityType))
            break   --since we don't know the next bit position
        end
    end

    RavKav_parseBits(data, 224, 8,CONTRACT_REC_REF, "Contract authenticator", en1545_UNDEFINED) --checksum?

    local cv_ref = nodes.append(CONTRACT_REC_REF, {classname="item", label="Contract valid", size=1})
    if contractValid then
        nodes.set_attribute(cv_ref,"val", bytes.new(1, 1))
        nodes.set_attribute(cv_ref,"alt", "True")
    else
        nodes.set_attribute(cv_ref,"val", bytes.new(1, 0))
        nodes.set_attribute(cv_ref,"alt", "False")
    end
end

function RavKav_parseContracts(APP_REF, cntrAry)
	log.print(log.DBG, "Parsing contracts...")
    local CONTRACTS_REF = nodes.find_first(APP_REF, {label="Contracts"})
    if CONTRACTS_REF then
        local numRecords = RavKav_getRecordInfo(CONTRACTS_REF)
        local nRec
        for nRec = 1, numRecords do
            RavKav_parseContract(CONTRACTS_REF, nRec, cntrAry[nRec])
        end
    end
end

function RavKav_SummariseProfile(PROFS_REF, id, GI_REF)
    local SRC_REF, NEW_REF = RavKav_copyField(PROFS_REF, GI_REF, "Profile", id, RAVKAV_PROFILES)
    if NEW_REF then
        if 0 < RavKav_getFieldAsNumber(SRC_REF, "Valid until") then
            RavKav_copyField(SRC_REF, NEW_REF, "Valid until")
        end
    end
end

function RavKav_summariseGeneralInfo(APP_REF, SUM_REF)
	log.print(log.DBG, "Summarising general info...")
    local ENV_REF = APP_REF:find_first({label="Environment"}):find_first({label="record"})
    if ENV_REF then
        local GI_REF = nodes.append(SUM_REF, {classname="file", label="General Information"})

        RavKav_copyField(APP_REF, GI_REF, "Serial number")
        RavKav_copyField(ENV_REF, GI_REF, "Issuer", nil, RAVKAV_ISSUERS)

        local idNumber = RavKav_getFieldAsNumber(ENV_REF, "ID number")
        if 0 == idNumber then
            local REF = nodes.append(GI_REF, {classname="item", label="Identity number", size=0})
            nodes.set_attribute(REF,"alt", "Anonymous Rav-Kav")
            return
        end

        RavKav_copyField(ENV_REF,GI_REF, "ID number")
        RavKav_copyField(ENV_REF,GI_REF, "Date of birth")

        local PROFS_REF = nodes.find_first(ENV_REF, {label="Profiles"})
        if PROFS_REF then
            RavKav_SummariseProfile(PROFS_REF, 1, GI_REF)
            RavKav_SummariseProfile(PROFS_REF, 2, GI_REF)
        end
    end
end

function RavKav_summariseEvent(EVENTS_REF, nRec, LT_REF)
    local EVENT_REC_REF = nodes.find_first(EVENTS_REF, {label="record", id=nRec})
    if nil == EVENT_REC_REF then return end

    local issuerId = RavKav_getFieldAsNumber(EVENT_REC_REF, "Issuer")
    if 0 == issuerId then return end

    local EVSUM_REF = nodes.append(LT_REF, {classname="record", label="Event", id=nRec})


    -- Description ---------------------------
    local description = ""

    if RAVKAV_ISSUERS[issuerId] then description = RAVKAV_ISSUERS[issuerId] end

    local lineNumber = RavKav_getFieldAsNumber(EVENT_REC_REF, "Line number")
    if 0 < lineNumber then
        description = description.." line "
        local routeSystemId = RavKav_getFieldAsNumber(EVENT_REC_REF, "Route system")
        local sLineNum = tostring(lineNumber)
        if RAVKAV_ROUTES[routeSystemId] then
            if 15 == issuerId then  --Metropolis
                local sRouteSystemId = tostring(routeSystemId)
                local posS, posE = string.find(sLineNum, sRouteSystemId)
                if 1 == posS then                                       --Does "1234567..." start with "12"?
                    sLineNum = string.sub(sLineNum, posE + 1, posE + 4) --"1234567..." -> "345"
                end
            end
            description = description..sLineNum..", cluster "..RAVKAV_ROUTES[routeSystemId]
        else
            description = description..sLineNum
        end
    end
    if 0 < #description then RavKav_addTextNode(EVSUM_REF, "Description", description) end
    ------------------------------------------


    if 0 < RavKav_getFieldAsNumber(EVENT_REC_REF, "Event time") then
        RavKav_copyField(EVENT_REC_REF, EVSUM_REF, "Event time")
    end

    if 2 == issuerId then   --Israel Rail
        local locationId = RavKav_getFieldAsNumber(EVENT_REC_REF, "Location")
        if RAVKAV_LOCATIONS[locationId] then
            RavKav_addTextNode(EVSUM_REF, "Station", RAVKAV_LOCATIONS[locationId])
        end
    end


    -- Details -------------------------------
    details = ""
    local eventType = RavKav_getFieldAsNumber(EVENT_REC_REF, "Event type")
    local contractId = RavKav_getFieldAsNumber(EVENT_REC_REF, "Contract ID")
    if 6 ~= eventType and 0 < contractId and 9 > contractId then    --not a transit trip
        local REF, data, debitedText = RavKav_getField(EVENT_REC_REF, "Debit amount")
        local debitAmount = 0
        if data then debitAmount = bytes.tonumber(data) end
        if 0 < debitAmount then
            if 1 ~= eventType then  --not an exit event
                details = RAVKAV_EVENT_TYPES[eventType]
            end
            details = details.." contract "..tostring(contractId).." charged "..debitedText
        else
            details = RAVKAV_EVENT_TYPES[eventType].." contract "..tostring(contractId)
        end

        local fareCode = RavKav_getFieldAsNumber(EVENT_REC_REF, "Fare code")
        if 0 < fareCode then
            details = details.." (code "..tostring(fareCode)..")"
        end
    else
        details = RAVKAV_EVENT_TYPES[eventType]
    end
    if 0 < #details then RavKav_addTextNode(EVSUM_REF, "Details", details) end
    ------------------------------------------
end

function RavKav_summariseLatestTravel(APP_REF, SUM_REF)
	log.print(log.DBG, "Summarising latest travel...")
    local EVENTS_REF = nodes.find_first(APP_REF, {label="Event logs"})
    if EVENTS_REF then
        local LT_REF = nodes.append(SUM_REF, {classname="file", label="Latest Travel"})
        local numRecords = RavKav_getRecordInfo(EVENTS_REF)
        local nRec
        for nRec = 1, numRecords do
            RavKav_summariseEvent(EVENTS_REF, nRec, LT_REF)
        end
    end
end

function RavKav_summariseTextArray(REC_REF, itmLabel, lookupAry, SM_REF, sumLabel)
    local summary = ""
    local ary = RavKav_getFieldsAsNumberAry(REC_REF, itmLabel)
    local i, id
    for i, id in ipairs(ary) do
        if lookupAry[id] then
            if 0 < #summary then summary = summary.."\n  " end
            summary = summary..lookupAry[id]
        end
    end
    if 0 < #summary then RavKav_addTextNode(SM_REF, sumLabel, summary) end
end

function RavKav_summariseIntegerArray(REC_REF, itmLabel, SM_REF, sumLabel)
    local summary = ""
    local ary = RavKav_getFieldsAsNumberAry(REC_REF, itmLabel)
    local i, val
    for i, val in ipairs(ary) do
        if 0 < val then
            if 0 < #summary then summary = summary..", " end
            summary = summary..tostring(val)
        end
    end
    if 0 < #summary then RavKav_addTextNode(SM_REF, sumLabel, summary) end
end

function RavKav_summariseContract(CONTRACTS_REF, nRec, VC_REF, IC_REF)
    local CONTRACT_REC_REF = nodes.find_first(CONTRACTS_REF, {label="record", id=nRec})
    if nil == CONTRACT_REC_REF then return end

    local issuerId = RavKav_getFieldAsNumber(CONTRACT_REC_REF, "Issuer")
    if 0 == issuerId then return end

    local contractValid = 0 ~= RavKav_getFieldAsNumber(CONTRACT_REC_REF, "Contract valid")
    local CT_REF = IC_REF
    if contractValid then CT_REF = VC_REF end

    local CTSUM_REF = nodes.append(CT_REF, {classname="record", label="Contract", id=nRec})

    -- Description ---------------------------
    local description = ""

    if RAVKAV_ISSUERS[issuerId] then description = RAVKAV_ISSUERS[issuerId] end

    local ticketType = RavKav_getFieldAsNumber(CONTRACT_REC_REF, "Ticket type")
    if RAVKAV_TICKET_TYPES[ticketType] then description = description.." "..RAVKAV_TICKET_TYPES[ticketType] end

    if 0 < #description then RavKav_addTextNode(CTSUM_REF, "Description", description) end
    ------------------------------------------

    RavKav_summariseTextArray(CONTRACT_REC_REF, "Route ID", RAVKAV_ROUTES, CTSUM_REF, "Clusters")

    -- Type of contract ----------------------
    if 0 < ticketType then
        local REF, data = RavKav_getField(CONTRACT_REC_REF, "Extended ticket type (ETT)")
        if data then
            local ett = bytes.tonumber(data)
            if RAVKAV_CONTRACT_DESCRIPTIONS[ticketType][ett] then
                RavKav_addTextNode(CTSUM_REF, "Type of contract", RAVKAV_CONTRACT_DESCRIPTIONS[ticketType][ett])
            end
        end
    end
    ------------------------------------------

    RavKav_summariseIntegerArray(CONTRACT_REC_REF, "Tariff code", CTSUM_REF, "Tariff codes")
    RavKav_summariseTextArray(CONTRACT_REC_REF, "Contract type", RAVKAV_CONTRACT_TYPES, CTSUM_REF, "Contract types")

    local journeyInterchangesFlag = 0 ~= RavKav_getFieldAsNumber(CONTRACT_REC_REF, "Journey interchanges flag")
    if journeyInterchangesFlag then RavKav_addTextNode(CTSUM_REF, "Journey interchanges", "Includes switching/resume") end

    if 0 < RavKav_getFieldAsNumber(CONTRACT_REC_REF, "Restriction duration") then
        RavKav_copyField(CONTRACT_REC_REF, CTSUM_REF, "Restriction duration")
    end

    RavKav_copyField(CONTRACT_REC_REF, CTSUM_REF, "Balance")

    if 0x3fff ~= RavKav_getFieldAsNumber(CONTRACT_REC_REF, "Valid from") then  --14 bits of valid from date are inverted
        RavKav_copyField(CONTRACT_REC_REF, CTSUM_REF, "Valid from")
    elseif 0 < RavKav_getFieldAsNumber(CONTRACT_REC_REF, "Date of purchase") then
        RavKav_copyField(CONTRACT_REC_REF, CTSUM_REF, "Date of purchase")
    end

    RavKav_copyField(CONTRACT_REC_REF, CTSUM_REF, "Valid to")

    if 0 < RavKav_getFieldAsNumber(CONTRACT_REC_REF, "Valid until") then
        RavKav_copyField(CONTRACT_REC_REF, CTSUM_REF, "Valid until")
    end
end

function RavKav_summariseContracts(APP_REF, SUM_REF)
	log.print(log.DBG, "Summarising contracts...")
    local CONTRACTS_REF = nodes.find_first(APP_REF, {label="Contracts"})
    if CONTRACTS_REF then
        local VC_REF = nodes.append(SUM_REF, {classname="file", label="Valid Contracts"})
        local IC_REF = nodes.append(SUM_REF, {classname="file", label="Invalid Contracts"})
        local numRecords = RavKav_getRecordInfo(CONTRACTS_REF)
        local nRec
        for nRec = 1, numRecords do
            RavKav_summariseContract(CONTRACTS_REF, nRec, VC_REF, IC_REF)
        end
    end
end

function RavKav_readFiles(APP_REF)
	local lid_index
	local lid_desc
	for lid_index, lid_desc in ipairs(LID_LIST) do
		RavKav_readFile(APP_REF, lid_desc[1], lid_desc[2], lid_desc[3])
	end
end

function RavKav_parseRecords(APP_REF)
    RavKav_parseGeneralInfo(APP_REF)
    local cntrAry = RavKav_parseCounters(APP_REF)
    RavKav_parseEventsLog(APP_REF)
    RavKav_parseContracts(APP_REF, cntrAry)
end

function RavKav_summariseRecords(APP_REF, root)
    local SUM_REF = nodes.append(root, {classname="block", label="Summary"})
    RavKav_summariseGeneralInfo(APP_REF, SUM_REF)
    RavKav_summariseLatestTravel(APP_REF, SUM_REF)
    RavKav_summariseContracts(APP_REF, SUM_REF)
end

function RavKav_parseAnswerToSelect(ctx)
    local ats
    local data

    for ats in ctx:find({label="answer to select"}) do
        data = nodes.get_attribute(ats,"val")
        tlv_parse(ats,data,RAVKAV_IDO)  
    end
end


RavKav_parseAnswerToSelect(CARD)
RavKav_parseRecords(CARD)
RavKav_summariseRecords(CARD, CARD)

