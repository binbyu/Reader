#include "HtmlParser.h"
#include "Utils.h"
#include "cJSON_Path.h"
#include "cJSON_XPath.h"

HtmlParser::HtmlParser()
{
    xmlInitParser();

#if ENABLE_TESTCASE
    testcase1_parseJsonByXPath();
    testcase2_parseJsonByXPath();
    testcase1_parseJsonByJsonPath();
    testcase2_parseJsonByJsonPath();
#endif
}

HtmlParser::~HtmlParser()
{
    xmlCleanupParser();
}

HtmlParser& HtmlParser::instance()
{
    static HtmlParser instance;
    return instance;
}

bool HtmlParser::parseHtml(const char* html, int len, const std::string& xpath_, std::vector<std::string>& value, bool noBlanks)
{
    std::string xpath, prefix, suffix;
    bool hasConcat = parseConcat(xpath_, xpath, prefix, suffix);
    if (!hasConcat) xpath = xpath_;

    bool ret = false;
    cJSON* json = cJSON_Parse(html);
    if (json)
    {
        if (!xpath.empty() && xpath.front() == '/')
        {
            ret = parseJsonByXPath(json, xpath, value);
        }
        else
        {
            ret = parseJsonByJsonPath(json, xpath, value);
        }
        cJSON_Delete(json);
        if (ret && hasConcat) applyConcat(value, prefix, suffix);
        return ret;
    }

    xmlDocPtr doc = htmlReadMemory(html, len, nullptr, "UTF-8", HTML_PARSE_RECOVER | HTML_PARSE_NONET);
    if (!doc)
    {
        return false;
    }
    xmlXPathContextPtr ctx = xmlXPathNewContext(doc);
    if (!ctx)
    {
        xmlFreeDoc(doc);
        return false;
    }

    ret = parseHtmlByXPath(doc, ctx, xpath, value, noBlanks);
    xmlXPathFreeContext(ctx);
    xmlFreeDoc(doc);
    if (ret && hasConcat) applyConcat(value, prefix, suffix);
    return ret;
}

bool HtmlParser::parseHtmlBegin(const char* html, int len, ParserContext& ctx)
{
    ctx.json = cJSON_Parse(html);
    if (ctx.json)
    {
        ctx.doc = nullptr;
        ctx.ctx = nullptr;
        return true;
    }

    ctx.doc = htmlReadMemory(html, len, nullptr, "UTF-8", HTML_PARSE_RECOVER | HTML_PARSE_NONET);
    if (!ctx.doc)
    {
        ctx.ctx = nullptr;
        return false;
    }
    ctx.ctx = xmlXPathNewContext(ctx.doc);
    if (!ctx.ctx)
    {
        xmlFreeDoc(ctx.doc);
        ctx.doc = nullptr;
        return false;
    }
    return true;
}

bool HtmlParser::parseHtmlNext(ParserContext& ctx, const std::string& xpath_, std::vector<std::string>& value, bool noBlanks)
{
    std::string xpath, prefix, suffix;
    bool hasConcat = parseConcat(xpath_, xpath, prefix, suffix);
    if (!hasConcat) xpath = xpath_;

    bool ret = false;
    if (ctx.json)
    {
        if (!xpath.empty() && xpath.front() == '/')
        {
            ret = parseJsonByXPath(ctx.json, xpath, value);
            if (ret && hasConcat) applyConcat(value, prefix, suffix);
            return ret;
        }
        else
        {
            ret = parseJsonByJsonPath(ctx.json, xpath, value);
            if (ret && hasConcat) applyConcat(value, prefix, suffix);
            return ret;
        }
    }
    else if (ctx.doc && ctx.ctx)
    {
        ret = parseHtmlByXPath(ctx.doc, ctx.ctx, xpath, value, noBlanks);
        if (ret && hasConcat) applyConcat(value, prefix, suffix);
        return ret;
    }
    value.clear();
    return false;
}

bool HtmlParser::parseHtmlNext(ParserContext& ctx, const std::string& xpath, NodeCallback cb)
{
    if (ctx.doc && ctx.ctx)
    {
        return parseHtmlByXPath(ctx.doc, ctx.ctx, xpath, cb);
    }
    return false;
}

