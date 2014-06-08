--
-- Parser for the OpenPGP card 2.0
-- 
-- Overview & Features : http://g10code.com/p-card.html
-- Specification       : http://g10code.com/docs/openpgp-card-2.0.pdf
-- 
--
-- @name OpenPGP (work)
-- @description OpenPGP cryptographic card
-- @targets 0.8
--
-- Terminology used inside this script:
--
--  * renderer: A method which parses a given piece of data and 
--    	generates a complete subtree for that data. There are three
--		types of renderers: 
--			- "simple value renderer" which just generate
--			  one leaf node in the tree (e.g. for an int-value)
--			- compound renderer which generate one parent and several
--			  child nodes (e.g. a part of an ASN.1 tree)
--			- compound renderer which do NOT generate a parent node
--			  but append their children to the current tree node. 
--			  These have the suffix "_elements"
--
--  * converter: A method which converts a piece of data into 
--      a human readable representation or nil. These are used by
--		the "simple value renderer".
--
-- TODO: 
--		- support bitmap/bitfields using a declarative syntax instead
--			of a programming syntax. usecases: 
--				- historical bytes/{card service data,card capabilities)
--
--		- extend handling of FCI beyond OpenPGP usage
--		- extend handling of Historical Bytes beyond OpenPGP usage
--		
--		- optional ask for PW1 and/or PW3 in order to extract private DOs 3+4
-- 
-- 2014 Andreas Heiduk
-- ------------------------------------------------------------------

require('lib.apdu')
require('lib.strict')

-- ------------------------------------------------------------------

function int2bytes(value)
	return bytes.new(8, bit.SHR(value, 8), bit.AND(value, 0xFF))
end

-- ------------------------------------------------------------------

function append_data_node(parent, classname, label, data, alt)
	local node_attrs = {
		classname = classname,
		label = label,
	}

	if data ~= nil then
		node_attrs.val = data
		node_attrs.size = #data
		node_attrs.alt = alt
	end
	return parent:append(node_attrs)
end

function append_inner_node(parent, classname, label, data, alt, suppress_data)
	if suppress_data then
		local node_attrs = {
			classname = classname,
			label = label,
			size = #data
		}
		
		return parent:append(node_attrs)
	else
		return append_data_node(parent, classname, label, data, alt)
	end
end


function set_command_failed(parent, label, status, data)
	parent:set_attribute("val", "unexpected response")
	append_data_node(parent, nil, "status", int2bytes(status))
	append_data_node(parent, nil, "response", data)
end

-- ------------------------------------------------------------------
-- simple value renderer 
-- ------------------------------------------------------------------

function value(label, converter)
	return function(parent, data)
		if data ~= nil then 
			local alt = converter(data)
			return append_data_node(parent, "item", label, data, alt)
		else
			return append_data_node(parent, "item", label)
		end
	end
end

function value_hex(label)
	return value(label, CONV_HEX)
end

function value_int(label)
	return value(label, CONV_INT)
end

function value_string(label)
	return value(label, CONV_AUTO)
end

function value_timestamp_unix(label)
	return value(label, CONV_TIMESTAMP)
end

function value_mapped(label, mapping)
	return value(label, function(data)
		local key = bytes.tonumber(data)
		local value = mapping[key]
		if value ~= nil then
			return value
		end
	end)
end

-- ------------------------------------------------------------------
-- compound value renderer
-- ------------------------------------------------------------------

function struct_generic(label, element_renderer, suppress_data)
	return function(parent, data)
		local node = append_inner_node(parent, "block", label, data, nil, suppress_data)
		element_renderer(node, data)
		return node		
	end
end

-- ------------------------------------------------------------------

function struct_fixed(label, elements, suppress_data)
	local element_renderer = struct_fixed_elements(elements)
	return struct_generic(label, element_renderer, suppress_data)
end

