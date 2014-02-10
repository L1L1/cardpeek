
local DEPRECATED_FUNCS = {}

function DEPRECATED_FOR(fname)
    local func = debug.getinfo(2, "n")
    local orig = debug.getinfo(3, "lS")

    if DEPRECATED_FUNCS[func.name]==nil then
        log.print(log.WARNING,orig.short_src .. "[" .. orig.currentline .. "]: " .. func.name .. "() is deprecated, please use " .. fname .. "() instead.\n" ..
                  "\tThis warning will only be printed once for " .. func.name .. "().")
        DEPRECATED_FUNCS[func.name]=true
    end
end

function ui.tree_find_all_nodes(origin_ref, alabel, aid)
    DEPRECATED_FOR("nodes.find")

    local node
    local ret = {}

    if origin_ref==nil then
        origin_ref = nodes.root()
    end
    for node in nodes.find(origin_ref, {label=alabel, id=aid}) do
        ret[#ret+1]=node;
    end

    return ret;
end

function ui.tree_find_node(origin_ref, alabel, aid)
    DEPRECATED_FOR("nodes.find_first")
    
    if origin_ref==nil then
        origin_ref = nodes.root()
    end
    return nodes.find_first(origin_ref, {label=alabel, id=aid})
end

function ui.tree_add_node(parent_ref, ...)
    DEPRECATED_FOR("nodes.append")
    
    local arg={...}
    if parent_ref==nil then
        parent_ref = nodes.root()
    end
    return nodes.append(parent_ref,{ classname=arg[1], label=arg[2], id=arg[3], size=arg[4] })
end

function ui.tree_set_value(node,val)
    DEPRECATED_FOR("nodes.set_attribute")
    
    return node:set_attribute("val",val)
end

function ui.tree_get_value(node,val)
    DEPRECATED_FOR("nodes.get_attribute")
    
    return node:get_attribute("val",val)
end

function ui.tree_set_alt_value(node,val)
    DEPRECATED_FOR("nodes.set_attribute")
    
    return node:set_attribute("alt",val)
end

function ui.tree_get_alt_value(node,val)
    DEPRECATED_FOR("nodes.get_attribute")
    
    return node:get_attribute("alt",val)
end

function ui.tree_delete_node(node)
    DEPRECATED_FOR("nodes.remove")
    
    return nodes.remove(node)
end
