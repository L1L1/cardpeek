-- This is the cardpeek configuration file
-- It does not serve much of a purpose right now, but should
-- grow into a config file in the future, to enable or disable
-- some features in the software.

-- This forces cardpeek to preload some parts of the 'card' library
-- that are written in LUA. Most card analysis scripts need this.
dofile("scripts/lib/apdu.lua")


-- EXPERIMENTAL ACG CONTACTLESS USB READER SUPPORT:
-- To enable experimental ACG contacless Multi-Iso and/or
-- LFX support, un-comment the lines below and set the proper
-- device name for your ACG serial USB device.
-- Please note that the serial device must have correct 
-- permissions set to be readable by cardpeek (by default 
-- most devices are only root-readable).
--
--ACG_MULTI_ISO_TTY="/dev/ttyUSB0"
--ACG_LFX_TTY="/dev/ttyUSB1"