function struct_fixed_elements(elements)
	return function(parent, data)
		local index = 0
		for i, d in pairs(elements) do
			local size = d[1]
			local renderer = d[2]
			local snippet = data:sub(index, index + size - 1)
			index = index + size
			renderer(parent, snippet)
		end
		
		local additional = data:sub(index)
		if additional ~= nil and #additional > 0 then
			append_data_node(parent, "item", "additional data", additional)
		end
	end
end

-- ------------------------------------------------------------------

function struct_tagged_elements(splitter, elements)
	return function(parent, data)
		while data do
			local tag, value
			tag, value, data = splitter(data)
			
			local renderer = elements[tag]
			if renderer ~= nil then
				renderer(parent, value)
			else
				local hexTag = string.format("0x%04X", tag)
				local node = append_data_node(parent, "item", "tag", value)
				node:set_attribute("id", hexTag)
			end
		end
	end
end
	
-- ------------------------------------------------------------------

function struct_asn1(label, elements, suppress_data)
	local element_renderer = struct_asn1_elements(elements)
	return struct_generic(label, element_renderer, suppress_data)
end

function struct_asn1_elements(elements)
	return struct_tagged_elements(asn1.split, elements)
end

-- ------------------------------------------------------------------

-- Compact Tag Length Value (0xAB means: tag=0xA0, length =0x0B)
function ctlv_split(data)
	if data then
		local tl = data:get(0)
		local tag = bit.AND(tl, 0xF0)
		local length = bit.AND(tl, 0x0F)
		local value = data:sub(1, length)
		local data = data:sub(1+length)
		return tag, value, data
	else
		return nil
	end
end

-- ------------------------------------------------------------------

-- Renderer for "Compact Tag Length Value"
function struct_ctlv(label, elements, suppress_data)
	local element_renderer = struct_ctlv_elements(elements)
	return struct_generic(label, element_renderer, suppress_data)
end

-- Renderer for "Comact Tag Length Value" without parent node
function struct_ctlv_elements(elements)
	return struct_tagged_elements(ctlv_split, elements)
end

-- ------------------------------------------------------------------
-- Bitmaps & Bitfields
-- ------------------------------------------------------------------

function value_bits(label, elements)
	local parent_renderer = value_hex(label)
	return function(parent, data)
		local node = parent_renderer(parent, data)
		for i, bit_handler in ipairs(elements) do
			bit_handler(node, data)
		end
	end
end

function bit_set(mask, label)
	return function(parent, data)
		local b = bytes.get(data, 0)
		local bits = bit.AND(b, mask)
		if bits == mask then
			-- a standard renderer would add "size" and "alt"
			parent:append({
				classname = "item",
				label = label,
				val = bytes.new(8, bits)
			})
		end
	end
end

function bit_clear(mask, label)
	return function(parent, data)
		local b = bytes.get(data, 0)
		local bits = bit.AND(b, mask)
		if bits == 0 then
			return parent:append({
				classname = 'item',
				label = label,
				-- TODO: how to represent unset bits? Perhaps by inverting the mask?
				val = bytes.new(8, bit.XOR(0xFF, mask))
			})
		end
	end
end

function bit_hex(mask, label)
	return function(parent, data)
		local b = bytes.get(data, 0)
		local bits = bit.AND(b, mask)
		if bits ~= 0 then
			-- a standard renderer would add "size" and "alt"
			parent:append({
				classname = "item",
				label = label,
				val = bytes.new(8, bits)
			})
		end
	end
end

-- ------------------------------------------------------------------

-- Creates a special renderer which does NOT create a tree-node but 
-- delegates that completely to the children. children are selected 
-- by the first byte.
-- NOTE: Children get the COMPLETE data including the tag!
function switch_u8(elements)
	return function(parent, data)
		if data then
			local tag = data:get(0)
			local handler = elements[tag]
			if handler ~= nil then
				return handler(parent, data)
			else
				local hexTag = string.format("0x%02X", tag)
				return append_data_node(parent, "item", "unknown tag "..hexTag, data)
			end
		end
	end
