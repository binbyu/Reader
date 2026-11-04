#pragma once
#include <string>
#include <vector>
#include <functional>
#include "common.h"
#include "cJSON.h"
#include "libxml/HTMLparser.h"
#include "libxml/xpath.h"
#include "libxml/HTMLtree.h"

struct ParserContext
{
    cJSON* json;
    xmlDocPtr doc;
    xmlXPathContextPtr ctx;
};

using NodeCallback = std::function<void(const xmlNode *node)>;

class HtmlParser
{
private:
    HtmlParser();
    ~HtmlParser();

public:
    static HtmlParser& instance();

    // html
    bool parseHtml(const char* html, int len, const std::string& xpath, std::vector<std::string>& value, bool formatText = false);
    bool parseHtmlBegin(const char* html, int len, ParserContext& ctx);
    bool parseHtmlNext(ParserContext& ctx, const std::string& xpath, std::vector<std::string>& value, bool formatText = false);
    bool parseHtmlNext(ParserContext& ctx, const std::string& xpath, NodeCallback cb);
    bool parseHtmlEnd(ParserContext& ctx);

    // xml
    bool parseXml(const char* xml, int len, const std::string& xpath, std::vector<std::string>& value);
    bool parseXmlBegin(const char* xml, int len, ParserContext& ctx);
    bool parseXmlNext(ParserContext& ctx, const std::string& xpath, std::vector<std::string>& value);
    bool parseXmlNext(ParserContext& ctx, const std::string& xpath, NodeCallback cb);
    bool parseXmlEnd(ParserContext& ctx);

private:
    bool parseHtmlByXPath(xmlDocPtr doc, xmlXPathContextPtr ctx, const std::string& xpath, std::vector<std::string>& value, bool formatText = false);
    bool parseHtmlByXPath(xmlDocPtr doc, xmlXPathContextPtr ctx, const std::string& xpath, NodeCallback cb);
    bool parseJsonByXPath(cJSON* json, const std::string& xpath, std::vector<std::string>& value);
    bool parseJsonByJsonPath(cJSON* json, const std::string& jpath, std::vector<std::string>& value);
    void extractStringValues(cJSON* json, std::vector<std::string>& value);
    void extractNodeText(const xmlNode *node, std::string &out);

    bool parseConcat(const std::string& expression, std::string& nodeXPath, std::string& prefix, std::string& suffix);
    void applyConcat(std::vector<std::string>& values, const std::string& prefix, const std::string& suffix);

#if ENABLE_TESTCASE
private:
    void testcase1_parseJsonByXPath();
    void testcase2_parseJsonByXPath();
    void testcase1_parseJsonByJsonPath();
    void testcase2_parseJsonByJsonPath();
#endif
};
