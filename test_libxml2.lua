require("libxml2")

local doc = xpath.loadFile("test.xml")
local xpc = xpath.newXPathContext(doc)
local node = xpath.findNodes(xpc, "/log/books")

while (node) do
    local children = xpath.childNode(node)
    while (children) do
        nodename = xpath.nodeName(children)
        local name  = xpath.getAttribute(children, "name")
        local value = xpath.getAttribute(children, "value")
        if name and value then
            print(nodename .. "  name = " .. name .. ", value = " .. value)
        end
        children = xpath.nextNode(children)
    end
    node = xpath.nextNode(node)
end