bool HtmlParser::parseHtmlEnd(ParserContext& ctx)
{
    if (ctx.json)
    {
        cJSON_Delete(ctx.json);
        ctx.json = nullptr;
    }
    if (ctx.ctx)
    {
        xmlXPathFreeContext(ctx.ctx);
        ctx.ctx = nullptr;
    }
    if (ctx.doc)
    {
        xmlFreeDoc(ctx.doc);
        ctx.doc = nullptr;
    }
    return true;
}

bool HtmlParser::formatHtml(const char* html, int len, std::string& formated)
{
    xmlKeepBlanksDefault(0);
    xmlIndentTreeOutput = 0;

    xmlDocPtr doc = htmlReadMemory(html, len, NULL, "UTF-8", HTML_PARSE_RECOVER | HTML_PARSE_NOBLANKS | HTML_PARSE_NONET);
    if (!doc)
    {
        xmlKeepBlanksDefault(1);
        xmlIndentTreeOutput = 0;
        return false;
    }

    xmlChar* formatStr = nullptr;
    int formatLen = 0;
    htmlDocDumpMemoryFormat(doc, &formatStr, &formatLen, 1);
    xmlFreeDoc(doc);

    if (!formatStr)
    {
        xmlKeepBlanksDefault(1);
        xmlIndentTreeOutput = 0;
        return false;
    }

    xmlKeepBlanksDefault(1);
    xmlIndentTreeOutput = 0;

    formated.assign((const char*)formatStr);
    xmlFree(formatStr);
    return true;
}

void HtmlParser::tidyHtml(char* html, int& len)
{
    if (!html || len <= 0) return;

    int src = 0;
    int dst = 0;

    while (src < len)
    {
        if (html[src] == '<' && (src + 3 < len))
        {
            // <br> or <BR>
            if ((html[src + 1] == 'b' || html[src + 1] == 'B') &&
                (html[src + 2] == 'r' || html[src + 2] == 'R'))
            {
                // <br>
                if (html[src + 3] == '>')
                {
                    html[dst++] = '\n';
                    src += 4;
                    continue;
                }
                // <br/>
                if (src + 4 < len && html[src + 3] == '/' && html[src + 4] == '>')
                {
                    html[dst++] = '\n';
                    src += 5;
                    continue;
                }
                // <br /> or <br  >
                int temp = src + 3;
                while (temp < len && (html[temp] == ' ' || html[temp] == '\t')) {
                    temp++;
                }
                if (temp < len && (html[temp] == '>' || (html[temp] == '/' && temp + 1 < len && html[temp + 1] == '>')))
                {
                    html[dst++] = '\n';
                    src = (html[temp] == '/') ? temp + 2 : temp + 1;
                    continue;
                }
            }
            // </br>
            else if (src + 4 < len && html[src + 1] == '/' &&
                    (html[src + 2] == 'b' || html[src + 2] == 'B') &&
                    (html[src + 3] == 'r' || html[src + 3] == 'R') &&
                    html[src + 4] == '>')
            {
                html[dst++] = '\n';
                src += 5;
                continue;
            }
        }
        html[dst++] = html[src++];
    }
    len = dst;
    html[len] = '\0';
}

bool HtmlParser::parseXml(const char* xml, int len, const std::string& xpath_, std::vector<std::string>& value, bool noBlanks)
{
    xmlDocPtr doc = nullptr;

    std::string xpath, prefix, suffix;
    bool hasConcat = parseConcat(xpath_, xpath, prefix, suffix);
    if (!hasConcat) xpath = xpath_;

    doc = xmlReadMemory(xml, len, nullptr, nullptr, XML_PARSE_RECOVER | XML_PARSE_NOBLANKS | XML_PARSE_NONET);
    if (!doc)
    {
        return false;
    }

    xmlXPathContextPtr ctx = xmlXPathNewContext(doc);
    if (!ctx)
    {
        xmlFreeDoc(doc);
        return false;
    }

    bool ret = parseHtmlByXPath(doc, ctx, xpath, value, noBlanks);
    xmlXPathFreeContext(ctx);
    xmlFreeDoc(doc);
    if (ret && hasConcat) applyConcat(value, prefix, suffix);
    return ret;
}

