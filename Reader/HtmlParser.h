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

    int HtmlParseByXpath(const char *html, int len, const std::string &xpath, std::vector<std::string> &value, BOOL *stop, BOOL clear = FALSE);

    // for multi parser
    int HtmlParseBegin(const char *html, int len, void **doc, void **ctx, BOOL* stop);
    int HtmlParseByXpath(void *doc, void *ctx, const std::string &xpath, std::vector<std::string> &value, BOOL* stop, BOOL clear = FALSE);
    int HtmlParseEnd(void *doc, void *ctx);

    int FormatHtml(char *html, int len, char **htmlfmt, int *fmtlen);
    void FreeFormat(char *htmlfmt);

private:
    char * CreateContent(const char* xml);
    void ReleaseContent(char *content);
};

#endif // !__CHTML_PARSER_H__
