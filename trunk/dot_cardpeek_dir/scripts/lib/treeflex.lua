
if not __original_tree_add_node then
 __original_tree_add_node = ui.tree_add_node
 __original_tree_delete_node = ui.tree_delete_node
end

function ui.tree_add_node(node,classname,description,id,length)
	treeFlex.cache = {}
        return __original_tree_add_node(node,classname,description,id,length);
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
			ret["description"] = m[1]
			ret["id"] = m[2]
		else
			ret["description"] = obj
		end
	end
	return ret
  end,

  internal_sort = function(self)
	table.sort(self.items)
	return self
  end,

  internal_normalize = function(self)
	local new_items = {}
	local prev_v = ""
	table.sort(self.items)
	for i,v in ipairs(self.items) do
		if v ~= prev_v then
			table.insert(new_items,v)
			prev_v = v
		end
	end
	self.items = new_items
	return self
  end,

  new = function(self,obj,root)
	  local o
	  local m
	  local obj_ref
  
  	  if obj then
	      	if string.match(obj,"^@[0123456789:]+") then
				o = { ["items"] = { obj } } 
		else
			if root then
				obj_ref = root .. ">" .. obj
			else
				obj_ref = "@>"..obj
			end
	  	  	if treeFlex.cache[obj_ref] then
	    			o = { ["items"] = treeFlex.cache[obj_ref] }
  			else 
	   			local pat = treeFlex.internal_pattern(obj)
	    			o = { ["items"] = ui.tree_find_all_nodes(nil,pat["description"],pat["id"]) }
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

	self:internal_sort()
	for pos=#self.items,1,-1 do
		ui.tree_delete_node(self.items[pos])
	end
	self.items = {}
	return self
  end,

  append = function(self,classname,description,id,length)
	local i,v
	local obj = treeFlex:new()
	
	for i,v in ipairs(self.items) do
		table.insert(obj.items,ui.tree_add_node(v,classname,description,id,length))
	end
	return obj
  end,

  find = function(self,obj)
	local i,v
	local i2,v2
	local res,pat
	local obj_ref
	local obj = treeFlex:new()

        for i,v in ipairs(self.items) do
		obj_ref = v..">"..obj
		if treeFlex.cache[obj_ref] then
			res = treeFlex.cache[obj_ref]
		else
                	pat = treeFlex.internal_pattern(obj)
			res = ui.tree_find_all_nodes(v,pat["description"],pat["id"])
                	treeFlex.cache[obj_ref] = res
		end
		if res then
			for i2,v2 in ipairs(res) do
				table.insert(obj.items,v2)
			end
		end
	end
  	return obj:internal_normalize()
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
	  self:each(function(i,v) res=res..' ['..i..']="'..v..'",' end)
	  return res .. '}';
  end
}

_ = function (describe,root)
	return treeFlex:new(describe,root)
end