end

-- ------------------------------------------------------------------

function struct_switch_u8(label, elements, suppress_data)
	local switch = switch_u8(elements)
	return function(parent, data)
		local node = append_inner_node(parent, "block", label, data, nil, suppress_data)
		switch(node, data)
	end
end

-- ------------------------------------------------------------------
-- Data Objects
-- ------------------------------------------------------------------

function data_object(tag, renderer)
	return function(parent)
		local sw, data = card.get_data(tag)
		if sw == 0x9000 then
			renderer(parent, data)
		else
			local node = renderer(parent, nil)
			set_command_failed(node, sw, data)
		end
	end
end

-- ------------------------------------------------------------------
-- Application 
-- ------------------------------------------------------------------

-- TODO: Support "setup" hook after "SELECT FILE"
-- TODO: Support Folder/File Structure (really below Application?)
-- TODO: Think about a plain, flat "do command" structure instead of SELECTs, DOs, files,...

function application(properties)
	return function(parent)
		local aid = properties.aid
		if aid ~= nil then 
			local label = properties.label or ""
			local app_node = parent:append({
				classname = "application", 
				label = label
			})
			
			-- SELECT FILE
			local sw, data = card.select(aid)
			if sw == 0x9000 then
				-- append "answer to select"
				if properties.fc ~= nil then
					properties.fc(app_node, data)
				end
				
				-- (optional) VERIFY
				if properties.prepare then
					properties.prepare(app_node)
				end

				-- GET DATA
				local dos = properties.dos
				if dos ~= nil then
					for i, handler in ipairs(DOs) do
						handler(app_node)
					end
				end
			else
				set_command_failed(app_node, "SELECT FILE failed", sw, data)
			end
		end
	end
end		

-- ------------------------------------------------------------------
-- Converter
-- ------------------------------------------------------------------

CONV_AUTO = function(data)
	if bytes.is_printable(data) then
		return bytes.format(data, '%P')
	else
		return nil
	end
end

-- ------------------------------------------------------------------

CONV_HEX = function(data)
	return nil
end

-- ------------------------------------------------------------------

CONV_INT = function(data) 
	return bytes.tonumber(data)
end

-- ------------------------------------------------------------------

CONV_TIMESTAMP = function(data)
	local seconds = bytes.tonumber(data)
	return os.date("%c", seconds)
end

-- ------------------------------------------------------------------
-- Historical Bytes
-- ------------------------------------------------------------------

HISTORICAL_BYTES_TYPE_00_ELEMENTS = struct_ctlv_elements({
	[0x10] = value_hex("country code and national date"),
	[0x20] = value_hex("issuer identification number"),
	[0x30] = value_bits("card service data", {
		bit_set(0x80, "application selection by full DF name"),
		bit_set(0x40, "application selection by partial DF name"),
		bit_set(0x20, "Data Objects available in DIR file"),
		bit_set(0x10, "Data Objects available in ATR file"),
		bit_set(0x08, "file IO services by READ BINARY command"),
		bit_clear(0x08, "file IO services by READ RECORD command"),
		bit_hex(0x07, "RFU"),
	}),
	[0x40] = value_hex("initial access data"),
	[0x50] = value_hex("card issuer data"),
	[0x60] = value_hex("pre-issuing data"),
	[0x70] = value_hex("card capabilities"),
})

CARD_LIFE_STATUS = value_mapped("card life status", {
	[0x00] = "no information",
	[0x03] = "initialisation state",
	[0x05] = "operational state",
})

