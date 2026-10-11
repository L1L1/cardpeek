--
-- Cardpeek script for the Uruguayan electronic identity card
-- (Cédula de Identidad electrónica, contact chip / IAS Classic applet).
--
-- Based on Agesic's "Documentación técnica de la cédula de identidad con chip"
-- (Guía de uso de Cédula de Identidad con chip a través de APDU):
--
--   SELECT IAS     00 A4 04 00 0C A0 00 00 00 18 40 00 00 01 63 42 00 00
--   GET DATA 7F30  00 CA 7F 30 00      -> applet label/version (IAS v4 / v5)
--   SELECT FILE    00 A4 00 00 02 B001 00  -> FCI, size in tag 81
--   READ BINARY    00 B0 <off_hi> <off_lo> <len>  (max 0xFF per command)
--
--   B001  cardholder certificate (X.509, DER)
--
-- No PIN is needed: everything read here is public data.
--
-- @name Uruguay eID v5
-- @description Uruguayan electronic identity card (Cédula de Identidad)
-- @targets 0.8

require('lib.apdu')
require('lib.strict')

local IAS_AID = "A0 00 00 00 18 40 00 00 01 63 42 00"

--------------------------------------------------------------------------
-- Text helpers
--------------------------------------------------------------------------

local function is_valid_utf8(s)
    local i, n = 1, #s
    while i <= n do
        local c = s:byte(i)
        local len
        if c < 0x80 then len = 1
        elseif c >= 0xC2 and c <= 0xDF then len = 2
        elseif c >= 0xE0 and c <= 0xEF then len = 3
        elseif c >= 0xF0 and c <= 0xF4 then len = 4
        else return false end
        if i + len - 1 > n then return false end
        for j = i + 1, i + len - 1 do
            local b = s:byte(j)
            if b < 0x80 or b > 0xBF then return false end
        end
        i = i + len
    end
    return true
end

local function latin1_to_utf8(s)
    return (s:gsub("[\128-\255]", function(ch)
        local c = ch:byte()
        return string.char(0xC0 + math.floor(c / 64), 0x80 + c % 64)
    end))
end

-- Bytestring -> displayable Lua string (handles ASCII, UTF-8 and Latin-1).
local function to_text(data)
    local s = bytes.format(data, "%C")
    s = s:gsub("[\0\255]+$", ""):gsub("^%s+", ""):gsub("%s+$", "")
    if not is_valid_utf8(s) then
        s = latin1_to_utf8(s)
    end
    return s
end

-- Returns the bytestring as text if it looks like a readable string
-- (UTF-8 first, then Latin-1), or nil if it looks like binary data.
local function readable_text(data)
    if data == nil or #data == 0 then
        return nil
    end
    local s = bytes.format(data, "%C"):gsub("%z+$", "")
    if #s == 0 then
        return nil
    end

    -- No C0 controls (except tab/CR/LF) and no DEL.
    if s:find("[%z\1-\8\11\12\14-\31\127]") then
        return nil
    end

    if is_valid_utf8(s) then
        -- Reject C1 controls (U+0080..U+009F).
        if s:find("\194[\128-\159]") then
            return nil
        end
        return s
    end

    -- Latin-1: no C1 controls, and mostly ASCII so that random
    -- binary values (key identifiers, hashes...) are not shown as text.
    if s:find("[\128-\159]") then
        return nil
    end
    local ascii = select(2, s:gsub("[\32-\126]", ""))
    if ascii * 4 < #s * 3 then
        return nil
    end
    return latin1_to_utf8(s)
end

