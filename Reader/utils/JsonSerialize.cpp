#include "JsonSerialize.h"
#include "Logger.h"
#include "cJSON_Serialize.hpp"
#include "framework.h"

CJSON_DEFINE_MENU(DisplayMode)
CJSON_DEFINE_MENU(WrapMode)
CJSON_DEFINE_MENU(MousePageMode)
CJSON_DEFINE_MENU(ScrollMode)
CJSON_DEFINE_MENU(AutoPageMode)
CJSON_DEFINE_MENU(AutoPageIntervalMode)
CJSON_DEFINE_MENU(KeyType)
CJSON_DEFINE_MENU(KeyId)
CJSON_DEFINE_MENU(MouseBackAction)
CJSON_DEFINE_MENU(ChapterRule::Rule)

CJSON_DEFINE_TYPE(POINT, x, y)
CJSON_DEFINE_TYPE(SIZE, cx, cy)
CJSON_DEFINE_TYPE(RECT, left, top, right, bottom)
CJSON_DEFINE_TYPE(LOGFONTW, lfHeight, lfWidth, lfEscapement, lfOrientation, lfWeight, lfItalic, lfUnderline, lfStrikeOut, lfCharSet, lfOutPrecision, lfClipPrecision, lfQuality, lfPitchAndFamily, lfFaceName)
CJSON_DEFINE_TYPE(WINDOWPLACEMENT, length, flags, showCmd, ptMinPosition, ptMaxPosition, rcNormalPosition)
CJSON_DEFINE_TYPE(WindowInfo, mode, placement, borderlessRect, style, exStyle, hideTaskbar, minToTray, alpha, textAlpha)
CJSON_DEFINE_TYPE(TextInfo, unified, textFont, textColor, titleFont, titleColor)
CJSON_DEFINE_TYPE(BackgroundInfo, color, enableImage, fileName, mode)
CJSON_DEFINE_TYPE(NetworkInfo, enableProxy, port, addr, user, pass, adapterName)
CJSON_DEFINE_TYPE(PagingInfo, mouseMode, scrollMode, preserveLines, autoMode, intervalMode, intervalValue)
CJSON_DEFINE_TYPE(KeyItem, type, id, value, enable)
CJSON_DEFINE_TYPE(KeysetInfo, enableMouseLRHide, enableEscHide, mouseBackAction, keys)
CJSON_DEFINE_TYPE(LayoutConfig, borderL, borderT, borderR, borderB, charGap, lineGap, paragraphGap, indent, noBlansLine, wordWrap, splitChapter)
CJSON_DEFINE_TYPE(ChapterRule, rule, keyword, regex)
CJSON_DEFINE_TYPE(UpgradeInfo, ignoreVersion, lastCheckTime)
CJSON_DEFINE_TYPE(WebShelfInfo, serverUrl)
#if ENABLE_TAG
CJSON_DEFINE_TYPE(TagInfo, enable, keyword, font, textColor, backgroundColor)
#endif
CJSON_DEFINE_TYPE(BookSource, title, host, queryUrl, queryMethod, queryParams, queryCharset, bookNameXpath, bookMainpageXpath, bookAuthorXpath, enableChapterPage, chapterPageXpath, chapterTitleXpath, chapterUrlXpath, enableChapterNext, chapterNextUrlXpath, chapterNextKeywordXpath, chapterNextKeyword, contentXpath, enableContentNext, contentNextUrlXpath, contentNextKeywordXpath, contentNextKeyword, contentFilterType, contentFilterKeyword)
CJSON_DEFINE_TYPE(BookInfo, fileName, index, marks, hasNew)
#if ENABLE_TAG
CJSON_DEFINE_TYPE(Data, version, window, background, network, paging, keyset, styles, layout, chapterRule, upgrade, webShelf, customColors, tags, sources, books)
#else
CJSON_DEFINE_TYPE(Data, version, window, background, network, paging, keyset, styles, layout, chapterRule, upgrade, webShelf, customColors, sources, books)
#endif
CJSON_DEFINE_TYPE(VersionInfo, version, downloadurl, description, force)
CJSON_DEFINE_TYPE(VersionList, list)
CJSON_DEFINE_TYPE(WebShelfItem, name, author, durChapterTitle, latestChapterTitle, origin, bookUrl, durChapterIndex, durChapterPos, totalChapterNum, durChapterTime)
CJSON_DEFINE_TYPE(WebShelfList, data, errorMsg, isSuccess)
CJSON_DEFINE_TYPE(WebChapterItem, index, title, url)
CJSON_DEFINE_TYPE(WebChapterList, data, errorMsg, isSuccess)
CJSON_DEFINE_TYPE(WebContent, data, errorMsg, isSuccess)
CJSON_DEFINE_TYPE(WebSaveBookProgress, name, author, durChapterTitle, durChapterIndex, durChapterPos, durChapterTime)


