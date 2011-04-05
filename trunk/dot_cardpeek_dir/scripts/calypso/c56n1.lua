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

require('lib.strict')
require('lib.en1545')

function mobib_BIRTHDATE(source)
	local source_digits = bytes.convert(4,source)
	local year   = tonumber(bytes.format("%D",bytes.sub(source_digits,0,3)))
	local month  = tonumber(bytes.format("%D",bytes.sub(source_digits,4,5)))
	local day    = tonumber(bytes.format("%D",bytes.sub(source_digits,6,7)))
	return os.date("%x",os.time({['year']=year,['month']=month,['day']=day}))
end

function mobib_HOLDERNUMBER(source)
	local source_digits = bytes.convert(4,source)
	return  tostring(bytes.sub(source_digits,0,5)).."/"..
		tostring(bytes.sub(source_digits,6,17)).."/"..
		tostring(bytes.sub(source_digits,18,18))

end

mobib_HolderExtension = {
  [0] = { en1545_UNDEFINED, 18, "HolderUnknownData1" },
  [1] = { mobib_HOLDERNUMBER, 76, "HolderNumber" },
  [2] = { en1545_UNDEFINED, 74, "HolderUnknownData2" },
  [3] = { mobib_BIRTHDATE, 32, "HolderBirthDate" },
  [4] = { en1545_UNDEFINED,  5, "HolderUnknownData3" },
  [5] = { en1545_ALPHA, 160, "HolderName" }
}

function mobib_process(APP)
	local HOLDER_EXT
	local RECORD_1
	local DATA_REF_1
	local RECORD_2
	local DATA_REF_2
	local data

	HOLDER_EXT = ui.tree_find_node(APP,"Holder Extended")
	RECORD_1 = ui.tree_find_node(HOLDER_EXT,"record",1)
	if RECORD_1==nil then 
		log.print(LOG_ERROR,"Could not find record 1 in 'Holder Extended' file")
		return false
	end
	DATA_REF_1 = ui.tree_find_node(RECORD_1,"raw data")
	if DATA_REF_1==nil then 
		log.print(LOG_ERROR,"Could not find data in record 1 of 'Holder Extended' file")
		return false
	end	
	RECORD_2 = ui.tree_find_node(HOLDER_EXT,"record",2)
	if RECORD_2==nil then 
		log.print(LOG_ERROR,"Could not find record 2 in 'Holder Extended' file")
		return false
	end
	DATA_REF_2 = ui.tree_find_node(RECORD_2,"raw data")
	if DATA_REF_1==nil then 
		log.print(LOG_ERROR,"Could not find data in record 2 of 'Holder Extended' file")
		return false
	end	
	
	data = ui.tree_get_value(DATA_REF_1) .. ui.tree_get_value(DATA_REF_2)
	
	ui.tree_delete_node(RECORD_2)
	ui.tree_delete_node(RECORD_1)
	-- print("deleting ",RECORD_1,RECORD_2)
	RECORD_1 = ui.tree_add_node(HOLDER_EXT,"record",1,#data)
	DATA_REF_1 = ui.tree_add_node(RECORD_1,"raw data")
	ui.tree_set_value(DATA_REF_1,data)
	en1545_map(APP,"Holder Extended",mobib_HolderExtension)
end

mobib_process(CARD)