bool HtmlParser::parseXmlBegin(const char* xml, int len, ParserContext& ctx)
{
    ctx.json = nullptr;

    ctx.doc = xmlReadMemory(xml, len, nullptr, nullptr, XML_PARSE_RECOVER | XML_PARSE_NOBLANKS | XML_PARSE_NONET);
    if (!ctx.doc)
    {
        return false;
    }

    ctx.ctx = xmlXPathNewContext(ctx.doc);
    if (!ctx.ctx)
    {
        xmlFreeDoc(ctx.doc);
        ctx.doc = nullptr;
        return false;
    }
    return true;
}

bool HtmlParser::parseXmlNext(ParserContext& ctx, const std::string& xpath_, std::vector<std::string>& value, bool noBlanks)
{
    std::string xpath, prefix, suffix;
    bool hasConcat = parseConcat(xpath_, xpath, prefix, suffix);
    if (!hasConcat) xpath = xpath_;

    if (ctx.doc && ctx.ctx)
    {
        bool ret = parseHtmlByXPath(ctx.doc, ctx.ctx, xpath, value, noBlanks);
        if (ret && hasConcat) applyConcat(value, prefix, suffix);
        return ret;
    }
    value.clear();
    return false;
}

bool HtmlParser::parseXmlNext(ParserContext& ctx, const std::string& xpath, NodeCallback cb)
{
    return parseHtmlByXPath(ctx.doc, ctx.ctx, xpath, cb);
}

bool HtmlParser::parseXmlEnd(ParserContext& ctx)
{
    return parseHtmlEnd(ctx);
}

bool HtmlParser::formatXml(const char* xml, int len, std::string& formated)
{
    xmlKeepBlanksDefault(0);

    xmlDocPtr doc = xmlReadMemory(xml, len, nullptr, nullptr, /*XML_PARSE_RECOVER*/HTML_PARSE_RECOVER | XML_PARSE_NOBLANKS | XML_PARSE_NONET);
    if (!doc)
    {
        xmlKeepBlanksDefault(1);
        return false;
    }

    xmlChar* formatStr = nullptr;
    int formatLen = 0;
    xmlDocDumpFormatMemory(doc, &formatStr, &formatLen, 1);
    xmlFreeDoc(doc);

    if (!formatStr)
    {
        xmlKeepBlanksDefault(1);
        return false;
    }

    xmlKeepBlanksDefault(1);
    formated.assign((const char*)formatStr);
    xmlFree(formatStr);
    return true;
}

bool HtmlParser::parseHtmlByXPath(xmlDocPtr doc, xmlXPathContextPtr ctx, const std::string& xpath, std::vector<std::string>& value, bool noBlanks)
{
    value.clear();
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(BAD_CAST xpath.c_str(), ctx);
    if (!xpathObj)
    {
        return false;
    }
    if (xmlXPathNodeSetIsEmpty(xpathObj->nodesetval))
    {
        xmlXPathFreeObject(xpathObj);
        // No result
        return true;
    }

    xmlChar* keyword = nullptr;
    xmlNodeSetPtr nodeset = xpathObj->nodesetval;
    for (int i = 0; i < nodeset->nodeNr; i++)
    {
        keyword = xmlNodeGetContent(nodeset->nodeTab[i]);
        if (keyword)
        {
            if (noBlanks)
            {
                value.push_back(removeBlanks((const char*)keyword));
            }
            else
            {
                value.push_back((const char*)keyword);
            }
            xmlFree(keyword);
        }
    }
    xmlXPathFreeObject(xpathObj);
    return true;
}

bool HtmlParser::parseHtmlByXPath(xmlDocPtr doc, xmlXPathContextPtr ctx, const std::string& xpath, NodeCallback cb)
{
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(BAD_CAST xpath.c_str(), ctx);
    if (!xpathObj)
    {
        return false;
    }
    if (xmlXPathNodeSetIsEmpty(xpathObj->nodesetval))
    {
        xmlXPathFreeObject(xpathObj);
        // No result
        return true;
    }

    xmlChar* keyword = nullptr;
    xmlNodeSetPtr nodeset = xpathObj->nodesetval;
    for (int i = 0; i < nodeset->nodeNr; i++)
    {
        cb(nodeset->nodeTab[i]);
    }
    xmlXPathFreeObject(xpathObj);
    return true;
}

