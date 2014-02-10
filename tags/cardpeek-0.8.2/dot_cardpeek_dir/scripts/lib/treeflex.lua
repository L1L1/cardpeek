--

local TREEFLEX_WARNING=false

if TREEFLEX_WARNING==false then
    log.print(log.WARNING,"the treeflex library included in this script is obsolete, please use the new 'nodes' API\n" ..
              "\tThis warning is only printed once.")
    TREEFLEX_WARNING=true
end