function historical_type_00(parent, data)
	-- special handling: 
	-- 		first byte: message type (0x00)
	--		followed by CTLV data upto ..
	--		last three bytes: 
	--			card life status (1 byte, e.g. 0x05)
	--			status word (2 byte, e.g. 0x9000)
	
	local card_life_status = data:sub(#data-3, #data-3)
	local status_bytes = data:sub(#data-2, #data-1)
	local ctlv_data = data:sub(1, #data-3-1)
	
	-- append CTLV data
	HISTORICAL_BYTES_TYPE_00_ELEMENTS(parent, ctlv_data)
	-- append non-CTLV data
	CARD_LIFE_STATUS(parent, card_life_status)
	append_data_node(parent, "item", "status bytes", status_bytes)
end

-- ------------------------------------------------------------------
-- Data Structures
-- ------------------------------------------------------------------

APPLICATION_ID = struct_fixed("Application ID", {
	{ 5, value_hex("RID") },
	{ 1, value_hex("Application") },
	{ 2, value_hex("Version") },
	{ 2, value_mapped("Manufacturer", {
		[0x0001] = "PPC Card Systems",
		[0x0002] = "Prism",
		[0x0003] = "OpenFortress",
		[0x0004] = "Wewid AB",
		[0x0005] = "ZeitControl",
		[0x002A] = "Magrathea",
		[0xF517] = "FSIJ",
		[0x0000] = "test card",
		[0xffff] = "test card",
	})},
	{ 4, value_hex("Card serial number") },
	{ 2, value_hex("Reserved/future use") },
})

CARDHOLDER = struct_asn1("Cardholder data", {
	[0x005b] = value_string("Name"),
	[0x5f2d] = value_string("Language preferences"),
	[0x5f35] = value_mapped("Sex", {
		[0x31] = "male",
		[0x32] = "female",
		[0x39] = "not announces"
	}),
})

HISTORICAL_BYTES = struct_switch_u8("Historical Bytes", {
	[0x00] = historical_type_00,
	-- FIXME: handle other types
})

PW_STATUS = struct_fixed("Password Status", {
	{ 1, value_mapped("Signature PIN", {
		[0x00] = "one PW1 per signature",
		[0x01] = "multiple signatures allowed"
	})},
	{ 1, value_int("Max Length PW1 (user)") },
	{ 1, value_int("Max Length Reset Code") },
	{ 1, value_int("Max Length PW3 (admin)") },
	{ 1, value_int("permissible retries PW1") },
	{ 1, value_int("permissible retries RC") },
	{ 1, value_int("permissible retries PW3") },
})

ALGO_ATTR_PARTS = {
	{ 1, value_hex("Algorithm ID")},
	{ 2, value_int("Length of modulus", CONV_INT)},
	{ 2, value_int("Length of public exponent", CONV_INT)},
	{ 1, value_mapped("private key format", {
		[0x00] = "standard (e, p, q)",
		[0x01] = "standard with modulus (n)",
		[0x02] = "CRT",
		[0x03] = "CRT with modulus"
	})}
}

APP_RELATED = struct_asn1("Application related data", {
	[0x004f] = APPLICATION_ID,
	[0x5f52] = HISTORICAL_BYTES,
	[0x0073] = struct_asn1("Discretionary data objects", {
		[0x00c0] = struct_fixed("Extended capabilities", {
			{ 1, value_bits("Supported features", {
				bit_set(0x80, "Secure Messaging"),
				bit_set(0x40, "GET CHALLENGE"),
				bit_set(0x20, "Key Import"),
				bit_set(0x10, "PW1 status changeable"),
				bit_set(0x08, "Private DOs"),
				bit_set(0x04, "Algorithm attributes changeable"),
			})},
			{ 1, value_mapped("Secure Messaging Algorithm", {
				[0x00] = "Triple DES",
				[0x01] = "AES-128"
			})},
			{ 2, value_int("Max length for GET CHALLENGE") },
			{ 2, value_int("Max length Caardholder Certificate") },
			{ 2, value_int("Max length command data") },
			{ 2, value_int("Max length response data") }
		}),
		[0x00c1] = struct_fixed("Algorithm attributes (signature)", ALGO_ATTR_PARTS),
		[0x00c2] = struct_fixed("Algorithm attributes (decryption)", ALGO_ATTR_PARTS),
		[0x00c3] = struct_fixed("Algorithm attributes (authentication)", ALGO_ATTR_PARTS),
		[0x00c4] = PW_STATUS,
		[0x00c5] = struct_fixed("Fingerprints", {
			{ 20, value_hex("Signature key") },
			{ 20, value_hex("Encryption key") },
			{ 20, value_hex("Authentication key") },
		}, true),
		[0x00c6] = struct_fixed("CA Fingerprints", {
			{ 20, value_hex("Fingerprint 1") },
			{ 20, value_hex("Fingerprint 2") },
			{ 20, value_hex("Fingerprint 3") },
		}, true),
		[0x00cd] = struct_fixed("Generation timestamps", {
			{ 4, value_timestamp_unix("Signature key") },
			{ 4, value_timestamp_unix("Encryption key") },
			{ 4, value_timestamp_unix("Authentication key") },
		}),
	}, true)
}, true)

SECURITY_SUP_TEMPLATE = struct_asn1("Security support template", {
	[0x0093] = value_int("Digital signature counter"),
})

-- ------------------------------------------------------------------

FC_ITEMS = {
	[0x80] = value_int("File size (excl. structural information)"),
	[0x81] = value_int("File size (incl. structural information)"),
	[0x82] = value_hex("File descriptor"),
	[0x83] = value_hex("File ID"),
	[0x84] = value_hex("DF name"),	-- OpenPGP
	[0x85] = value_hex("proprietary information"),
	[0x86] = value_hex("Security attributes"),
	[0x87] = value_hex("File ID with FCI extension"),
}

FCI = struct_asn1_elements({
	[0x62] = struct_asn1("File control parameters", FC_ITEMS),
	[0x64] = struct_asn1("File management data", FC_ITEMS),
	[0x6f] = struct_asn1("File control information", FC_ITEMS),
})

-- ------------------------------------------------------------------

DOs = {
	data_object(0x004f, APPLICATION_ID),
	data_object(0x0065, CARDHOLDER),
	data_object(0x5f50, value_string("URL of public key")),
	data_object(0x005e, value_hex("Login data")),
	data_object(0x0101, value_hex("Private DO 1")),
	data_object(0x0102, value_hex("Private DO 2")),
	data_object(0x0103, value_hex("Private DO 3")),
	data_object(0x0104, value_hex("Private DO 4")),
	data_object(0x5f52, HISTORICAL_BYTES),
	data_object(0x006e, APP_RELATED),
	data_object(0x00c4, PW_STATUS),
	data_object(0x007a, SECURITY_SUP_TEMPLATE),
	data_object(0x7f21, value_hex("Cardholder certificate")),
}

-- ------------------------------------------------------------------

function unlock_private_dos(pw1, pw3)
	return function(parent)
		if pw1 ~= nil then
			card.verify(pw1, 0x00, 0x82)
		end
		
		if pw3 ~= nil then
			card.verify(pw3, 0x00, 0x83)
		end
	end
end

OPENPGP_APP = application({
	label = "OpenPGP Application",
	aid = "#D276000124".."01",	-- RID of FSFE (D276000124) + PIX for OpenPGP (01)
	fc = FCI,
	-- Enable to unlock Private DOs 3 and 4 -- but three wrong tries
	-- will lock up the card! After that a factory reset is required.
	-- prepare = unlock_private_dos("123456", "12345678"),
	dos = DOs,
})

-- ------------------------------------------------------------------

function card.verify(pin, p1, p2)
	local command, length

	p1 = p1 or 0x00
	p2 = p2 or 0x00
	pin = bytes.new_from_chars(pin)
	length = #pin
	command = bytes.new(8,card.CLA,0x20,p1,p2,length,pin)
	return card.send(command)
end

-- ------------------------------------------------------------------

if card.connect() then
	local root_node = card.tree_startup("OpenPGP Card")
	OPENPGP_APP(root_node)
	card.disconnect()
end
