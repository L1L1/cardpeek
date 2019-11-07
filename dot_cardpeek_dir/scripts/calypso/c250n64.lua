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

SERVICE_PROVIDERS = {
  [5] = "TAM"
}

TRANSPORT_LIST = {  
  [1] = "Urban Bus",  
  [2] = "Interurban Bus",  
  [3] = "Metro",  
  [4] = "Tram",  
  [5] = "Train",  
  [8] = "Parking"
}

TRANSITION_LIST = {  
  [1] = "Entry",  
  [2] = "Exit",  
  [4] = "Inspection",  
  [6] = "Interchange (entry)",  
  [7] = "Interchange (exit)"
}

function process_events(cardenv,node_label)
    local event_node
    local record_node
    local ref_node

    local code_value
    local code_transport
    local code_transition
    local code_transport_string
    local code_transition_string
    local code_string
    local service_provider_value
    local location_id_value
    local sector_id
    local station_id
    local location_string

    event_node = cardenv:find_first({label=node_label, parsed="true"})

    if event_node==nil then 
       log.print(log.WARNING,"No " .. node_label .. " found in card")
       return 0 
    end

    for record_node in event_node:find({label="record"}) do

        -- is it TAM?
        ref_node = record_node:find_first({label="EventServiceProvider"})
        service_provider_value = bytes.tonumber(ref_node:get_attribute("val"))
        ref_node:set_attribute("alt",SERVICE_PROVIDERS[service_provider_value])

        ref_node = record_node:find_first({label="EventCode"})
        code_value = bytes.tonumber(ref_node:get_attribute("val"))

        -- is it a bus, a tram, ...?
        code_transport = bit.SHR(code_value,4)
        code_transport_string  = TRANSPORT_LIST[code_transport]
        if code_transport_string==nil then code_transport_string = code_transport end

        -- is it an entry, an exit, ...?
        code_transition = bit.AND(code_value,0xF)
        code_transition_string = TRANSITION_LIST[code_transition]   
        if (code_transition_string==nil) then code_transition_string = code_transition end

        ref_node:set_attribute("alt",code_transport_string.." - "..code_transition_string)
    end
end

process_events(CARD,"Event logs")