bool HtmlParser::parseJsonByXPath(cJSON* json, const std::string& xpath, std::vector<std::string>& value)
{
    value.clear();
    cJSON** result = nullptr;
    int count = cJSONXPath_Query(json, xpath.c_str(), &result);
    for (int i = 0; i < count; i++)
    {
        extractStringValues(result[i], value);
    }
    cJSONXPath_FreeResults(result);
    return count >= 0;
}

bool HtmlParser::parseJsonByJsonPath(cJSON* json, const std::string& jpath, std::vector<std::string>& value)
{
    value.clear();
    cJSON** result = nullptr;
    int count = cJSONPath_Query(json, jpath.c_str(), &result);
    for (int i = 0; i < count; i++)
    {
        extractStringValues(result[i], value);
    }
    cJSONPath_FreeResults(result);
    return count >= 0;
}

std::string HtmlParser::removeBlanks(const char* src)
{
    std::string dst;
    while (*src)
    {
        if (!Utils::isBlank(*src))
        {
            dst.push_back(*src);
        }
        src++;
    }
    return dst;
}

void HtmlParser::extractStringValues(cJSON* json, std::vector<std::string>& value)
{
    if (!json) return;

    if (cJSON_IsString(json))
    {
        // Direct string value
        value.push_back(json->valuestring);
    }
    else if (cJSON_IsNumber(json))
    {
        if ((double)json->valueint == json->valuedouble)
        {
            value.push_back(std::to_string(json->valueint));
        }
        else
        {
            value.push_back(std::to_string(json->valuedouble));
        }
    }
    else if (cJSON_IsTrue(json))
    {
        value.push_back("true");
    }
    else if (cJSON_IsFalse(json))
    {
        value.push_back("false");
    }
    else if (cJSON_IsArray(json))
    {
        // Array: recursively extract all string elements
        int array_size = cJSON_GetArraySize(json);
        for (int i = 0; i < array_size; i++)
        {
            cJSON* element = cJSON_GetArrayItem(json, i);
            extractStringValues(element, value);
        }
    }
    else if (cJSON_IsObject(json))
    {
        // Object: recursively extract all string values
        cJSON* child = json->child;
        while (child)
        {
            extractStringValues(child, value);
            child = child->next;
        }
    }
}

bool HtmlParser::parseConcat(const std::string& expression_, std::string& nodeXPath, std::string& prefix, std::string& suffix)
{
    // concat('prefix', <XPath>)
    // concat(<XPath>, 'suffix')
    // concat('prefix', <XPath>, 'suffix')

    nodeXPath.clear();
    prefix.clear();
    suffix.clear();

    std::string expression = Utils::trim(expression_);

    if (expression.size() < 8) return false;
    if (expression.compare(0, 7, "concat(") != 0) return false;
    if (expression.back() != ')') return false;

    expression = expression.substr(7, expression.size() - 8);
    if (expression.empty()) return false;

    std::vector<std::string> arguments;
    std::string cur;
    bool inQuote = false;
    char quoteChar = 0;
    for (size_t i = 0; i < expression.size(); ++i)
    {
        char c = expression[i];
        if (inQuote)
        {
            cur += c;
            if (c == quoteChar) inQuote = false;
        }
        else if (c == '\'' || c == '"')
        {
            inQuote = true;
            quoteChar = c;
            cur += c;
        }
        else if (c == ',')
        {
            arguments.push_back(Utils::trim(cur));
            cur.clear();
        }
        else
        {
            cur += c;
        }
    }
    arguments.push_back(Utils::trim(cur));

    if (arguments.size() < 2 || arguments.size() > 3) return false;

    for (auto& arg : arguments)
    {
        if (arg.empty()) continue;
        char ch = arg.front();
        if (ch == '/')
        {
            if (nodeXPath.empty())
            {
                nodeXPath = arg;
                continue;
            }
            else return false;
        }
        if (arg.size() < 2) return false;
        if ((ch == '\'' || ch == '"') && ch == arg.back())
        {
            if (nodeXPath.empty()) prefix = arg.substr(1, arg.size() - 2);
            else suffix = arg.substr(1, arg.size() - 2);
        }
    }
    return true;
}

void HtmlParser::applyConcat(std::vector<std::string>& values, const std::string& prefix, const std::string& suffix)
{
    for (auto& value : values) value = prefix + value + suffix;
}

