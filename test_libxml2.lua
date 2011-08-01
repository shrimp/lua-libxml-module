require("libxml2")

local doc = xpath.loadFile("test.xml")
local xpc = xpath.newXPathContext(doc)
local xpathObj = xpath.xmlXPathEvalExpression(xpc, "/log/books")
local node = xpath.findNodes(xpathObj)

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

xpath.freeXPathObject(xpathObj)
xpath.freeXPathContext(xpc)
xpath.freeDoc(doc)
xpath.xmlCleanupParser()
