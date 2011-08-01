#define DDEBUG 0
#include "ddebug.h"

#include "lua.h"
#include "lauxlib.h"

#include <libxml/xpath.h>
#include <libxml/parser.h>
#include <libxml/xpathInternals.h>
#include <libxml/tree.h>

#include <string.h>


xmlNodePtr xmlFirstElementChild(xmlNodePtr parent);
xmlNodePtr xmlNextElementSibling(xmlNodePtr node);

xmlNodePtr
xmlNextElementSibling(xmlNodePtr node) {
    if (node == NULL)
        return(NULL);
    switch (node->type) {
        case XML_ELEMENT_NODE:
        case XML_TEXT_NODE:
        case XML_CDATA_SECTION_NODE:
        case XML_ENTITY_REF_NODE:
        case XML_ENTITY_NODE:
        case XML_PI_NODE:
        case XML_COMMENT_NODE:
        case XML_DTD_NODE:
        case XML_XINCLUDE_START:
        case XML_XINCLUDE_END:
            node = node->next;
            break;
        default:
            return(NULL);
    }
    while (node != NULL) {
        if (node->type == XML_ELEMENT_NODE)
            return(node);
        node = node->next;
    }
    return(NULL);
}

xmlNodePtr
xmlFirstElementChild(xmlNodePtr parent) {
    xmlNodePtr cur = NULL;

    if (parent == NULL)
        return(NULL);
    switch (parent->type) {
        case XML_ELEMENT_NODE:
        case XML_ENTITY_NODE:
        case XML_DOCUMENT_NODE:
        case XML_HTML_DOCUMENT_NODE:
            cur = parent->children;
            break;
        default:
            return(NULL);
    }
    while (cur != NULL) {
        if (cur->type == XML_ELEMENT_NODE)
            return(cur);
        cur = cur->next;
    }
    return(NULL);
}


static int lua_xmlParseDoc(lua_State *L)
{
    const char* string = (char *)luaL_checkstring(L, 1);

    xmlInitParser();
    xmlDocPtr doc = xmlParseDoc((const xmlChar *)string);

    if (doc == NULL) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushlightuserdata(L, doc);

    return 1;
}


static int lua_loadFile(lua_State *L)
{
    const char* filename = (char *) luaL_checkstring(L, 1);
    xmlDocPtr doc = xmlParseFile(filename);

    if (doc == NULL) {
        lua_pushnil(L);
        //return luaL_error(L, "unable to parse file \"%s\"\n", filename);
        return 1;
    }

    lua_pushlightuserdata(L, doc);

    return 1;
}


static int lua_initParser(lua_State *L)
{
    xmlInitParser();
    return 1;
}

static int lua_newXPathContext(lua_State *L)
{
    const xmlDocPtr doc = (xmlDocPtr)lua_touserdata(L, 1);
    xmlXPathContextPtr   xpathCtx;
    xpathCtx = xmlXPathNewContext(doc);

    if(xpathCtx == NULL) {
        xmlFreeDoc(doc);
        lua_pushnil(L);
        return 1;
    }

    lua_pushlightuserdata(L, xpathCtx);

    return 1;
}


static int lua_nodeName(lua_State *L)
{
    xmlNodePtr xnp  = (xmlNodePtr)lua_touserdata(L, 1);
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
    const xmlXPathContextPtr xpcp  = (xmlXPathContextPtr)lua_touserdata(L, 1);
    const xmlChar *ns   = (xmlChar *)luaL_checkstring(L, 2);
    const xmlChar *v    = (xmlChar *)luaL_checkstring(L, 3);

    if (xmlXPathRegisterNs(xpcp, ns, v) !=0) {
        return luaL_error(L, "Error: unable to register Ns with ns=\"%s\" and value=\"%s\"\n", ns, v);
    }

    return 1;
}


static int lua_xmlXPathEvalExpression(lua_State *L)
{
    const xmlXPathContextPtr xpathCtx  = (xmlXPathContextPtr)lua_touserdata(L, 1);
    const xmlChar *xpathExpr   = (xmlChar *)luaL_checkstring(L, 2);
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);

    if(xpathObj == NULL) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushlightuserdata(L, xpathObj);

    return 1;

}