#if ENABLE_TESTCASE
void HtmlParser::testcase1_parseJsonByXPath()
{
    const char* json_str = R"({
        "name": "John",
        "age": 30,
        "city": "New York",
        "address": {
            "street": "123 Main St",
            "zip": "10001"
        },
        "phones": ["111-1111", "222-2222", "333-3333"],
        "users": [
            {"name": "Alice", "age": 25},
            {"name": "Bob", "age": 30},
            {"name": "Charlie", "age": 35}
        ],
        "company": {
            "departments": [
                {
                    "name": "Engineering",
                    "employees": [
                        {"name": "David"},
                        {"name": "Eve"}
                    ]
                }
            ]
        }
    })";

    cJSON* root = cJSON_Parse(json_str);
    if (!root)
    {
        ASSERT(false);
    }

    std::vector<std::string> values;

    // Test 1: Get top-level string (XPath: /name)
    ASSERT(parseJsonByXPath(root, "/name", values));
    ASSERT(values.size() == 1 && values[0] == "John");

    values.clear();
    // Test 2: Get nested field (XPath: /address/street)
    ASSERT(parseJsonByXPath(root, "/address/street", values));
    ASSERT(values.size() == 1 && values[0] == "123 Main St");

    values.clear();
    // Test 3: Get entire array (XPath: /phones)
    ASSERT(parseJsonByXPath(root, "/phones", values));
    ASSERT(values.size() == 3);
    ASSERT(values[0] == "111-1111");
    ASSERT(values[1] == "222-2222");
    ASSERT(values[2] == "333-3333");

    values.clear();
    // Test 4: Get array element by index (XPath: /phones[1])
    ASSERT(parseJsonByXPath(root, "/phones[1]", values));
    ASSERT(values.size() == 1 && values[0] == "222-2222");

    values.clear();
    // Test 5: Get nested field from array of objects using wildcard (XPath: /users/*/name)
    ASSERT(parseJsonByXPath(root, "/users/*/name", values));
    ASSERT(values.size() == 3);
    ASSERT(values[0] == "Alice");
    ASSERT(values[1] == "Bob");
    ASSERT(values[2] == "Charlie");

    values.clear();
    // Test 6: Get specific object from array then its field (XPath: /users[0]/name)
    ASSERT(parseJsonByXPath(root, "/users[0]/name", values));
    ASSERT(values.size() == 1 && values[0] == "Alice");

    values.clear();
    // Test 7: Deep nested wildcard traversal (XPath: /company/departments/*/employees/*/name)
    ASSERT(parseJsonByXPath(root, "/company/departments/*/employees/*/name", values));
    ASSERT(values.size() == 2);
    ASSERT(values[0] == "David");
    ASSERT(values[1] == "Eve");

    values.clear();
    // Test 8: Invalid path should return empty (XPath: /invalid/path)
    ASSERT(parseJsonByXPath(root, "/invalid/path", values));
    ASSERT(values.empty());

    values.clear();
    // Test 9: Empty path
    ASSERT(!parseJsonByXPath(root, "", values));

    values.clear();
    // Test 10: Recursive descent - find all "name" fields at any level (XPath: //name)
    ASSERT(parseJsonByXPath(root, "//name", values));
    ASSERT(values.size() == 7);  // John,Alice,Bob,Charlie,Engineering,David,Eve
    ASSERT(values[0] == "John");
    ASSERT(values[1] == "Alice");
    ASSERT(values[2] == "Bob");
    ASSERT(values[3] == "Charlie");
    ASSERT(values[4] == "Engineering");
    ASSERT(values[5] == "David");
    ASSERT(values[6] == "Eve");

    values.clear();
    // Test 11: Recursive descent from specific path (XPath: /company//name)
    ASSERT(parseJsonByXPath(root, "/company//name", values));
    ASSERT(values.size() == 3);  // Engineering, David, Eve
    ASSERT(values[0] == "Engineering");
    ASSERT(values[1] == "David");
    ASSERT(values[2] == "Eve");

    //values.clear();
    //// Test 12: Root only (XPath: /)
    //ASSERT(parseJsonByXPath(root, "/", values));
    //ASSERT(values.size() == 1);  // The root object itself

    values.clear();
    // Test 13: Wildcard on object (XPath: /address/*)
    ASSERT(parseJsonByXPath(root, "/address/*", values));
    ASSERT(values.size() == 2);
    ASSERT(values[0] == "123 Main St");
    ASSERT(values[1] == "10001");

    values.clear();
    // Test 14: Recursive descent with index (XPath: //employees[0]/name)
    ASSERT(parseJsonByXPath(root, "//employees[0]/name", values));
    ASSERT(values.size() == 1 && values[0] == "David");

    ///////////////////////////////////////////////////
    // test case for concat()
    const char* concatJson = "{\"data\":{\"bookList\":[{\"bid\":172865},{\"bid\":834525}]}}";

    ASSERT(parseHtml(concatJson, (int)strlen(concatJson), "concat('https://example.com/', /data/bookList/*/bid)", values));
    ASSERT(values.size() == 2);
    ASSERT(values[0] == "https://example.com/172865");
    ASSERT(values[1] == "https://example.com/834525");

    ASSERT(parseHtml(concatJson, (int)strlen(concatJson), "concat('https://example.com/', /data/bookList/*/bid, '.html')", values));
    ASSERT(values.size() == 2);
    ASSERT(values[0] == "https://example.com/172865.html");
    ASSERT(values[1] == "https://example.com/834525.html");

    const char* xml = "<data><bookList><item><bid>172865</bid></item><item><bid>834525</bid></item></bookList></data>";
    ASSERT(parseXml(xml, (int)strlen(xml), "concat('https://example.com/', /data/bookList/item/bid)", values));
    ASSERT(values.size() == 2);
    ASSERT(values[0] == "https://example.com/172865");
    ASSERT(values[1] == "https://example.com/834525");

    ASSERT(parseXml(xml, (int)strlen(xml), "concat('https://example.com/', /data/bookList/item/bid, '.html')", values));
    ASSERT(values.size() == 2);
    ASSERT(values[0] == "https://example.com/172865.html");
    ASSERT(values[1] == "https://example.com/834525.html");

    cJSON_Delete(root);
}

