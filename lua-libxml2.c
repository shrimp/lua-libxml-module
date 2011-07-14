#define DDEBUG 0
#include "ddebug.h"

#include "lua.h"
#include "lauxlib.h"

#include <libxml/xpath.h>
#include <libxml/parser.h>
#include <libxml/xpathInternals.h>
#include <libxml/tree.h>

#include <string.h>


static xmlNodePtr _findNodes(const xmlChar *xpathExpr, xmlXPathContextPtr xpathCtx);


static int lua_loadFile(lua_State *L)
{
    const char* filename = (char *) luaL_checkstring(L, 1);
    xmlInitParser();
    xmlDocPtr doc = xmlParseFile(filename);

    if (doc == NULL) {
        return luaL_error(L, "unable to parse file \"%s\"\n", filename);
    }

    xmlDocPtr docp = (xmlDocPtr)lua_newuserdata(L, sizeof(xmlDoc));
    *docp = *doc;
    dd("docp : %p.\n", docp);
    luaL_getmetatable(L, "xmlDoc");
    lua_setmetatable(L, -2);

    return 1;
}


static int lua_newXPathContext(lua_State *L)
{
    const xmlDocPtr doc = (xmlDocPtr)luaL_checkudata(L, 1, "xmlDoc");
    xmlXPathContextPtr   xpathCtx;
    xpathCtx = xmlXPathNewContext(doc);

    if(xpathCtx == NULL) {
        luaL_error(L, "Error: unable to create new XPath context\n");
        xmlFreeDoc(doc);
        return 2;
    }

    xmlXPathContextPtr xpc = (xmlXPathContextPtr)lua_newuserdata(L, sizeof(xmlXPathContext));
    *xpc = *xpathCtx;
    luaL_getmetatable(L, "xmlXPathContext");
    lua_setmetatable(L, -2);

    return 1;
}


static int lua_nodeName(lua_State *L)
{
    xmlNodePtr xnp  = (xmlNodePtr)luaL_checkudata(L, 1, "xmlNode");
    const char *name = (char *)xnp->name;

    if (name == NULL) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L, name);
    return 1;
}


static int lua_registerNs(lua_State *L)
{
    const xmlXPathContextPtr xpcp  = (xmlXPathContextPtr)luaL_checkudata(L, 1, "xmlXPathContext");
    const xmlChar *ns   = (xmlChar *)luaL_checkstring(L, 2);
    const xmlChar *v    = (xmlChar *)luaL_checkstring(L, 3);

    if (xmlXPathRegisterNs(xpcp, ns, v) !=0) {
        return luaL_error(L, "Error: unable to register Ns with ns=\"%s\" and value=\"%s\"\n", ns, v);
    }

    return 1;
}


static xmlNodePtr _findNodes(const xmlChar *xpathExpr, xmlXPathContextPtr xpathCtx)
{
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);

    if(xpathObj == NULL) {
        xmlXPathFreeContext(xpathCtx);
        dd("unable to evaluate xpath expression \"%s\"\n", xpathExpr);
        return NULL;
    }

    xmlNodeSetPtr nodeset = xpathObj->nodesetval;
    if (nodeset == NULL || xmlXPathNodeSetIsEmpty(nodeset)) {
        dd("there's no result for the xpath expression \"%s\"\n", xpathExpr);
        return NULL;
    }

    return  *nodeset->nodeTab;
}


static int lua_findNodes(lua_State *L)
{
    const xmlXPathContextPtr xpathCtx  = (xmlXPathContextPtr)luaL_checkudata(L, 1, "xmlXPathContext");
    const xmlChar *xpathExpr   = (xmlChar *)luaL_checkstring(L, 2);

    xmlNodePtr np = _findNodes(xpathExpr, xpathCtx);
    if (np == NULL) {
        lua_pushnil(L);
        return 1;
    }

    xmlNodePtr node = (xmlNodePtr)lua_newuserdata(L, sizeof(xmlNode));
    *node = *np;
    luaL_getmetatable(L, "xmlNode");
    lua_setmetatable(L, -2);

    return 1;
}