-- UTF-16BE (ASN.1 BMPString) -> UTF-8, or nil if malformed.
local function bmp_to_utf8(data)
    if #data % 2 ~= 0 then
        return nil
    end
    local out = {}
    for i = 0, #data - 1, 2 do
        local c = data:get(i) * 256 + data:get(i + 1)
        if c >= 0xD800 and c <= 0xDFFF then
            return nil
        elseif c < 0x80 then
            out[#out + 1] = string.char(c)
        elseif c < 0x800 then
            out[#out + 1] = string.char(0xC0 + math.floor(c / 64), 0x80 + c % 64)
        else
            out[#out + 1] = string.char(0xE0 + math.floor(c / 4096),
                                        0x80 + math.floor(c / 64) % 64,
                                        0x80 + c % 64)
        end
    end
    return table.concat(out)
end

--------------------------------------------------------------------------
-- APDU helpers
--------------------------------------------------------------------------

-- Sends an APDU, transparently handling 6Cxx (wrong Le) and
-- 61xx (GET RESPONSE) in case the reader/driver does not.
local function send(apdu)
    local sw, resp = card.send(apdu)

    if bit.AND(sw, 0xFF00) == 0x6C00 then
        apdu = bytes.concat(bytes.sub(apdu, 0, #apdu - 2), bit.AND(sw, 0xFF))
        sw, resp = card.send(apdu)
    end

    local data = resp or bytes.new(8)
    while bit.AND(sw, 0xFF00) == 0x6100 do
        sw, resp = card.send(bytes.new(8, 0x00, 0xC0, 0x00, 0x00, bit.AND(sw, 0xFF)))
        if resp then
            data = bytes.concat(data, resp)
        end
    end

    return sw, data
end

local function select_ias()
    return send(bytes.new(8, "00 A4 04 00 0C", IAS_AID, "00"))
end

local function select_file(fid)
    return send(bytes.new(8, string.format("00 A4 00 00 02 %s 00", fid)))
end

-- Extracts the file size from an FCI/FCP template (tag 81, or 80).
local function fci_file_size(fci)
    if fci == nil or #fci == 0 then
        return nil
    end
    local tag, value = asn1.split(fci)
    if tag ~= 0x6F and tag ~= 0x62 then
        return nil
    end
    while value do
        local t, v, rest = asn1.split(value)
        if t == nil then
            break
        end
        if (t == 0x81 or t == 0x80) and #v > 0 then
            return bytes.tonumber(v)
        end
        value = rest
    end
    return nil
end

-- Reads the currently selected transparent file, 0xFF bytes at a time.
-- If size is nil, reads until the card reports end of file.
local function read_selected(size)
    local data = bytes.new(8)
    local offset = 0
    local sw = 0x9000

    while size == nil or offset < size do
        local n = 0xFF
        if size ~= nil then
            n = math.min(0xFF, size - offset)
        end

        local resp
        sw, resp = send(bytes.new(8, 0x00, 0xB0,
                                  bit.AND(bit.SHR(offset, 8), 0x7F),
                                  bit.AND(offset, 0xFF), n))

        if resp and #resp > 0 then
            data = bytes.concat(data, resp)
            offset = offset + #resp
        end

        if sw ~= 0x9000 or resp == nil or #resp == 0 then
            break
        end
        if size == nil and #resp < n then
            break
        end
    end

    -- Hitting end of file is expected when the size is unknown.
    if size == nil and #data > 0 and
       (sw == 0x6B00 or sw == 0x6282 or bit.AND(sw, 0xFF00) == 0x6C00) then
        sw = 0x9000
    end

    return sw, data
end

-- ASCII view of arbitrary bytes: printable ASCII kept, everything else '.'.
local function ascii_dump(data)
    return (bytes.format(data, "%C"):gsub("[^\32-\126]", "."))
end

-- "AB:CD:EF..." hex view.
local function hex_colon(data)
    return (bytes.format(data, "%D"):gsub("(%x%x)", "%1:"):gsub(":$", ""))
end

-- Best readable form of arbitrary bytes: text if it is text, ASCII
-- (non-printable bytes as '.') if most bytes are printable, hex otherwise.
local function bytes_text(data)
    if data == nil or #data == 0 then
        return ""
    end
    local text = readable_text(data)
    if text then
        return text
    end
    local printable = 0
    for i = 0, #data - 1 do
        local b = data:get(i)
        if b >= 0x20 and b <= 0x7E then
            printable = printable + 1
        end
    end
    if printable * 10 >= #data * 7 then
        return ascii_dump(data)
    end
    return hex_colon(data)
end

-- Unsigned big-endian bytestring -> decimal string (any length).
local function bytes_to_decimal(data)
    local n = {}
    for i = 0, #data - 1 do
        n[#n + 1] = data:get(i)
    end
    local limbs = {}
    while #n > 0 do
        local q, rem = {}, 0
        for _, b in ipairs(n) do
            local cur = rem * 256 + b
            local d = math.floor(cur / 10000000)
            rem = cur % 10000000
            if #q > 0 or d > 0 then
                q[#q + 1] = d
            end
        end
        table.insert(limbs, 1, rem)
        n = q
    end
    if #limbs == 0 then
        return "0"
    end
    local out = { string.format("%d", limbs[1]) }
    for i = 2, #limbs do
        out[#out + 1] = string.format("%07d", limbs[i])
    end
    return table.concat(out)
end

-- { Spanish label, technical name }
local OID_NAMES = {
    ["2.5.4.3"]  = { "Nombre y apellido", "commonName" },
    ["2.5.4.4"]  = { "Apellido", "surname" },
    ["2.5.4.5"]  = { "Número de tarjeta", "serialNumber" },
    ["2.5.4.6"]  = { "País", "countryName" },
    ["2.5.4.7"]  = { "Localidad", "localityName" },
    ["2.5.4.8"]  = { "Departamento", "stateOrProvinceName" },
    ["2.5.4.9"]  = { "Dirección", "streetAddress" },
    ["2.5.4.10"] = { "Emitido por", "organizationName" },
    ["2.5.4.11"] = { "Unidad organizativa", "organizationalUnitName" },
    ["2.5.4.12"] = { "Cargo", "title" },
    ["2.5.4.42"] = { "Nombres", "givenName" },
    ["2.5.4.97"] = { "Identificador de organización", "organizationIdentifier" },
    ["1.2.840.113549.1.9.1"]  = { "Correo electrónico", "emailAddress" },
    ["1.2.840.113549.1.1.1"]  = { "Clave RSA", "rsaEncryption" },
    ["1.2.840.113549.1.1.5"]  = { "Firma RSA con SHA-1", "sha1WithRSAEncryption" },
    ["1.2.840.113549.1.1.10"] = { "Firma RSA-PSS", "RSASSA-PSS" },
    ["1.2.840.113549.1.1.11"] = { "Firma RSA con SHA-256", "sha256WithRSAEncryption" },
    ["1.2.840.113549.1.1.12"] = { "Firma RSA con SHA-384", "sha384WithRSAEncryption" },
    ["1.2.840.113549.1.1.13"] = { "Firma RSA con SHA-512", "sha512WithRSAEncryption" },
    ["1.2.840.10045.2.1"]     = { "Clave de curva elíptica", "ecPublicKey" },
    ["1.2.840.10045.4.3.2"]   = { "Firma ECDSA con SHA-256", "ecdsa-with-SHA256" },
    ["1.2.840.10045.4.3.3"]   = { "Firma ECDSA con SHA-384", "ecdsa-with-SHA384" },
    ["2.16.840.1.101.3.4.2.1"] = { "SHA-256", "sha256" },
    ["1.2.840.10045.3.1.7"]   = { "Curva P-256", "prime256v1" },
    ["1.3.132.0.34"]          = { "Curva P-384", "secp384r1" },
    ["2.5.29.9"]  = { "Atributos del titular", "subjectDirectoryAttributes" },
    ["2.5.29.14"] = { "Identificador de clave del titular", "subjectKeyIdentifier" },
    ["2.5.29.15"] = { "Uso de la clave", "keyUsage" },
    ["2.5.29.16"] = { "Período de uso de la clave privada", "privateKeyUsagePeriod" },
    ["2.5.29.17"] = { "Nombre alternativo del titular", "subjectAltName" },
    ["2.5.29.18"] = { "Nombre alternativo del emisor", "issuerAltName" },
    ["2.5.29.19"] = { "Restricciones básicas", "basicConstraints" },
    ["2.5.29.31"] = { "Lista de revocación (CRL)", "cRLDistributionPoints" },
    ["2.5.29.32"] = { "Políticas del certificado", "certificatePolicies" },
    ["2.5.29.32.0"] = { "Cualquier política", "anyPolicy" },
    ["2.5.29.35"] = { "Identificador de clave del emisor", "authorityKeyIdentifier" },
    ["2.5.29.37"] = { "Uso extendido de la clave", "extKeyUsage" },
    ["1.3.6.1.5.5.7.1.1"]  = { "Acceso a información del emisor", "authorityInfoAccess" },
    ["1.3.6.1.5.5.7.1.3"]  = { "Declaraciones de certificado cualificado", "qcStatements" },
    ["1.3.6.1.5.5.7.2.1"]  = { "Declaración de prácticas (CPS)", "cps" },
    ["1.3.6.1.5.5.7.2.2"]  = { "Aviso al usuario", "userNotice" },
    ["1.3.6.1.5.5.7.3.1"]  = { "Autenticación de servidor", "serverAuth" },
    ["1.3.6.1.5.5.7.3.2"]  = { "Autenticación de cliente", "clientAuth" },
    ["1.3.6.1.5.5.7.3.4"]  = { "Protección de correo", "emailProtection" },
    ["1.3.6.1.4.1.311.20.2.2"] = { "Inicio de sesión con tarjeta", "smartcardLogon" },
    ["1.3.6.1.5.5.7.9.1"]  = { "Fecha de nacimiento", "dateOfBirth" },
    ["1.3.6.1.5.5.7.9.2"]  = { "Lugar de nacimiento", "placeOfBirth" },
    ["1.3.6.1.5.5.7.9.3"]  = { "Sexo", "gender" },
    ["1.3.6.1.5.5.7.9.4"]  = { "Nacionalidad", "countryOfCitizenship" },
    ["1.3.6.1.5.5.7.48.1"] = { "Servicio OCSP", "ocsp" },
    ["1.3.6.1.5.5.7.48.2"] = { "Certificado del emisor", "caIssuers" },
}

local unknown_oids = {}

-- Returns the Spanish label for an OID (or a generic one), and its
-- full description "label (technicalName, oid)".
local function oid_info(oid)
    local entry = OID_NAMES[oid]
    if entry then
        return entry[1], string.format("%s (%s, %s)", entry[1], entry[2], oid)
    end
    if not unknown_oids[oid] then
        unknown_oids[oid] = true
        log.print(log.INFO, "OID sin nombre en el script: " .. oid)
    end
    local label = oid
    if oid:match("^2%.16%.858%.") then
        label = "OID de Uruguay " .. oid
    end
    return label, label
end

local function decode_oid(data)
    local parts, value = {}, 0
    for i = 0, #data - 1 do
        local b = data:get(i)
        value = value * 128 + b % 128
        if b < 0x80 then
            if #parts == 0 then
                local first = math.min(math.floor(value / 40), 2)
                parts[1] = string.format("%d", first)
                parts[2] = string.format("%d", value - first * 40)
            else
                parts[#parts + 1] = string.format("%d", value)
            end
            value = 0
        end
    end
    return table.concat(parts, ".")
end

--------------------------------------------------------------------------
-- Tree helpers
--------------------------------------------------------------------------

-- Appends "label: value" to the tree. When the raw bytes are text, the
-- value is stored as that text so it is also readable in cardpeek's raw
-- view and exports; binary values keep the original bytes.
local function add_item(parent, label, raw, text, classname)
    local spec = { classname = classname or "item", label = label }
    if raw ~= nil then
        spec.val = readable_text(raw) or raw
        spec.size = #raw
    end
    local node = parent:append(spec)
    if text ~= nil and text ~= "" then
        node:set_attribute("alt", text)
    end
    return node
end

--------------------------------------------------------------------------
-- ASN.1 value decoding
--------------------------------------------------------------------------

-- Total length (header + value) of the DER element at the start of
-- data, or nil if the header is malformed or the value is truncated.
-- Checked before asn1.split(), which logs errors on invalid input.
local function der_element_length(data)
    local n = #data
    if n < 2 then return nil end
    local i = 1
    if bit.AND(data:get(0), 0x1F) == 0x1F then
        while i < n and bit.AND(data:get(i), 0x80) ~= 0 do i = i + 1 end
        i = i + 1
    end
    if i >= n then return nil end
    local l = data:get(i)
    local header, len = i + 1, l
    if l == 0x80 or l > 0x83 then
        return nil
    elseif l > 0x80 then
        local count = l - 0x80
        if i + count >= n then return nil end
        len = 0
        for k = 1, count do len = len * 256 + data:get(i + k) end
        header = i + 1 + count
    end
    if header + len > n then return nil end
    return header + len
end

-- Splits DER content into a list of { tag, value } elements.
local function der_list(data)
    local out = {}
    local rest = data
    while rest and #rest > 0 and der_element_length(rest) do
        local tag, value, tail = asn1.split(rest)
        if tag == nil or tag == 0 then
            break
        end
        out[#out + 1] = { tag = tag, value = value or bytes.new(8) }
        rest = tail
    end
    return out
end

local function is_constructed(tag)
    while tag > 255 do
        tag = math.floor(tag / 256)
    end
    return bit.AND(tag, 0x20) ~= 0
end

local function integer_text(data)
    if #data == 0 then
        return ""
    end
    if #data <= 20 then
        local dec = bytes_to_decimal(data)
        if #data > 2 then
            local text = (#data >= 4) and readable_text(data) or nil
            return dec .. "  (" .. hex_colon(data) .. ")" ..
                   (text and ("  \"" .. text .. "\"") or "")
        end
        return dec
    end
    local bits = (data:get(0) == 0) and (#data - 1) * 8 or #data * 8
    return string.format("%d bits", bits)
end

-- UTCTime / GeneralizedTime -> "DD/MM/YYYY hh:mm:ss UTC"
local function time_text(data, date_only)
    local s = bytes.format(data, "%C")
    local y, mo, d, h, mi, rest = s:match("^(%d%d%d%d)(%d%d)(%d%d)(%d%d)(%d%d)(.*)$")
    if #s < 15 or y == nil then
        local yy
        yy, mo, d, h, mi, rest = s:match("^(%d%d)(%d%d)(%d%d)(%d%d)(%d%d)(.*)$")
        if yy == nil then
            return ascii_dump(data)
        end
        y = (tonumber(yy) < 50 and "20" or "19") .. yy
    end
    if date_only then
        return string.format("%s/%s/%s", d, mo, y)
    end
    local sec = rest:match("^(%d%d)") or "00"
    local zone = rest:match("Z$") and " UTC" or (rest:match("([%+%-]%d%d%d%d)$") or "")
    return string.format("%s/%s/%s %s:%s:%s%s", d, mo, y, h, mi, sec, zone)
end

-- Readable text for a primitive ASN.1 value.
local function value_text(tag, value)
    if tag == 0x06 then
        return (oid_info(decode_oid(value)))
    elseif tag == 0x02 or tag == 0x0A then
        return integer_text(value)
    elseif tag == 0x01 then
        return (#value > 0 and value:get(0) ~= 0) and "sí" or "no"
    elseif tag == 0x17 or tag == 0x18 then
        return time_text(value)
    elseif tag == 0x1E then
        return bmp_to_utf8(value) or ascii_dump(value)
    elseif tag == 0x05 then
        return ""
    end
    return bytes_text(value)
end

-- Collects the readable values found anywhere inside DER content.
local function collect_texts(data, out)
    out = out or {}
    for _, e in ipairs(der_list(data)) do
        if is_constructed(e.tag) then
            collect_texts(e.value, out)
        elseif e.tag ~= 0x05 and e.tag ~= 0x01 then
            local t = value_text(e.tag, e.value)
            if t ~= "" then
                out[#out + 1] = t
            end
        end
    end
    return out
end

local function first_of(data, tag)
    for _, e in ipairs(der_list(data)) do
        if e.tag == tag then
            return e.value
        end
    end
    return nil
end

--------------------------------------------------------------------------
-- X.509 certificate
--------------------------------------------------------------------------

-- Name ::= SEQUENCE OF SET OF { type OID, value }  ->  { {label, text, raw} }
local function name_fields(name)
    local fields = {}
    for _, rdn in ipairs(der_list(name)) do
        for _, atv in ipairs(der_list(rdn.value)) do
            local f = der_list(atv.value)
            if f[1] and f[1].tag == 0x06 and f[2] then
                fields[#fields + 1] = { (oid_info(decode_oid(f[1].value))),
                                        value_text(f[2].tag, f[2].value), f[2].value }
            end
        end
    end
    return fields
end

local function name_summary(fields)
    local cn, parts = nil, {}
    for _, f in ipairs(fields) do
        if f[1] == "Nombre y apellido" then
            cn = f[2]
        end
        parts[#parts + 1] = f[2]
    end
    return cn or table.concat(parts, ", ")
end

local function add_name(parent, label, name)
    local fields = name_fields(name)
    local node = add_item(parent, label, nil, name_summary(fields), "block")
    for _, f in ipairs(fields) do
        add_item(node, f[1], f[3], f[2])
    end
    return node
end

local GENERAL_NAMES = {
    [0x81] = "Correo electrónico",
    [0x82] = "Nombre DNS",
    [0x86] = "URL",
    [0x87] = "Dirección IP",
}

-- GeneralNames: adds one item per name, returns their texts.
local function add_general_names(parent, data)
    local texts = {}
    for _, gn in ipairs(der_list(data)) do
        local label, text = GENERAL_NAMES[gn.tag], nil
        if gn.tag == 0x87 then
            local b = {}
            for i = 0, #gn.value - 1 do b[#b + 1] = tostring(gn.value:get(i)) end
            text = table.concat(b, ".")
        elseif gn.tag == 0xA4 then
            label = "Nombre X.500"
            local inner = der_list(gn.value)[1]
            text = inner and name_summary(name_fields(inner.value)) or ""
        elseif gn.tag == 0xA0 then
            -- otherName { type-id OID, [0] value }
            local f = der_list(gn.value)
            label = (f[1] and f[1].tag == 0x06) and (oid_info(decode_oid(f[1].value))) or "Otro nombre"
            text = table.concat(collect_texts(f[2] and f[2].value or bytes.new(8)), ", ")
        elseif label then
            text = bytes_text(gn.value)
        else
            label = string.format("Nombre [%d]", bit.AND(gn.tag, 0x1F))
            text = table.concat(collect_texts(gn.value), ", ")
            if text == "" then text = bytes_text(gn.value) end
        end
        add_item(parent, label, gn.value, text)
        texts[#texts + 1] = text
    end
    return texts
end

local KEY_USAGE = { "firma digital", "no repudio", "cifrado de claves",
                    "cifrado de datos", "acuerdo de claves", "firma de certificados",
                    "firma de CRL", "solo cifrado", "solo descifrado" }

-- Extension decoders: (node, extnValue content) -> summary text.
local EXTENSIONS = {}

EXTENSIONS["2.5.29.15"] = function(node, v) -- keyUsage
    local bs = der_list(v)[1]
    if not bs or #bs.value < 2 then return "" end
    local unused, set = bs.value:get(0), {}
    for i = 0, (#bs.value - 1) * 8 - 1 - unused do
        local byte = bs.value:get(1 + math.floor(i / 8))
        if bit.AND(byte, bit.SHR(0x80, i % 8)) ~= 0 then
            set[#set + 1] = KEY_USAGE[i + 1] or ("bit " .. i)
        end
    end
    return table.concat(set, ", ")
end

EXTENSIONS["2.5.29.37"] = function(node, v) -- extKeyUsage
    local names = {}
    for _, e in ipairs(der_list(der_list(v)[1].value)) do
        local n = value_text(e.tag, e.value)
        add_item(node, "Uso", e.value, n)
        names[#names + 1] = n
    end
    return table.concat(names, ", ")
end

EXTENSIONS["2.5.29.19"] = function(node, v) -- basicConstraints
    local ca, pathlen = false, nil
    for _, e in ipairs(der_list(der_list(v)[1].value)) do
        if e.tag == 0x01 then ca = #e.value > 0 and e.value:get(0) ~= 0 end
        if e.tag == 0x02 then pathlen = integer_text(e.value) end
    end
    if ca then
        return "Autoridad certificadora (CA)" ..
               (pathlen and (", longitud máxima de cadena " .. pathlen) or "")
    end
    return "Certificado final (no es CA)"
end

EXTENSIONS["2.5.29.14"] = function(node, v) -- subjectKeyIdentifier
    local id = der_list(v)[1]
    return id and bytes_text(id.value) or ""
end

EXTENSIONS["2.5.29.35"] = function(node, v) -- authorityKeyIdentifier
    local seq = der_list(v)[1]
    local keyid = seq and first_of(seq.value, 0x80)
    return keyid and bytes_text(keyid) or table.concat(collect_texts(v), ", ")
end

local function general_names_ext(node, v)
    local seq = der_list(v)[1]
    return seq and table.concat(add_general_names(node, seq.value), ", ") or ""
end
EXTENSIONS["2.5.29.17"] = general_names_ext -- subjectAltName
EXTENSIONS["2.5.29.18"] = general_names_ext -- issuerAltName

EXTENSIONS["2.5.29.31"] = function(node, v) -- cRLDistributionPoints
    local urls = {}
    local function find_uris(data)
        for _, e in ipairs(der_list(data)) do
            if e.tag == 0x86 then
                local u = readable_text(e.value) or ascii_dump(e.value)
                add_item(node, "URL", e.value, u)
                urls[#urls + 1] = u
            elseif is_constructed(e.tag) then
                find_uris(e.value)
            end
        end
    end
    find_uris(v)
    return table.concat(urls, ", ")
end

EXTENSIONS["1.3.6.1.5.5.7.1.1"] = function(node, v) -- authorityInfoAccess
    local texts = {}
    for _, ad in ipairs(der_list(der_list(v)[1].value)) do
        local f = der_list(ad.value)
        if f[1] and f[1].tag == 0x06 and f[2] then
            local label = oid_info(decode_oid(f[1].value))
            local loc = readable_text(f[2].value) or table.concat(collect_texts(f[2].value), ", ")
            add_item(node, label, f[2].value, loc)
            texts[#texts + 1] = label .. ": " .. loc
        end
    end
    return table.concat(texts, ", ")
end

EXTENSIONS["2.5.29.32"] = function(node, v) -- certificatePolicies
    local texts = {}
    for _, pi in ipairs(der_list(der_list(v)[1].value)) do
        local f = der_list(pi.value)
        if f[1] and f[1].tag == 0x06 then
            local policy = oid_info(decode_oid(f[1].value))
            local pnode = add_item(node, "Política", f[1].value, policy)
            texts[#texts + 1] = policy
            -- policyQualifiers: { OID, qualifier }
            for _, q in ipairs(f[2] and der_list(f[2].value) or {}) do
                local qf = der_list(q.value)
                if qf[1] and qf[1].tag == 0x06 and qf[2] then
                    add_item(pnode, (oid_info(decode_oid(qf[1].value))), qf[2].value,
                             is_constructed(qf[2].tag)
                                 and table.concat(collect_texts(qf[2].value), ", ")
                                 or value_text(qf[2].tag, qf[2].value))
                end
            end
        end
    end
    return table.concat(texts, ", ")
end

EXTENSIONS["2.5.29.9"] = function(node, v) -- subjectDirectoryAttributes
    local texts = {}
    for _, attr in ipairs(der_list(der_list(v)[1].value)) do
        local f = der_list(attr.value)
        if f[1] and f[1].tag == 0x06 and f[2] then
            local oid = decode_oid(f[1].value)
            local label = oid_info(oid)
            local vals = {}
            for _, e in ipairs(der_list(f[2].value)) do
                if e.tag == 0x18 and oid == "1.3.6.1.5.5.7.9.1" then
                    vals[#vals + 1] = time_text(e.value, true)
                elseif is_constructed(e.tag) then
                    vals[#vals + 1] = table.concat(collect_texts(e.value), ", ")
                else
                    vals[#vals + 1] = value_text(e.tag, e.value)
                end
            end
            add_item(node, label, f[2].value, table.concat(vals, ", "))
            texts[#texts + 1] = label .. ": " .. table.concat(vals, ", ")
        end
    end
    return table.concat(texts, ", ")
end

local function add_extensions(parent, data)
    local list = der_list(data)[1]
    if not list then return end
    local exts = add_item(parent, "Extensiones", nil, nil, "block")
    local names = {}

    for _, ext in ipairs(der_list(list.value)) do
        local f = der_list(ext.value)
        if f[1] and f[1].tag == 0x06 then
            local oid = decode_oid(f[1].value)
            local label = oid_info(oid)
            local critical = f[2] and f[2].tag == 0x01 and #f[2].value > 0 and f[2].value:get(0) ~= 0
            local value = f[#f].value
            local node = add_item(exts, label, value)

            local text
            local decoder = EXTENSIONS[oid]
            if decoder then
                local ok, res = pcall(decoder, node, value)
                text = ok and res or nil
            end
            if text == nil then
                text = (der_element_length(value) == #value)
                       and table.concat(collect_texts(value), ", ") or ""
                if text == "" then text = bytes_text(value) end
            end
            if critical then
                text = "[crítica] " .. text
            end
            node:set_attribute("alt", text)
            names[#names + 1] = label
        end
    end
    exts:set_attribute("alt", table.concat(names, ", "))
end

local function add_public_key(parent, spki)
    local f = der_list(spki)
    if #f < 2 then return end
    local alg = der_list(f[1].value)
    local alg_oid = alg[1] and decode_oid(alg[1].value) or ""
    local text = alg[1] and (oid_info(alg_oid)) or "?"
    local node = add_item(parent, "Clave pública", f[2].value, text)

    local key = bytes.sub(f[2].value, 1, #f[2].value - 1) -- skip "unused bits"
    if alg_oid == "1.2.840.113549.1.1.1" then
        local rsa = der_list(key)[1]
        local parts = rsa and der_list(rsa.value) or {}
        if #parts >= 2 then
            text = text .. " de " .. integer_text(parts[1].value)
            add_item(node, "Módulo", parts[1].value, integer_text(parts[1].value))
            add_item(node, "Exponente", parts[2].value, integer_text(parts[2].value))
        end
    elseif alg_oid == "1.2.840.10045.2.1" and alg[2] and alg[2].tag == 0x06 then
        text = text .. ", " .. oid_info(decode_oid(alg[2].value))
        add_item(node, "Punto público", key, string.format("%d bytes", #key))
    end
    node:set_attribute("alt", text)
end

local function parse_certificate(file_node, data, info)
    local cert = der_list(data)[1]
    if not cert or cert.tag ~= 0x30 then
        file_node:set_attribute("alt", "No es un certificado X.509")
        return
    end
    -- Drop trailing padding after the certificate.
    local der = asn1.join(cert.tag, cert.value)
    file_node:set_attribute("val", der)
    file_node:set_attribute("size", #der)

    local top = der_list(cert.value)               -- tbs, algorithm, signature
    local tbs = der_list(top[1].value)
    local i, version = 1, 1
    if tbs[1] and tbs[1].tag == 0xA0 then
        local v = der_list(tbs[1].value)[1]
        version = v and (tonumber(bytes_to_decimal(v.value)) + 1) or 1
        i = 2
    end
    local serial, issuer, validity = tbs[i], tbs[i + 2], tbs[i + 3]
    local subject, spki = tbs[i + 4], tbs[i + 5]

    local subject_name = subject and name_summary(name_fields(subject.value)) or ""
    if subject then add_name(file_node, "Titular", subject.value) end
    if issuer then add_name(file_node, "Emisor", issuer.value) end

    local not_after = ""
    if validity then
        local t = der_list(validity.value)
        if #t == 2 then
            local from, to = time_text(t[1].value), time_text(t[2].value)
            not_after = to
            local vnode = add_item(file_node, "Validez", nil, from .. "  →  " .. to, "block")
            add_item(vnode, "Válido desde", t[1].value, from)
            add_item(vnode, "Válido hasta", t[2].value, to)
        end
    end

    if serial then
        add_item(file_node, "Número de serie", serial.value, integer_text(serial.value))
    end
    add_item(file_node, "Versión", nil, "v" .. version)

    local sig_alg = top[2] and der_list(top[2].value)[1]
    local sig_alg_text = sig_alg and (oid_info(decode_oid(sig_alg.value))) or "?"
    add_item(file_node, "Algoritmo de firma", sig_alg and sig_alg.value, sig_alg_text)

    if spki then add_public_key(file_node, spki.value) end

    for j = i + 6, #tbs do
        if tbs[j].tag == 0xA3 then
            add_extensions(file_node, tbs[j].value)
        end
    end

    if top[3] then
        local sig = bytes.sub(top[3].value, 1, #top[3].value - 1)
        add_item(file_node, "Firma del emisor", sig,
                 string.format("%s, %d bits", sig_alg_text, #sig * 8))
    end

    file_node:set_attribute("alt", string.format("%s, válido hasta %s", subject_name, not_after))
    info.cert_subject = subject_name
end

--------------------------------------------------------------------------
-- Main processing
--------------------------------------------------------------------------

local CERT_FID = "B001"

local function read_version(app)
    local sw, resp = send(bytes.new(8, "00 CA 7F 30 00"))
    if sw ~= 0x9000 or #resp == 0 then
        log.print(log.INFO, string.format("GET DATA 7F30 falló: SW=%04X", sw))
        return
    end
    local tag, value = asn1.split(resp)
    if tag ~= 0x7F30 then
        return
    end

    local label, version
    for _, e in ipairs(der_list(value)) do
        if e.tag == 0xC0 then label = to_text(e.value) end
        if e.tag == 0xC1 then version = to_text(e.value) end
    end

    local chip
    if label and label:match("v4$") then
        chip = "Chip 2015, híbrida (Match-on-Card P2=21)"
    elseif label and label:match("v5$") then
        chip = "Chip 2022, interfaz dual (Match-on-Card P2=4A, PACE)"
    end
    local node = add_item(app, "Chip", resp,
                          chip or table.concat({ label or "", version or "" }, " "))
    if label then add_item(node, "Applet", nil, label) end
    if version then add_item(node, "Versión", nil, version) end
    if chip then log.print(log.INFO, chip) end
end

local function read_certificate(app, info)
    local node = app:append({ classname = "file", label = "Certificado de firma", id = CERT_FID })

    local sw, fci = select_file(CERT_FID)
    if sw ~= 0x9000 then
        node:set_attribute("alt", string.format("No se pudo seleccionar (SW=%04X)", sw))
        log.print(log.WARNING, string.format("SELECT %s falló: SW=%04X", CERT_FID, sw))
        return
    end

    local data
    sw, data = read_selected(fci_file_size(fci))
    if #data == 0 then
        node:set_attribute("alt", string.format("Sin contenido (SW=%04X)", sw))
        log.print(log.WARNING, string.format("READ BINARY %s falló: SW=%04X", CERT_FID, sw))
        return
    end
    if sw ~= 0x9000 then
        log.print(log.WARNING, string.format("READ BINARY %s incompleto: SW=%04X", CERT_FID, sw))
    end

    node:set_attribute("size", #data)
    node:set_attribute("val", data)
    log.print(log.INFO, string.format("Archivo %s: %d bytes", CERT_FID, #data))

    local ok, err = pcall(parse_certificate, node, data, info)
    if not ok then
        log.print(log.WARNING, "Error al decodificar el certificado: " .. tostring(err))
    end
end

local function process_eid(root)
    card.CLA = 0x00

    local sw = select_ias()
    if sw ~= 0x9000 then
        log.print(log.ERROR, string.format("SELECT del applet IAS falló: SW=%04X", sw))
        add_item(root, "Error", bytes.new(8, bit.SHR(sw, 8), bit.AND(sw, 0xFF)),
                 string.format("No se pudo seleccionar el applet IAS (SW=%04X). ¿Es una cédula uruguaya?", sw))
        return false
    end

    local app = root:append({ classname = "application",
                              label = "Cédula de identidad (IAS)",
                              id = (IAS_AID:gsub(" ", "")) })
    read_version(app)

    local info = {}
    read_certificate(app, info)

    if info.cert_subject then
        app:set_attribute("alt", info.cert_subject)
        log.print(log.INFO, "Certificado: " .. info.cert_subject)
    end
    return true
end

if card.connect() then
    local root = card.tree_startup("Cédula de identidad uruguaya")
    process_eid(root)
    card.disconnect()
end