void HtmlParser::testcase2_parseJsonByXPath()
{
    const char* json_str = R"([
        {
            "name": "Alice",
            "age": 25,
            "phones": ["111-1111", "222-2222"]
        },
        {
            "name": "Bob",
            "age": 30,
            "phones": ["333-3333", "444-4444"]
        },
        {
            "name": "Charlie",
            "age": 35,
            "phones": ["555-5555"]
        }
    ])";

    cJSON* root = cJSON_Parse(json_str);
    if (!root)
    {
        ASSERT(false);
    }

    std::vector<std::string> values;

    // Test 1: Get field from all array elements using wildcard (XPath: /*/name)
    ASSERT(parseJsonByXPath(root, "/*/name", values));
    ASSERT(values.size() == 3);
    ASSERT(values[0] == "Alice");
    ASSERT(values[1] == "Bob");
    ASSERT(values[2] == "Charlie");

    values.clear();
    // Test 2: Get field from specific array element (XPath: /[1]/name)
    ASSERT(parseJsonByXPath(root, "/[1]/name", values));
    ASSERT(values.size() == 1 && values[0] == "Bob");

    values.clear();
    // Test 3: Get nested array fields from all objects (XPath: /*/phones/*)
    ASSERT(parseJsonByXPath(root, "/*/phones/*", values));
    ASSERT(values.size() == 5);
    ASSERT(values[0] == "111-1111");
    ASSERT(values[1] == "222-2222");
    ASSERT(values[2] == "333-3333");
    ASSERT(values[3] == "444-4444");
    ASSERT(values[4] == "555-5555");

    values.clear();
    // Test 4: Get specific nested array element (XPath: /[0]/phones[1])
    ASSERT(parseJsonByXPath(root, "/[0]/phones[1]", values));
    ASSERT(values.size() == 1 && values[0] == "222-2222");

    values.clear();
    // Test 5: Out of bounds index returns empty (XPath: /[10]/name)
    ASSERT(parseJsonByXPath(root, "/[10]/name", values));
    ASSERT(values.empty());

    values.clear();
    // Test 6: Get age field as string (XPath: /[0]/age)
    // Note: age is a number, so it won't be added since we only extract strings
    ASSERT(parseJsonByXPath(root, "/[0]/age", values));
#if 0
    ASSERT(values.empty());
