--
-- This file is part of Cardpeek, the smartcard reader utility.
--
-- Copyright 2009 by 'L1L1' and 2013-2014 by 'kalon33'
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

---------------------------------------------------------------------
-- Most of the data and coding ideas in this file 
-- was contributed by 'Pascal Terjan', based on location 
-- data from 'Nicolas Derive'.
---------------------------------------------------------------------

require('lib.strict')
require('etc.gironde-transgironde')

SERVICE_PROVIDERS = {
  [21] = "TBC",
  [16] = "Transgironde"
}

TRANSITION_LIST = {  
  [1] = "Entrée",
  [6] = "Correspondance"
}

function navigo_process_events(ctx)
	local EVENTS
	local RECORD
	local REF
	local rec_index
	local code_value
	local route_number_value
	local vehicle_number_value
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
	
	EVENTS = ui.tree_find_node(ctx,"Event logs, parsed");
	if EVENTS==nil then 
	   log.print(log.WARNING,"No event found in card")
	   return 0 
	end

	for rec_index=1,16 do
	    RECORD = ui.tree_find_node(EVENTS,"record",rec_index)
	    if RECORD==nil then break end
	    
	    REF = ui.tree_find_node(RECORD,"EventServiceProvider")
	    service_provider_value = bytes.tonumber(ui.tree_get_value(REF))
if service_provider_value == 21 then
TRANSPORT_LIST = {  
  [0] = "Information de mode de transport absente (ordinateur de bord non configuré)",
  [1] = "Tram",    
  [5] = "Bus" -- apparently bus, maybe tram B in some conditions, needs verification --> seems that a "lost" vehicle (without line information) is a bus by default.
}
else
TRANSPORT_LIST = {  
  [1] = "Car"
}
end	
	    ui.tree_set_alt_value(REF,SERVICE_PROVIDERS[service_provider_value])
	    
	    REF = ui.tree_find_node(RECORD,"EventRouteNumber")
	    route_number_value = bytes.tonumber(ui.tree_get_value(REF))
	    if (route_number_value >= 1 and route_number_value <= 16) then
	    ui.tree_set_alt_value(REF,"Liane "..route_number_value)
	    else
		 if ((route_number_value >= 20 and route_number_value <= 30) or (route_number_value >= 62 and route_number_value <= 67) or (route_number_value >= 70 and route_number_value <=96)) then
	    ui.tree_set_alt_value(REF,"Ligne "..route_number_value)
	    else
		 if ((route_number_value >= 32 and route_number_value <= 37)) then
	    ui.tree_set_alt_value(REF,"Corol "..route_number_value)
	    else
		 if ((route_number_value == 38) or (route_number_value >= 48 and route_number_value <= 49) or (route_number_value == 68)) then
	    ui.tree_set_alt_value(REF,"Flexo "..route_number_value)
	    else
		 if ((route_number_value >= 51 and route_number_value <= 58) and not (route_number_value == 53) and not (route_number_value == 56)) then
	    ui.tree_set_alt_value(REF,"Flexo de nuit "..route_number_value)
	    else
		 if (route_number_value == 53 or route_number_value == 56) then
	    ui.tree_set_alt_value(REF,"Ligne "..route_number_value.." Express")
	    else
		 if (route_number_value == 59) then
	    ui.tree_set_alt_value(REF,"Ligne A")
	    else
		 if (route_number_value == 60) then
	    ui.tree_set_alt_value(REF,"Ligne B")
	    else
		 if (route_number_value == 61) then
	    ui.tree_set_alt_value(REF,"Ligne C")
	    else
		 if service_provider_value == 16 then
	    ui.tree_set_alt_value(REF,"Ligne "..route_number_value)
	    else
	    ui.tree_set_alt_value(REF,"Ligne inconnue (ou non définie dans l'ordinateur de bord au moment du badgeage)")
	    end
	    end
	    end
	    end
	    end
	    end
	    end
	    end
	    end

	    REF = ui.tree_find_node(RECORD,"EventVehiculeId")
	    vehicle_number_value = bytes.tonumber((ui.tree_get_value(REF)))
	    ui.tree_set_alt_value(REF,"Véhicule n°"..vehicle_number_value)
	    
	    REF = ui.tree_find_node(RECORD,"EventCode")
	    code_value = bytes.tonumber(ui.tree_get_value(REF))

	    code_transport = bit.SHR(code_value,4)
	    code_transport_string  = TRANSPORT_LIST[code_transport]
	    if code_transport_string==nil then code_transport_string = code_transport end

	    code_transition = bit.AND(code_value,0xF)
	    if (code_transition == 6) and (route_number_value == 0) then code_transition = 0 end
	    code_transition_string = TRANSITION_LIST[code_transition]
	    if (code_transition_string==nil) then code_transition_string = code_transition end

	    ui.tree_set_alt_value(REF,code_transport_string.." - "..code_transition_string)
	    REF = ui.tree_find_node(RECORD,"EventLocationId")
	       location_id_value = bytes.tonumber(ui.tree_get_value(REF))
	    if location_id_value == 0 then ui.tree_set_alt_value(REF,"Aucun positionnement précis disponible")
	    else
	    if TRANSGIRONDE_STOPS_LIST[location_id_value] then
	    location_string = "arrêt "..TRANSGIRONDE_STOPS_LIST[location_id_value]
	    ui.tree_set_alt_value(REF, location_string)
	    else
--	    ui.tree_set_alt_value(REF, location_id_value)
	    ui.tree_set_alt_value(REF, TRANSGIRONDE_STOPS_LIST[location_id_value])
	    end
	    end

	end
	end
end

navigo_process_events(CARD)


