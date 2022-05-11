#include "framework.h"
#include "HtmlParser.h"
#include "libxml/HTMLparser.h"
#include "libxml/xpath.h"
#include "libxml/HTMLtree.h"


HtmlParser::HtmlParser()
{
    xmlInitParser();
}


HtmlParser::~HtmlParser()
{
    xmlCleanupParser();
}

HtmlParser* HtmlParser::Instance()
{
    static HtmlParser* s_HtmlParser = NULL;
    if (!s_HtmlParser)
        s_HtmlParser = new HtmlParser;
    return s_HtmlParser;
}

void HtmlParser::ReleaseInstance()
{
    if (Instance())
        delete Instance();
}

char * HtmlParser::CreateContent(const char* xml)
{
    char *content = NULL;
    const char *txml;
    char *tcnt;

    content = (char*)malloc(strlen(xml) + 1);
    if (!content)
        return NULL;

    txml = xml;
    tcnt = content;
    while (*txml)
    {
        switch (*txml)
        {
        case ' ':
        case '\r':
        case '\t':
        case '\n':
            break;
        default:
            *tcnt = *txml;
            tcnt++;
            break;
        }
        txml++;
    }
    *tcnt = 0;

    return content;
}

void HtmlParser::ReleaseContent(char *content)
{
    if (content)
        free(content);
}

#define GOTO_STOP(s) if (*(s)) goto _stop

int HtmlParser::HtmlParseByXpath(const char* html, int len, const std::string& xpath, std::vector<std::string>& value, BOOL* stop, BOOL clear)
{
    int i;
    xmlDocPtr doc = NULL;
    xmlXPathContextPtr xpathCtx = NULL;
    xmlXPathObjectPtr xpathObj = NULL;
    xmlNodeSetPtr nodeset = NULL;
    xmlChar* keyword = NULL;
    char* content = NULL;

    GOTO_STOP(stop);

    doc = htmlReadMemory(html, len, NULL, NULL, HTML_PARSE_RECOVER);
    if (doc == NULL)
    {
        return 1;
    }
    
    GOTO_STOP(stop);

    xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx == NULL)
    {
        xmlFreeDoc(doc);
        return 1;
    }

    GOTO_STOP(stop);

    xpathObj = xmlXPathEvalExpression(BAD_CAST xpath.c_str(), xpathCtx);
    xmlXPathFreeContext(xpathCtx);
    xpathCtx = NULL;
    if (xpathObj == NULL)
    {
        xmlFreeDoc(doc);
        return 1;
    }

    GOTO_STOP(stop);

    if (xmlXPathNodeSetIsEmpty(xpathObj->nodesetval))
    {
        xmlXPathFreeObject(xpathObj);
        // No result
        return 0;
    }

    nodeset = xpathObj->nodesetval;
    for (i = 0; i < nodeset->nodeNr; i++)
    {
        GOTO_STOP(stop);
        keyword = xmlNodeGetContent(nodeset->nodeTab[i]);

        if (clear)
        {
            if (keyword)
            {
                content = CreateContent((const char*)keyword);
                value.push_back(content);
                ReleaseContent(content);
            }
        }
        else
        {
            if (keyword)
                value.push_back((const char*)keyword);
        }
        if (keyword)
            xmlFree(keyword);
    }
    xmlXPathFreeObject(xpathObj);

    xmlFreeDoc(doc);
    return 0;

_stop:
    if (xpathObj)
        xmlXPathFreeObject(xpathObj);
    if (xpathCtx)
        xmlXPathFreeContext(xpathCtx);
    if (doc)
        xmlFreeDoc(doc);
    return 1;
}

int HtmlParser::HtmlParseBegin(const char *html, int len, void** pdoc, void** pctx, BOOL* stop)
{
    xmlDocPtr doc = NULL;
    xmlXPathContextPtr xpathCtx = NULL;

    *pdoc = NULL;
    *pctx = NULL;
    GOTO_STOP(stop);
    doc = htmlReadMemory(html, len, NULL, NULL, HTML_PARSE_RECOVER);
    if (doc == NULL)
    {
        return 1;
    }

    GOTO_STOP(stop);

    xpathCtx = xmlXPathNewContext(doc);
    if (xpathCtx == NULL)
    {
        xmlFreeDoc(doc);
        return 1;
    }

    *pdoc = doc;
    *pctx = xpathCtx;
    return 0;

_stop:
    if (xpathCtx)
        xmlXPathFreeContext(xpathCtx);
    if (doc)
        xmlFreeDoc(doc);
    return 1;
}

int HtmlParser::HtmlParseByXpath(void* doc_, void* ctx_, const std::string& xpath, std::vector<std::string>& value, BOOL* stop, BOOL clear)
{
    int i;
    xmlDocPtr doc = (xmlDocPtr)doc_;
    xmlXPathContextPtr xpathCtx = (xmlXPathContextPtr)ctx_;
    xmlXPathObjectPtr xpathObj = NULL;
    xmlNodeSetPtr nodeset = NULL;
    xmlChar* keyword = NULL;
    char* content = NULL;

    if (!doc || !xpathCtx)
        return 1;

    GOTO_STOP(stop);

    xpathObj = xmlXPathEvalExpression(BAD_CAST xpath.c_str(), xpathCtx);
    if (xpathObj == NULL)
    {
        return 1;
    }

    if (xmlXPathNodeSetIsEmpty(xpathObj->nodesetval))
    {
        xmlXPathFreeObject(xpathObj);
        // No result
        return 0;
    }

    nodeset = xpathObj->nodesetval;
    for (i = 0; i < nodeset->nodeNr; i++)
    {
        GOTO_STOP(stop);
        keyword = xmlNodeGetContent(nodeset->nodeTab[i]);
        if (clear)
        {
            if (keyword)
            {
                content = CreateContent((const char*)keyword);
                value.push_back(content);
                ReleaseContent(content);
            }
        }
        else
        {
            if (keyword)
                value.push_back((const char*)keyword);
        }
        if (keyword)
            xmlFree(keyword);
    }
    xmlXPathFreeObject(xpathObj);
    return 0;

_stop:
    if (xpathObj)
        xmlXPathFreeObject(xpathObj);
    return 1;
}

int HtmlParser::HtmlParseEnd(void* doc_, void* ctx_)
{
    xmlDocPtr doc = (xmlDocPtr)doc_;
    xmlXPathContextPtr xpathCtx = (xmlXPathContextPtr)ctx_;

    if (xpathCtx)
        xmlXPathFreeContext(xpathCtx);
    if (doc)
        xmlFreeDoc(doc);
    return 0;
}

int HtmlParser::FormatHtml(char *html, int len, char **htmlfmt, int *fmtlen)
{
    xmlDocPtr doc = NULL;
    xmlChar *format_str = NULL;
    int size = 0;

    xmlKeepBlanksDefault(0);
    xmlIndentTreeOutput = 0;
    doc = htmlReadMemory(html, len, NULL, NULL, HTML_PARSE_RECOVER | HTML_PARSE_NOBLANKS);

    if (doc)
    {
        htmlDocDumpMemoryFormat(doc, &format_str, &size, 1);
        xmlFreeDoc(doc);
        if (!format_str || size <= 0)
        {
            if (format_str)
                xmlFree(format_str);
        }
    }

    xmlKeepBlanksDefault(1);
    xmlIndentTreeOutput = 0;

    *htmlfmt = (char *)format_str;
    *fmtlen = size;
    return 0;
}

void HtmlParser::FreeFormat(char *htmlfmt)
{
    xmlFree(htmlfmt);
}