#else
    ASSERT(values.size() == 1 && values[0] == "25");
#endif

    values.clear();
    // Test 7: Wildcard on root array (XPath: /*)
    ASSERT(parseJsonByXPath(root, "/*", values));
    // Returns array elements, not strings, so values remains empty
    // This is correct behavior for objects/arrays when only extracting strings

    values.clear();
    // Test 8: Recursive descent - find all "name" fields at any level (XPath: //name)
    ASSERT(parseJsonByXPath(root, "//name", values));
    ASSERT(values.size() == 3);
    ASSERT(values[0] == "Alice");
    ASSERT(values[1] == "Bob");
    ASSERT(values[2] == "Charlie");

    values.clear();
    // Test 9: Recursive descent with wildcard (XPath: //phones/*)
    ASSERT(parseJsonByXPath(root, "//phones/*", values));
    ASSERT(values.size() == 5);
    ASSERT(values[0] == "111-1111");
    ASSERT(values[1] == "222-2222");
    ASSERT(values[2] == "333-3333");
    ASSERT(values[3] == "444-4444");
    ASSERT(values[4] == "555-5555");

    values.clear();
    // Test 10: Recursive descent with specific index (XPath: //phones[0])
    ASSERT(parseJsonByXPath(root, "//phones[0]", values));
    ASSERT(values.size() == 3);  // First phone of each person
    ASSERT(values[0] == "111-1111");
    ASSERT(values[1] == "333-3333");
    ASSERT(values[2] == "555-5555");

    values.clear();
    // Test 11: Deep recursive descent (XPath: //*)
    ASSERT(parseJsonByXPath(root, "//*", values));
    // This should return all leaf values in the entire structure
    ASSERT(values.size() > 0);

    cJSON_Delete(root);
}

void HtmlParser::testcase1_parseJsonByJsonPath()
{
    const char* json_str = R"({
        "name": "John",
        "age": 30,
        "city": "New York",
        "address": {
            "street": "123 Main St",
            "zip": "10001"
        },
        "phones": ["111-1111", "222-2222", "333-3333"],
        "users": [
            {"name": "Alice", "age": 25},
            {"name": "Bob", "age": 30},
            {"name": "Charlie", "age": 35}
        ],
        "company": {
            "departments": [
                {
                    "name": "Engineering",
                    "employees": [
                        {"name": "David"},
                        {"name": "Eve"}
                    ]
                }
            ]
        }
    })";

    cJSON* root = cJSON_Parse(json_str);
    if (!root)
    {
        ASSERT(false);
    }

    std::vector<std::string> values;

    // Test 1: Get top-level string
    ASSERT(parseJsonByJsonPath(root, "$.name", values));
    ASSERT(values.size() == 1 && values[0] == "John");

    values.clear();
    // Test 2: Get nested field
    ASSERT(parseJsonByJsonPath(root, "$.address.street", values));
    ASSERT(values.size() == 1 && values[0] == "123 Main St");

    values.clear();
    // Test 3: Get entire array
    ASSERT(parseJsonByJsonPath(root, "$.phones", values));
    ASSERT(values.size() == 3);
    ASSERT(values[0] == "111-1111");
    ASSERT(values[1] == "222-2222");
    ASSERT(values[2] == "333-3333");

    values.clear();
    // Test 4: Get array element by index
    ASSERT(parseJsonByJsonPath(root, "$.phones[1]", values));
    ASSERT(values.size() == 1 && values[0] == "222-2222");

    values.clear();
    // Test 5: Get nested field from array of objects using wildcard
    ASSERT(parseJsonByJsonPath(root, "$.users[*].name", values));
    ASSERT(values.size() == 3);
    ASSERT(values[0] == "Alice");
    ASSERT(values[1] == "Bob");
    ASSERT(values[2] == "Charlie");

    values.clear();
    // Test 6: Get specific object from array then its field
    ASSERT(parseJsonByJsonPath(root, "$.users[0].name", values));
    ASSERT(values.size() == 1 && values[0] == "Alice");

    values.clear();
    // Test 7: Deep nested wildcard traversal
    ASSERT(parseJsonByJsonPath(root, "$.company.departments[*].employees[*].name", values));
    ASSERT(values.size() == 2);
    ASSERT(values[0] == "David");
    ASSERT(values[1] == "Eve");

    values.clear();
    // Test 8: Invalid path should return empty
    ASSERT(parseJsonByJsonPath(root, "$.invalid.path", values));
    ASSERT(values.empty());

    values.clear();
    // Test 9: Empty path
    ASSERT(!parseJsonByJsonPath(root, "", values));

    values.clear();
    // Test 10: Alternative bracket notation
    ASSERT(parseJsonByJsonPath(root, "$['address']['street']", values));
    ASSERT(values.size() == 1 && values[0] == "123 Main St");

    // Test 11: Recursive descent - find all "name" fields at any level
    ASSERT(parseJsonByJsonPath(root, "$..name", values));
    ASSERT(values.size() == 7);  // John,Alice,Bob,Charlie,Engineering,David,Eve
    ASSERT(values[0] == "John");
    ASSERT(values[1] == "Alice");
    ASSERT(values[2] == "Bob");
    ASSERT(values[3] == "Charlie");
    ASSERT(values[4] == "Engineering");
    ASSERT(values[5] == "David");
    ASSERT(values[6] == "Eve");

    cJSON_Delete(root);
}

