--
-- This file is part of Cardpeek, the smartcard reader utility.
--
-- Copyright 2009 by 'L1L1'
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

require('lib.tlv')

VITALE_IDO = {
  ['65'] = "Bénéficiaire",
  ['65/90'] = "Nationalité",
  ['65/93'] = "Numéro de sécurité sociale",
  ['65/80'] = "Nom",
  ['65/81'] = "Prénom",
  ['65/82'] = "Date de naissance",
  ['66'] = "Carte vitale",
  ['66/80'] = "Date de début de validité",
  ['7F21']  = "Certificat",
  ['5F37']  = "Signature de l'AC",
  ['5F38']  = "Résidu de la clé publique",
}

function read_bin()
        local total, sw, resp, r

	total = bytes.new(8)
	r = 0
	repeat 
          sw, resp = card.read_binary(".",r)
	  bytes.append(total,resp)
	  r = r + 256
	until #resp~=256 or sw~=0x9000

        if #total>0 then
	   return 0x9000, total
	end
        return sw, total
end


AID_VITALE    = "#D250000002564954414C45"
AID_VITALE_MF = "#D2500000024D465F564954414C45"

function create_card_map()
	local resp, sw
	local map = {}
	local tag,val,rest
	local tag2,val2,rest2
	local entry, aid, file, name

	sw,resp = card.select(AID_VITALE)
	if sw~=0x9000 then
	   return nil
	end
	sw,resp = card.select(".2001")
	if sw~=0x9000 then
	   return nil
	end
	sw,resp = read_bin()
	if sw~=0x9000 then 
	   return nil
	end
	tag,val = asn1.split(resp) -- E0,DATA,nil
	tag,rest2,rest = asn1.split(val) -- A0,DATA,DATA
	
	repeat
	  tag2,val2,rest2 = asn1.split(rest2)
	  if tag2==0x82 then
	     entry = tostring(bytes.sub(val2,0,1))
	     aid   = "#"..tostring(bytes.sub(val2,4,-1))
	     map[entry]={ ['aid'] = aid, ['files'] = {} }
	  end
	until rest2==nil or tag==0
	repeat
	  tag,val,rest = asn1.split(rest)
	  if tag==0x80 then
	     entry = tostring(bytes.sub(val,8,9))
	     file  = "."..tostring(bytes.sub(val,10,11))
	     name  = bytes.toprintable(bytes.sub(val,0,7))
	     table.insert(map[entry]['files'],{ ['name']=name, ['ef']=file })
	  end
	until rest==nil or tag==0

	for entry in pairs(map) do
	  if map[entry]['aid']==AID_VITALE_MF then
	     table.insert(map[entry]['files'],{ ['name']="DIR", ['ef']=".2F00" })
	     table.insert(map[entry]['files'],{ ['name']="PAN", ['ef']=".2F02" })
	  elseif map[entry]['aid']==AID_VITALE then
	     table.insert(map[entry]['files'],{ ['name']="POINTERS", ['ef']=".2001" })
	  end
	end

	return map
end

--[[
AID = { 
  "D2500000024D465F564954414C45",
  "E828BD080FD2500000044164E86C65",
  "D2500000044164E86C650101"
}
--]]


card.connect()
CARD = card.tree_startup("ATR")

map = create_card_map()

if map then
   for app in pairs(map) do
       sw,resp = card.select(map[app]['aid'])
       if sw==0x9000 then
          APP = ui.tree_add_node(CARD,"AID",map[app]['aid'])
	  tlv_parse(APP,resp)
       end
       for i in pairs(map[app]['files']) do
          EF = ui.tree_add_node(APP,"file",map[app]['files'][i]['name'],nil,"EF="..map[app]['files'][i]['ef'])
	  sw,resp = card.select(map[app]['files'][i]['ef'])
	  tlv_parse(EF,resp)
	  if sw==0x9000 then
	     sw,resp = read_bin()
	     CONTENT = ui.tree_add_node(EF,"content")
	     if sw==0x9000 then
	        if resp[0]==0 or (resp[0]==0x04 and resp[1]==0x00) then
                  ui.tree_set_value(CONTENT,tostring(resp))
	        else
                  tlv_parse(CONTENT,resp,VITALE_IDO)
                end
	     else
	        ui.tree_set_value(CONTENT,string.format("data not accessible (code %04X)",sw))
	     end
	  end
       end
   end
end

card.disconnect()