bool JsonSerialize::enserialize(const Data& data, std::vector<uint8_t>& bytes)
{
    cJSON* root = cJSON_CreateObject();
    to_json(root, nullptr, data);
    char* jsonStr = cJSON_Print(root);
    bytes.assign(jsonStr, jsonStr + strlen(jsonStr));
    cJSON_free(jsonStr);
    cJSON_Delete(root);
    return true;
}

bool JsonSerialize::deserialize(const std::vector<uint8_t>& bytes, Data& data)
{
    std::string jsonStr(bytes.begin(), bytes.end());
    cJSON* root = cJSON_Parse(jsonStr.c_str());
    if (!root)
    {
        lgerror("Failed to parse JSON: %s", cJSON_GetErrorPtr());
        return false;
    }
    from_json(root, nullptr, data);
    cJSON_Delete(root);
    return true;
}

bool JsonSerialize::enserialize(const std::vector<BookSource>& bookSrcs, std::vector<uint8_t>& bytes)
{
    cJSON* root = cJSON_CreateArray();
    to_json(root, nullptr, bookSrcs);
    char* jsonStr = cJSON_Print(root);
    bytes.assign(jsonStr, jsonStr + strlen(jsonStr));
    cJSON_free(jsonStr);
    cJSON_Delete(root);
    return true;
}

bool JsonSerialize::deserialize(const std::vector<uint8_t>& bytes, std::vector<BookSource>& bookSrcs)
{
    std::string jsonStr(bytes.begin(), bytes.end());
    cJSON* root = cJSON_Parse(jsonStr.c_str());
    if (!root)
    {
        lgerror("Failed to parse book source JSON: %s", cJSON_GetErrorPtr());
        return false;
    }

    if (cJSON_IsObject(root))
    {
        cJSON* bookSources = cJSON_GetObjectItem(root, "book_sources");
        if (bookSources && cJSON_IsArray(bookSources) && cJSON_GetArraySize(bookSources) > 0)
        {
            bool result = deserializeFromOldBookSource(bookSources, bookSrcs);
            cJSON_Delete(root);
            return result;
        }
        lgerror("Failed to parse book source JSON: %s", cJSON_GetErrorPtr());
        cJSON_Delete(root);
        return false;
    }

    from_json(root, nullptr, bookSrcs);
    cJSON_Delete(root);
    return true;
}

bool JsonSerialize::enserialize(const VersionList& list, std::vector<uint8_t>& bytes)
{
    cJSON* root = cJSON_CreateObject();
    to_json(root, nullptr, list);
    char* jsonStr = cJSON_Print(root);
    bytes.assign(jsonStr, jsonStr + strlen(jsonStr));
    cJSON_free(jsonStr);
    cJSON_Delete(root);
    return true;
}

bool JsonSerialize::deserialize(const std::vector<uint8_t>& bytes, VersionList& list)
{
    std::string jsonStr(bytes.begin(), bytes.end());
    cJSON* root = cJSON_Parse(jsonStr.c_str());
    if (!root)
    {
        lgerror("Failed to parse version list JSON: %s", cJSON_GetErrorPtr());
        return false;
    }
    from_json(root, nullptr, list);
    cJSON_Delete(root);
    return true;
}