static int lua_getAttribute(lua_State *L)
{
    xmlNodePtr xnp  = (xmlNodePtr)luaL_checkudata(L, 1, "xmlNode");
    const xmlChar * name     = (xmlChar *)luaL_checkstring(L, 2);
    char * attr     = (char *)xmlGetProp(xnp, name);

    if (attr == NULL) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L, attr);
    return 1;
}


static int lua_childNode(lua_State *L)
{
    xmlNodePtr xnp  = (xmlNodePtr)luaL_checkudata(L, 1, "xmlNode");
    xmlNodePtr cnode = xmlFirstElementChild(xnp);
    if (cnode == NULL) {
        lua_pushnil(L);
        return 1;
    }

    xmlNodePtr child = (xmlNodePtr)lua_newuserdata(L, sizeof(xmlNode));
    *child = *cnode;
    luaL_getmetatable(L, "xmlNode");
    lua_setmetatable(L, -2);
    return 1;
}


static int lua_nextNode(lua_State *L)
{
    xmlNodePtr node = (xmlNodePtr)luaL_checkudata(L, 1, "xmlNode");
    xmlNodePtr sibling = xmlNextElementSibling(node);
    if (sibling == NULL) {
        lua_pushnil(L);
        return 1;
    }

    dd("have next node");
    xmlNodePtr next = (xmlNodePtr)lua_newuserdata(L, sizeof(xmlNode));
    *next = *sibling;
    luaL_getmetatable(L, "xmlNode");
    lua_setmetatable(L, -2);
    return 1;
}


static int lua_getContent(lua_State *L)
{
    xmlNodePtr xnp  = (xmlNodePtr)luaL_checkudata(L, 1, "xmlNode");
    if (xnp->children == NULL) {
        lua_pushnil(L);
        return 1;
    }

    char * content  = (char *)xmlNodeGetContent(xnp->children);
    if (content == NULL) {
        lua_pushnil(L);
        xmlFree(content);
        return 1;
    }

    lua_pushstring(L, content);
    xmlFree(content);
    return 1;
}


static char * _findValue(xmlNodePtr node, xmlChar *name)
{
    xmlNodePtr child     = xmlFirstElementChild(node);
    xmlNodePtr nextnode  = child;

    while(nextnode && strcmp((const char *)nextnode->name, (const char *)name))
        nextnode = xmlNextElementSibling(nextnode);

    if (nextnode && nextnode->children) {
        return (char *)nextnode->children->content;
    }

    return NULL;
}

static int lua_findValue(lua_State *L)
{
    xmlNodePtr node     = (xmlNodePtr) luaL_checkudata(L, 1, "xmlNode");
    xmlChar * nodename  = (xmlChar *) luaL_checkstring(L, 2);
    char * value        = _findValue(node, nodename);
    dd("find node's value is %s\n.", value);

    if (value == NULL) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L, value);
    return 1;
}


static int lua_getNsPrefix(lua_State *L)
{
    xmlNodePtr xnp  = (xmlNodePtr)luaL_checkudata(L, 1, "xmlNode");
    if (xnp->ns == NULL || xnp->ns->prefix == NULL) {
        lua_pushnil(L);
        return 1;
    }

    dd("xmlNode's namespace prefix is %s: \n", xnp->ns->prefix);
    char * ns = (char *)xnp->ns->prefix;

    lua_pushstring(L, ns);
    return 1;
}


static const struct luaL_Reg xpath[] = {
    {"loadFile", lua_loadFile},
    {"newXPathContext", lua_newXPathContext},
    {"registerNs", lua_registerNs},
    {"findNodes", lua_findNodes},
    {"childNode", lua_childNode},
    {"getAttribute", lua_getAttribute},
    {"getContent", lua_getContent},
    {"nodeName", lua_nodeName},
    {"nextNode", lua_nextNode},
    {"getNsPrefix", lua_getNsPrefix},
    {"findValue", lua_findValue},
    {NULL, NULL}
};


int
luaopen_libxml2(lua_State *L)
{
    luaL_register(L, "xpath", xpath);
    luaL_newmetatable(L, "xmlXPathContext");
    luaL_newmetatable(L, "xmlDoc");
    luaL_newmetatable(L, "xmlNode");
    luaL_newmetatable(L, "xmlNodeSet");

    return 1;
}
