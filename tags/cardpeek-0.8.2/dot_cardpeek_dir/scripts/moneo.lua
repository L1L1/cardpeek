--
-- This file is part of Cardpeek, the smartcard reader utility.
--
-- Copyright 2009i-2013 by 'L1L1'
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
-- @name Moneo
-- @description French stored value card
-- @targets 0.8

--
-- April 2013: Added contributions from Darius Matboo who provided a 
--             human readable interpretation of some of the data. 
--

require('lib.apdu')
require('lib.tlv')

function BCD(data,index)
	return tostring(string.format("%x",data:get(index)));
end

function BCD_amount(data)
	return string.format("%i,%i Euros", BCD(data,0)*100+BCD(data,1), BCD(data,2))
end

function BCD_date(data)
	return string.format("%02i/%02i/%i", BCD(data,2), BCD(data,1), 2000+BCD(data,0)) 
end

function BCD_date_time(data)
	return string.format("%02i/%02i/%i %02i:%02i:%02i", BCD(data,3), BCD(data,2), BCD(data,0)*100+BCD(data,1),
               					            BCD(data,4), BCD(data,5), BCD(data,6)) 
end



function moneo_identifiants(node,data)

	node:append({ classname = 'item', 
		      label = 'PAN', 
		      val = data:sub(0,9),
		      alt = tostring(data:sub(0,9)):sub(1,19)
		    })
 	
	node:append({ classname = 'item', 
		      label = "Date d'expiration", 
		      val = data:sub(10,11),
		      alt = string.format("%i/%i", BCD(data,11), BCD(data,10)+2000) 
                    })

	node:append({ classname = 'item', 
		      label = "Date d'activation", 
		      val = data:sub(12,14),
		      alt = BCD_date(data:sub(12,14)) 
                    })
	
	node:append({ classname = 'item', 
		      label = "Code pays", 
		      val = data:sub(15,16),
		      alt = tostring(data:sub(15,16))
                    })

	node:append({ classname = 'item', 
		      label = "Code monnaie", 
		      val = data:sub(17,19),
		      alt = data:sub(17,19):format("%C") 
                    })	

end

function moneo_journal_rechargements(node,data)

	node:append({ classname = 'item', 
		      label = 'Compteur de rechargements', 
		      val = data:sub(1,2),
		      alt = data:sub(1,2):tonumber()
		    })
 	
	node:append({ classname = 'item', 
		      label = 'Montant de recharge', 
		      val = data:sub(4,6),
		      alt = BCD_amount(data:sub(4,6)) 
                    })

	node:append({ classname = 'item',
		      label = 'Reste',
		      val = data:sub(7,9),
		      alt = BCD_amount(data:sub(7,9))
		    })

	node:append({ classname = 'item', 
		      label = 'Terminal', 
		      val = data:sub(13,20),
		      alt = tostring(data:sub(13,20)) 
                    })

	node:append({ classname = 'item', 
		      label = 'Date', 
		      val = data:sub(24,30),
		      alt = BCD_date_time(data:sub(24,30))
		    })

end

function moneo_journal_des_achats(node,data)

	node:append({ classname = 'item', 
                      label = "Compteur d'achats", 
                      val = data:sub(1,2),
                      alt = data:sub(1,2):tonumber()
                    })
        
        node:append({ classname = 'item', 
                      label = 'Montant de la transaction', 
                      val = data:sub(5,7),
                      alt = BCD_amount(data:sub(5,7))  
                    })

        node:append({ classname = 'item', 
                      label = 'Terminal', 
                      val = data:sub(8,17),
                      alt = tostring(data:sub(8,17)):sub(1,19)
                    })

        node:append({ classname = 'item', 
                      label = 'Date', 
                      val = data:sub(29,35),
                      alt = BCD_date_time(data:sub(29,35))
                    })

        node:append({ classname = 'item',
                      label = 'Reste',
                      val = data:sub(26,28),
                      alt = BCD_amount(data:sub(26,28))
                    })

end

function moneo_montants(node,data)

	node:append({ classname = 'item', 
                      label = 'Credit actuel', 
                      val = data:sub(0,2),
                      alt = BCD_amount(data:sub(0,2))  
                    })

	node:append({ classname = 'item', 
                      label = 'Credit maximum', 
                      val = data:sub(3,5),
                      alt = BCD_amount(data:sub(3,5))  
                    })

	node:append({ classname = 'item', 
                      label = 'Montant maximum de transaction', 
                      val = data:sub(6,8),
                      alt = BCD_amount(data:sub(6,8))  
                    })

end

function moneo_seqnum(node,data)
	
	node:set_attribute("alt",data:tonumber()) 

end

MONEO_SFI = {
  { 'Rechargement rapide', 0x08 },
  { 'Numero de sequence de rechargement rapide', 0x0A, moneo_seqnum },
  { 'ICP', 0x0C },
  { 'Identifiants', 0x17, moneo_identifiants },
  { 'Montants', 0x18, moneo_montants },
  { 'Type de porte-monnaie', 0x19 },
  { 'Numero de sequence de rechargement', 0x1A, moneo_seqnum },
  { "Numero de sequence d'achat", 0x1B, moneo_seqnum },
  { 'Journal des rechargements', 0x1C, moneo_journal_rechargements },
  { 'Journal des achats', 0x1D, moneo_journal_des_achats }

}

function process_moneo(card_ctx)
	local sw, resp
	local APP
	local AID = "#A00000006900"
	local r
	local SFI
	local i,v
	local NODE

	sw, resp = card.select(AID,nil,0)

	if (#resp==0) then
	   return FALSE
	end

	-- this is for a strange bug seen in some moneo cards: --
	if resp:get(1)>=0x82 then
	   asn1.enable_single_byte_length(true)
	end

	APP = nodes.append(card_ctx, { classname="application", label="application", id=AID })
	tlv_parse(APP,resp)
	for i,v in ipairs(MONEO_SFI) do
	    SFI = nodes.append(APP,{ classname="file", label=v[1], id=string.format("%02X",v[2]) })
	    for r=1,255 do
	        sw,resp = card.read_record(v[2],r)
	        if (#resp==0) then
                   break
		end
		NODE = nodes.append(SFI, { classname="record", label="record", id=r, size=#resp, val=resp })
		if v[3] then
			v[3](NODE,resp)
		end
	    end
	end
end



if card.connect() then
  CARD = card.tree_startup("MONEO")
  process_moneo(CARD)
  card.disconnect()
else
  ui.question("No card detected",{"OK"})
end