bool JsonSerialize::enserialize(const WebShelfItem& data, std::vector<uint8_t>& bytes)
{
    cJSON* root = cJSON_CreateObject();
    to_json(root, nullptr, data);
    char* jsonStr = cJSON_Print(root);
    bytes.assign(jsonStr, jsonStr + strlen(jsonStr));
    cJSON_free(jsonStr);
    cJSON_Delete(root);
    return true;
}

bool JsonSerialize::deserialize(const std::vector<uint8_t>& bytes, WebShelfItem& data)
{
    std::string jsonStr(bytes.begin(), bytes.end());
    cJSON* root = cJSON_Parse(jsonStr.c_str());
    if (!root)
    {
        lgerror("Failed to parse web shelf item JSON: %s", cJSON_GetErrorPtr());
        return false;
    }
    from_json(root, nullptr, data);
    cJSON_Delete(root);
    return true;
}

bool JsonSerialize::enserialize(const WebShelfList& list, std::vector<uint8_t>& bytes)
{
    cJSON* root = cJSON_CreateObject();
    to_json(root, nullptr, list);
    char* jsonStr = cJSON_Print(root);
    bytes.assign(jsonStr, jsonStr + strlen(jsonStr));
    cJSON_free(jsonStr);
    cJSON_Delete(root);
    return true;
}

bool JsonSerialize::deserialize(const std::vector<uint8_t>& bytes, WebShelfList& list)
{
    std::string jsonStr(bytes.begin(), bytes.end());
    cJSON* root = cJSON_Parse(jsonStr.c_str());
    if (!root)
    {
        lgerror("Failed to parse web shelf list JSON: %s", cJSON_GetErrorPtr());
        return false;
    }
    from_json(root, nullptr, list);
    cJSON_Delete(root);
    return true;
}

bool JsonSerialize::enserialize(const WebChapterList& list, std::vector<uint8_t>& bytes)
{
    cJSON* root = cJSON_CreateObject();
    to_json(root, nullptr, list);
    char* jsonStr = cJSON_Print(root);
    bytes.assign(jsonStr, jsonStr + strlen(jsonStr));
    cJSON_free(jsonStr);
    cJSON_Delete(root);
    return true;
}

bool JsonSerialize::deserialize(const std::vector<uint8_t>& bytes, WebChapterList& list)
{
    std::string jsonStr(bytes.begin(), bytes.end());
    cJSON* root = cJSON_Parse(jsonStr.c_str());
    if (!root)
    {
        lgerror("Failed to parse web chapter list JSON: %s", cJSON_GetErrorPtr());
        return false;
    }
    from_json(root, nullptr, list);
    cJSON_Delete(root);
    return true;
}

bool JsonSerialize::enserialize(const WebContent& data, std::vector<uint8_t>& bytes)
{
    cJSON* root = cJSON_CreateObject();
    to_json(root, nullptr, data);
    char* jsonStr = cJSON_Print(root);
    bytes.assign(jsonStr, jsonStr + strlen(jsonStr));
    cJSON_free(jsonStr);
    cJSON_Delete(root);
    return true;
}

bool JsonSerialize::deserialize(const std::vector<uint8_t>& bytes, WebContent& data)
{
    std::string jsonStr(bytes.begin(), bytes.end());
    cJSON* root = cJSON_Parse(jsonStr.c_str());
    if (!root)
    {
        lgerror("Failed to parse web content JSON: %s", cJSON_GetErrorPtr());
        return false;
    }
    from_json(root, nullptr, data);
    cJSON_Delete(root);
    return true;
}

bool JsonSerialize::enserialize(const WebSaveBookProgress& data, std::vector<uint8_t>& bytes)
{
    cJSON* root = cJSON_CreateObject();
    to_json(root, nullptr, data);
    char* jsonStr = cJSON_PrintUnformatted(root);
    bytes.assign(jsonStr, jsonStr + strlen(jsonStr));
    cJSON_free(jsonStr);
    cJSON_Delete(root);
    return true;
}

