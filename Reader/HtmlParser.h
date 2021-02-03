#ifndef __CHTML_PARSER_H__
#define __CHTML_PARSER_H__

#include <string>
#include <vector>

class HtmlParser
{
private:
    HtmlParser();
    ~HtmlParser();

public:
    static HtmlParser* Instance();
    static void ReleaseInstance();

    int HtmlParseByXpath(const char *html, int len, const std::string &xpath, std::vector<std::string> &value, bool *stop, bool clear = false);

    // for multi parser
    int HtmlParseBegin(const char *html, int len, void **doc, void **ctx, bool* stop);
    int HtmlParseByXpath(void *doc, void *ctx, const std::string &xpath, std::vector<std::string> &value, bool* stop, bool clear = false);
    int HtmlParseEnd(void *doc, void *ctx);

private:
    char * CreateContent(const char* xml);
    void ReleaseContent(char *content);
};

#endif // !__CHTML_PARSER_H__
