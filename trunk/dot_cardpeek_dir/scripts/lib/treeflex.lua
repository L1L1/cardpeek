
if not __original_tree_add_node then
 __original_tree_add_node = ui.tree_add_node
 __original_tree_delete_node = ui.tree_delete_node
end

function ui.tree_add_node(node,classname,label,id,length)
	treeFlex.cache = {}
        return __original_tree_add_node(node,classname,label,id,length);
end

function ui.tree_delete_node(node)
	treeFlex.cache = {}
	return __original_tree_delete_node(node);
end

treeFlex = {

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

  new = function(self,obj,root)
	  local o
	  local m
	  local obj_ref
  
  	  if obj then
	      	if type(obj)=="userdata" then
			o = { ["items"] = { obj } } 
		elseif type(obj)=="table" then
			log.print(log.WARNING,"treeFlex:new(x) or _(x) was called on an object x that is already a treeFlex object.")
			o = { ["items"] = obj["items"] }
		else
			if root then
				obj_ref = tostring(root) .. ">" .. obj
			else
				obj_ref = "@>"..obj
			end
	  	  	if treeFlex.cache[obj_ref] then
	    			o = { ["items"] = treeFlex.cache[obj_ref] }
  			else 
	   			local pat = treeFlex.internal_pattern(obj)
	    			o = { ["items"] = ui.tree_find_all_nodes(nil,pat["label"],pat["id"]) }
				if #o.items==0 then
					log.print(log.WARNING,"treeFlex.new('"..obj.."') returns an empty set.")
				end
				treeFlex.cache[obj_ref] = o.items
	      		end
		end
	  else
	      	o = { ["items"] = {} }
  	  end

          setmetatable(o,self)
	  self.__index = self;
	  return o
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
	local obj = treeFlex:new()

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

  val = function(self)
	if self.items[1] then
  		return ui.tree_get_value(self.items[1])
	end		
	return nil
  end,

  setVal = function(self,value)
	if self.items[1] then
		ui.tree_set_value(self.items[1],value) 
	end
	return self
  end,

  alt = function(self)
	if self.items[1] then
	  	return ui.tree_get_alt_value(self.items[1])
	end
	return nil
  end,

  setAlt = function(self,value)
	  if self.items[1] then
	          ui.tree_set_alt_value(self.items[1],value) 
	  end
	  return self
  end,

  remove = function(self)
	local pos

	for pos=#self.items,1,-1 do
		ui.tree_delete_node(self.items[pos])
	end
	self.items = {}
	return self
  end,

  append = function(self,classname,label,id,length)
	local i,v
	local obj = treeFlex:new()

	if #self.items==0 then
		log.print(log.WARNING,"treeFlex.append() was called on an empty object.")
	end

	for i,v in ipairs(self.items) do
		table.insert(obj.items,ui.tree_add_node(v,classname,label,id,length))
	end
	return obj
  end,

  find = function(self,obj)
	local i,v
	local i2,v2
	local res,pat
	local obj_ref
	local retval = treeFlex:new()

        for i,v in ipairs(self.items) do
		obj_ref = tostring(v)..">"..obj
		if treeFlex.cache[obj_ref] then
			res = treeFlex.cache[obj_ref]
		else
                	pat = treeFlex.internal_pattern(obj)
			res = ui.tree_find_all_nodes(v,pat["label"],pat["id"])
                	treeFlex.cache[obj_ref] = res
		end
		if res then
			for i2,v2 in ipairs(res) do
				table.insert(retval.items,v2)
			end
		end
	end
  	return retval -- FIXME: do we need to normalize
  end,

  children = function(self)
	local info
	local i,v,j
	local obj = treeFlex:new()

	for i,v in ipairs(self.items) do 
		info = ui.tree_get_node(v)  
	  	for index=0,info.num_children-1 do
			table.insert(obj.items,v..":"..index)
		end
	end
	return obj
  end,

  size = function(self)
	return #self.items
  end,

  exists = function(self)
	return #self.items>0
  end,

  __tostring = function(self)
	  local res = "treeFlex = {"
	  self:each(function(i,v) res=res..' ['..i..']="'..tostring(v)..'",' end)
	  return res .. '}';
  end
}

_ = function (describe,root)
	return treeFlex:new(describe,root)
end