static int lua_findNodes(lua_State *L)
{
    const xmlXPathObjectPtr xpathObj  = (xmlXPathObjectPtr)lua_touserdata(L, 1);

    xmlNodeSetPtr nodeset = xpathObj->nodesetval;
    if (nodeset == NULL || xmlXPathNodeSetIsEmpty(nodeset)) {
        dd("there's no result for the xpath expression \"%s\"\n", xpathExpr);
        lua_pushnil(L);
        return 1;
    }

    xmlNodePtr np =  *nodeset->nodeTab;

    if (np == NULL) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushlightuserdata(L, np);

    return 1;
}


static int lua_getAttribute(lua_State *L)
{
    xmlNodePtr xnp  = (xmlNodePtr)lua_touserdata(L, 1);
    const xmlChar * name     = (xmlChar *)luaL_checkstring(L, 2);
    char * attr     = (char *)xmlGetProp(xnp, name);

    if (attr == NULL) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushstring(L, attr);
    free(attr);
    return 1;
}


static int lua_childNode(lua_State *L)
{
    xmlNodePtr xnp  = (xmlNodePtr)lua_touserdata(L, 1);
    xmlNodePtr cnode = xmlFirstElementChild(xnp);
    if (cnode == NULL) {
        lua_pushnil(L);
        return 1;
    }

    lua_pushlightuserdata(L, cnode);
    return 1;
}


static int lua_nextNode(lua_State *L)
{
    xmlNodePtr node = (xmlNodePtr)lua_touserdata(L, 1);
    xmlNodePtr sibling = xmlNextElementSibling(node);
    if (sibling == NULL) {
        lua_pushnil(L);
        return 1;
    }

    dd("have next node");
    lua_pushlightuserdata(L, sibling);
    return 1;
}


static int lua_getContent(lua_State *L)
{
    xmlNodePtr xnp  = (xmlNodePtr)lua_touserdata(L, 1);
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
    free(content);
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
    xmlNodePtr node     = (xmlNodePtr) lua_touserdata(L, 1);
    xmlChar * nodename  = (xmlChar *) luaL_checkstring(L, 2);
    const char * value        = _findValue(node, nodename);
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
    xmlNodePtr xnp  = (xmlNodePtr)lua_touserdata(L, 1);
    if (xnp->ns == NULL || xnp->ns->prefix == NULL) {
        lua_pushnil(L);
        return 1;
    }

    dd("xmlNode's namespace prefix is %s: \n", xnp->ns->prefix);
    char * ns = (char *)xnp->ns->prefix;

    lua_pushstring(L, ns);
    return 1;
}


static int lua_freeDoc(lua_State *L)
{
    const xmlDocPtr doc = (xmlDocPtr)lua_touserdata(L, 1);
    if (doc == NULL) {
        return 1;
    }

    xmlFreeDoc(doc);
    return 0;
}


static int lua_freeNode(lua_State *L)
{
    const xmlNodePtr xnp  = (xmlNodePtr)lua_touserdata(L, 1);
    if (xnp == NULL) {
        return 1;
    }

    xmlFreeNode(xnp);
    return 0;
}

static int lua_freeXPathContext(lua_State *L)
{
    const xmlXPathContextPtr xpathCtx  = (xmlXPathContextPtr)lua_touserdata(L, 1);
    if (xpathCtx == NULL) {
        return 1;
    }

    xmlXPathFreeContext(xpathCtx);
    return 0;
}

static int lua_freeXPathObject(lua_State *L)
{
    const xmlXPathObjectPtr xpathObj  = (xmlXPathObjectPtr)lua_touserdata(L, 1);
    if (xpathObj == NULL) {
        return 1;
    }

    xmlXPathFreeObject(xpathObj);
    return 0;
}

static int lua_xmlCleanupParser(lua_State *L)
{
    xmlCleanupParser();
    return 0;
}

static const struct luaL_Reg xpath[] = {
    {"xmlCleanupParser", lua_xmlCleanupParser},
    {"freeDoc", lua_freeDoc},
    {"freeNode", lua_freeNode},
    {"freeXPathContext", lua_freeXPathContext},
    {"freeXPathObject", lua_freeXPathObject},
    {"loadFile", lua_loadFile},
    {"xmlParseDoc", lua_xmlParseDoc},
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
    {"initParser", lua_initParser},
    {"xmlXPathEvalExpression", lua_xmlXPathEvalExpression},
    {NULL, NULL}
};


int
luaopen_libxml2(lua_State *L)
{
    luaL_register(L, "xpath", xpath);

    return 1;
}
