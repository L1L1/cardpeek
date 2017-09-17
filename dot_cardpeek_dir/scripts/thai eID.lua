-- By David Huang <khym@azeotrope.org>
-- Based on information from https://github.com/chakphanu/ThaiNationalIDCard
--
-- @name Thai eID
-- @description Thai national ID card.
-- @targets 0.8

require("lib.strict")

-- Convert a TIS-620-encoded string to UTF-8 and remove trailing spaces
function format_string(node)
	local data = node:get_attribute("val")
	local s = converter:iconv(data:format("%C"))
	s = string.gsub(s, " +$", "")
	node:set_attribute("alt", s)
	return s
end

-- Split string with "#" as the delimiter, and label the pieces
function format_splithash(node, labels)
	local s = format_string(node)
	local pattern = string.rep("([^#]*)", #labels, "#")
	for i,e in ipairs({string.match(s, pattern)}) do
		if labels[i] then
			node:append({ classname="item", label=labels[i], val=e })
		end
	end
end

function format_name(node)
	local labels = { "Prefix", "First", "Middle", "Last" }
	format_splithash(node, labels)
end

function format_address(node)
	local labels = {
		"House Number", "Village Number (Mu Ban)",
		"Lane (Trok/Soi)", "Road (Thanon)", nil, "Tambon/Khwaeng",
		"Amphur/Khet", "Province"
	}
	format_splithash(node, labels)
end

-- Thai ID number
function format_id(node)
	local s = format_string(node)
	local a, b, c, d, e
	local _, _, a, b, c, d, e = string.find(s, "(.)(....)(.....)(..)(.)")
	node:set_attribute("alt", string.format("%s-%s-%s-%s-%s", a, b, c, d, e))
end

-- Thai ID card serial number
function format_serial(node)
	local s = format_string(node)
	local a, b, c
	local _, _, a, b, c = string.find(s, "(....)(..)(.*)")
	node:set_attribute("alt", string.format("%s-%s-%s", a, b, c))
end

-- Convert Buddhist calendar date to Gregorian
-- Expiration date of 9999-99-99 means no expiration
-- Note that some people may only have a birth year, with no birth month
-- or day. In that case, the month and day will be "00".
function format_date(node)
	local s = format_string(node)
	local m, d, y
	if s == "99999999" then
		s = "Lifelong"
	else
		_, _, y, m, d = string.find(s, "(....)(..)(..)")
		y = tonumber(y)
		m = tonumber(m)
		d = tonumber(d)
		s = string.format("%04d-%02d-%02d", y-543, m, d)
	end
	node:set_attribute("alt", s)
end

function format_sex(node)
	local data = bytes.get(node:get_attribute("val"), 0)
	local s
	if data == 0x31 then
		s = "Male"
	elseif data == 0x32 then
		s = "Female"
	else
		s = string.format("Unknown %c", data)
	end
	node:set_attribute("alt", s)
end

function format_photo(node)
	node:set_attribute("mime-type", "image/jpeg")
end

local eid_elements =
{	-- Label, offset, length, format function
	{ "Citizen ID", 4, 13, format_id },
	{ "Thai Name", 17, 100, format_name },
	{ "Romanized Name", 117, 100, format_name } ,
	{ "Date of Birth", 217, 8, format_date },
	{ "Sex", 225, 1, format_sex },
	{ "Address", 5497, 160, format_address },
	{ "Date of Issue", 359, 8, format_date },
	{ "Date of Expiry", 367, 8, format_date },
	{ "Issuer", 246, 100, format_string },
	{ "Card Number", 5657, 14, format_serial },
--	{ "Unknown", 375, 2, format_string }, -- Is 0x3031 ("01") on my cards
--	{ "Unknown", 377, 2,  },
--	{ "Unknown (Fingerprints?)", 5671, 279, },
	{ "Photo", 379, 5118, format_photo }
}

function thaiid_get_binary(offset, length)
	return card.send(bytes.new(8, 0x80, 0xB0, bit.AND(bit.SHR(offset,8),0x7F), bit.AND(offset,0xFF), 0x02, 0x00, length))
end

-- Read the contents of the card into a bytestring
function thaiid_read_data(card)
	local sw, resp
	local data
	
	-- Thailand Ministry of Interior AID
	sw, resp = card.select("#A000000054480001")
	if sw == 0x9000 then
		local offset, size, chunk
		
		data = bytes.new(8)
		size = 5950
		chunk = 255
		
		for offset = 0, size-1, chunk do
			sw, resp = thaiid_get_binary(offset, math.min(chunk, size))
			if sw ~= 0x9000 then
				break
			end
			data = data .. resp
			size = size - chunk
		end
	end
	
	return data
end

if card.connect() then
	local CARD
	local data
	
	CARD = card.tree_startup("Thai eID")
	data = thaiid_read_data(card)
	card.disconnect()

	-- Strings are encoded in TIS-620; need to convert to UTF-8 for Cardpeek
	converter = iconv.open("TIS-620", "UTF-8")
	
	-- Extract info from the bytestring by offset and length
	for k, v in ipairs(eid_elements) do
		local element
		local node
		
		element = bytes.sub(data, v[2], v[2]+v[3]-1)
		node = CARD:append({ classname="item", label=v[1], val=element, size=v[3] })
		if type(v[4])=="function" then
			v[4](node)
		end
	end
end