bool JsonSerialize::deserialize(const std::vector<uint8_t>& bytes, WebSaveBookProgress& data)
{
    std::string jsonStr(bytes.begin(), bytes.end());
    cJSON* root = cJSON_Parse(jsonStr.c_str());
    if (!root)
    {
        lgerror("Failed to parse web save book progress JSON: %s", cJSON_GetErrorPtr());
        return false;
    }
    from_json(root, nullptr, data);
    cJSON_Delete(root);
    return true;
}

struct BookSourceOld
{
    std::wstring title;
    std::string host;
    std::string query_url;
    int query_method; // 0: GET, 1: POST
    std::string query_params; // POST params
    int query_charset; // 0: auto, 1: utf8, 2: gbk
    // query page
    std::string book_name_xpath;
    std::string book_mainpage_xpath;
    std::string book_author_xpath;

    // main page
    int enable_chapter_page;
    std::string chapter_page_xpath;

    // chapter page
    std::string chapter_title_xpath;
    std::string chapter_url_xpath;
    int enable_chapter_next;
    std::string chapter_next_url_xpath;
    std::string chapter_next_keyword_xpath;
    std::string chapter_next_keyword;

    // content page
    std::string content_xpath;
    int enable_content_next;
    std::string content_next_url_xpath;
    std::string content_next_keyword_xpath;
    std::string content_next_keyword;
    int content_filter_type; // 0: disable, 1: keyword, 2: regex
    std::wstring content_filter_keyword;
};
CJSON_DEFINE_TYPE(BookSourceOld, title, host, query_url, query_method, query_params, query_charset, book_name_xpath, book_mainpage_xpath, book_author_xpath, enable_chapter_page, chapter_page_xpath, chapter_title_xpath, chapter_url_xpath, enable_chapter_next, chapter_next_url_xpath, chapter_next_keyword_xpath, chapter_next_keyword, content_xpath, enable_content_next, content_next_url_xpath, content_next_keyword_xpath, content_next_keyword, content_filter_type, content_filter_keyword)

bool JsonSerialize::deserializeFromOldBookSource(cJSON* book_sources, std::vector<BookSource>& bookSrcs)
{
    std::vector<BookSourceOld> oldSources;
    from_json(book_sources, nullptr, oldSources);
    for (auto& oldSrc : oldSources)
    {
        BookSource src;
        src.title = oldSrc.title;
        src.host = oldSrc.host;
        src.queryUrl = oldSrc.query_url;
        src.queryMethod = oldSrc.query_method;
        src.queryParams = oldSrc.query_params;
        src.queryCharset = oldSrc.query_charset;
        src.bookNameXpath = oldSrc.book_name_xpath;
        src.bookMainpageXpath = oldSrc.book_mainpage_xpath;
        src.bookAuthorXpath = oldSrc.book_author_xpath;
        src.enableChapterPage = oldSrc.enable_chapter_page;
        src.chapterPageXpath = oldSrc.chapter_page_xpath;
        src.chapterTitleXpath = oldSrc.chapter_title_xpath;
        src.chapterUrlXpath = oldSrc.chapter_url_xpath;
        src.enableChapterNext = oldSrc.enable_chapter_next;
        src.chapterNextUrlXpath = oldSrc.chapter_next_url_xpath;
        src.chapterNextKeywordXpath = oldSrc.chapter_next_keyword_xpath;
        src.chapterNextKeyword = oldSrc.chapter_next_keyword;
        src.contentXpath = oldSrc.content_xpath;
        src.enableContentNext = oldSrc.enable_content_next;
        src.contentNextUrlXpath = oldSrc.content_next_url_xpath;
        src.contentNextKeywordXpath = oldSrc.content_next_keyword_xpath;
        src.contentNextKeyword = oldSrc.content_next_keyword;
        src.contentFilterType = oldSrc.content_filter_type;
        src.contentFilterKeyword = oldSrc.content_filter_keyword;
        bookSrcs.push_back(src);
    }
    return true;
}