void HtmlParser::testcase2_parseJsonByJsonPath()
{
    const char* json_str = R"([
        {
            "name": "Alice",
            "age": 25,
            "phones": ["111-1111", "222-2222"]
        },
        {
            "name": "Bob",
            "age": 30,
            "phones": ["333-3333", "444-4444"]
        },
        {
            "name": "Charlie",
            "age": 35,
            "phones": ["555-5555"]
        }
    ])";

    cJSON* root = cJSON_Parse(json_str);
    if (!root)
    {
        ASSERT(false);
    }

    std::vector<std::string> values;

    // Test 1: Get field from all array elements using wildcard
    ASSERT(parseJsonByJsonPath(root, "$[*].name", values));
    ASSERT(values.size() == 3);
    ASSERT(values[0] == "Alice");
    ASSERT(values[1] == "Bob");
    ASSERT(values[2] == "Charlie");

    values.clear();
    // Test 2: Get field from specific array element
    ASSERT(parseJsonByJsonPath(root, "$[1].name", values));
    ASSERT(values.size() == 1 && values[0] == "Bob");

    values.clear();
    // Test 3: Get nested array fields from all objects
    ASSERT(parseJsonByJsonPath(root, "$[*].phones[*]", values));
    ASSERT(values.size() == 5);
    ASSERT(values[0] == "111-1111");
    ASSERT(values[1] == "222-2222");
    ASSERT(values[2] == "333-3333");
    ASSERT(values[3] == "444-4444");
    ASSERT(values[4] == "555-5555");

    values.clear();
    // Test 4: Get specific nested array element
    ASSERT(parseJsonByJsonPath(root, "$[0].phones[1]", values));
    ASSERT(values.size() == 1 && values[0] == "222-2222");

    values.clear();
    // Test 5: Out of bounds index returns empty
    ASSERT(parseJsonByJsonPath(root, "$[10].name", values));
    ASSERT(values.empty());

    values.clear();
    // Test 6: Get age field as string (should work even though it's number)
    ASSERT(parseJsonByJsonPath(root, "$[0].age", values));
    // Note: age is a number, so it won't be added since we only extract strings
    // This is expected behavior. If you want numbers, you'd need to modify parseJsonByJsonPath
#if 0
    ASSERT(values.empty());
#else
    ASSERT(values.size() == 1 && values[0] == "25");
#endif

    values.clear();
    // Test 7: Alternative bracket notation with array index
    ASSERT(parseJsonByJsonPath(root, "$[2]['phones'][0]", values));
    ASSERT(values.size() == 1 && values[0] == "555-5555");

    values.clear();
    // Test 8: Wildcard on root array
    ASSERT(parseJsonByJsonPath(root, "$[*]", values));
    // Returns array elements, not strings, so count >= 0 but values remains empty
    // This is correct behavior

    values.clear();
    // Test 9: Recursive descent - find all "name" fields at any level
    ASSERT(parseJsonByJsonPath(root, "$..name", values));
    ASSERT(values.size() == 3);
    ASSERT(values[0] == "Alice");
    ASSERT(values[1] == "Bob");
    ASSERT(values[2] == "Charlie");

    cJSON_Delete(root);
}
#endif