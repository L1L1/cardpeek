NETWORK_CODES = {
    -- ------------------------------------------------------------------------
    -- Belgium
    -- ------------------------------------------------------------------------
    [56] = {
        [1] = "Brussels",
    },
    -- ------------------------------------------------------------------------
    -- France
    -- ------------------------------------------------------------------------
    [250] = {
        [000] = "Nord-Pas-de-Calais",
        [502] = "Rhône-Alpes",
        [901] = "Île-de-France",
        [920] = "Provence-Alpes-Côte d'Azur",
        [921] = "Aquitaine",
    },
}

function network_code_name(cc, network_code)
    local country_network_codes = NETWORK_CODES[cc]
    if country_network_codes then
        local network_name = country_network_codes[network_code]
        if network_name then
            return network_name
        end
    end
    return tostring(network_code)
end
