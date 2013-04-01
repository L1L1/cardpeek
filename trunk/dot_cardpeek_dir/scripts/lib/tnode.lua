

if not __original_tree_add_node then
 __original_tree_add_node = ui.tree_add_node
 __original_tree_delete_node = ui.tree_delete_node
end

require('lib.strict')

function ui.tree_add_node(node,classname,label,id,length)
	tNode.cache = {}
        return __original_tree_add_node(node,classname,label,id,length);
end

function ui.tree_delete_node(node)
	tNode.cache = {}
	return __original_tree_delete_node(node);
end

function check_self(self)
	if self==nil then
		local func   = debug.getinfo(2,"n").name
		log.print(log.ERROR,"Did you write '." .. func ..  "()' intead of ':" .. func .. "()'?")
		error("missing parameter in call to " .. func .. "()")
	end
end

tNode = {

  cache = { },

  internal_pattern = function(obj)
	local ret = { }
	local m

  	if string.match(obj,"^#.+") then
		ret["id"]= string.sub(obj,2)
	else
		m = { string.match(obj,"^([^#]+)#(.+)") }
		if m[1] then
			ret["label"] = m[1]
			ret["id"] = m[2]
		else
			ret["label"] = obj
		end
	end
	return ret
  end,

  internal_cache_get = function(node,root)
  	local obj_ref

	if root then
		obj_ref = tostring(root) .. ">" .. node
	else
		obj_ref = "@>" .. node
	end
	return tNode.cache[obj_ref]
  end, 

  internal_cache_set = function(node,root,val)
	local obj_ref

	if root then
		obj_ref = tostring(root) .. ">" .. node
	else
		obj_ref = "@>" .. node
	end
	tNode.cache[obj_ref] = val
  end,

  new = function(self,expression,root)
	  local o
	  local m
	  local cached_obj

	  if root~=nil and type(root)~="userdata" then
		log.print(log.ERROR,"Optional last parameter of tNode.new must be a node");
		return nil 
	  end
  
  	  if expression then
		if type(expression)=="string" then

			cached_obj = tNode.internal_cache_get(expression,root)
	  	  	
			if cached_obj then
	    			o = { ["items"] = cached_object }
  			else 
	   			local pat = tNode.internal_pattern(expression)
	    			o = { ["items"] = ui.tree_find_all_nodes(nil,pat["label"],pat["id"]) }
				if #o.items==0 then
					log.print(log.DEBUG,"tNode.new('"..expression.."') returns an empty set.")
				else
					tNode.internal_cache_set(expression,root,o.items)
				end
	      		end
		
	      	elseif type(expression)=="userdata" then

			o = { ["items"] = { expression } } 

		elseif type(expression)=="table" and getmetatable(expression)==tNode then

			log.print(log.WARNING,"tNode:new(x) or _n(x) was called on an object x that is already a tNode.")
			o = { ["items"] = expression["items"] }

		else

			log.print(log.ERROR,"calling tNode:new(x) on a value x that is not a string, a tNode or a node.")	
			return nil
		end
	  else
	      	o = { ["items"] = {} }
  	  end

	  self.__index = self;
          return setmetatable(o,self)
  end,

  each = function(self,f)
	local i,v;
	for i,v in ipairs(self.items) do f(i,v) end
	return self
  end,

  get = function(self,index)
	if index then
		return self.items[index]
	end
	return self.items
  end,

  eq = function(self,index)
	local obj = tNode:new()

	if self.items[index] then
	   	obj.items = { self.items[index] }
	else
	  	obj.items = { }
	end
	return obj
  end,

  first = function(self)
	  return self:eq(1)
  end,

  last = function(self)
	  return self:eq(#self.items)
  end,

  val = function(self,expression)
	if expression~=nil then
		if self.items[1] then
			ui.tree_set_value(self.items[1],expression) 
		end
	        return self
	end
	
	if self.items[1] then
  		return ui.tree_get_value(self.items[1])
	end		
	return nil
  end,

  alt = function(self,expression)
	if expression~=nil then
		if self.items[1] then
			ui.tree_set_alt_value(self.items[1],expression) 
	  	end
	  	return self
	end

	if self.items[1] then
	  	return ui.tree_get_alt_value(self.items[1])
	end
	return nil
  end,

  attr = function(self,a_name,a_value)
	if a_value~=nil then 
		if self.items[1] then
			if a_name=="val" then
				ui.tree_set_value(self.items[1],a_value)
			else
				ui.tree_set_attribute(self.items[1],a_name,a_value)
			end
		end
		return self
	end
	if self.items[1] then
		if a_name=="val" then
			return ui.tree_get_value(self.item[1])
		end
		return ui.tree_get_attribute(self.items[1],a_name)
	end
	return nil
  end,

  remove = function(self)
	local pos

	for pos=#self.items,1,-1 do
		ui.tree_delete_node(self.items[pos])
	end
	self.items = {}
	return self
  end,

  append = function(self,node)
	local i,v
	local a_name, a_val
	local obj

	if type(node)~="table" then
		log.print(log.ERROR,"tNode.append() was called with an object that is not a table");
		return nil;
	end

	local obj = tNode:new()
	
  	-- if self.items is empty, will create a node to the root
	if #self.items==0 then
		local added_node = ui.tree_add_node()
		table.insert(obj.items,added_node)
	else
		for i,v in ipairs(self.items) do
			local added_node = ui.tree_add_node(v)
			table.insert(obj.items,added_node)
		end
	end

	for i,v in ipairs(obj.items) do
		for a_name,a_val in pairs(node) do
			if a_name=="val" then
				ui.tree_set_value(v,a_val)
			else
				ui.tree_set_attribute(v,a_name,a_val)
			end
		end
	end

	return obj
  end,

  find = function(self,expression,id)
	local i,v
	local i2,v2
	local pat
	local found_objs
	local retval = tNode:new()
	local found_all = {}

	if type(expression)~="string" then
		log.print(log.ERROR,"tNode.find() must take a string paramater.");
		return nil
	end
	if id then
		expression = expression .. "#" .. id
	end

        for i,v in ipairs(self.items) do

		found_objs = tNode.internal_cache_get(expression,v)

		if found_objs==nil then

                	pat = tNode.internal_pattern(expression)
			found_objs = ui.tree_find_all_nodes(v,pat["label"],pat["id"])
			if #found_objs>0 then
                		tNode.internal_cache_set(expression,v,found_objs)
			end
		end

		-- this avoids duplicating objects in retval
		for i2,v2 in ipairs(found_objs) do
			found_all[tostring(v2)] = v2;
		end

	end

	-- reinsert data in retval without duplicates
	for i,v in pairs(found_all) do
		table.insert(retval.items,v)
	end
	
	return retval 
  end,

  children = function(self)
	local node
	local i,v,j
	local obj = tNode:new()

	for i,v in ipairs(self.items) do 
		node = ui.tree_child_node(v)
		while node do
			table.insert(obj.items,node)
			node = ui.tree_next_node(node)
		end
	end
	return obj
  end,

  length = function(self)
	check_self(self)
	return #self.items
  end,

  toArray = function(self)
	return self.items
  end,

  __tostring = function(self)
	  local res = "tNode = {"
	  self:each(function(i,v) res=res..' ['..i..']="'..tostring(v)..'",' end)
	  return res .. '}';
  end
}

_n = function (expression,root)
	return tNode:new(expression,root)
end

