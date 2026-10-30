#pragma once

#include "common.h"
#include "framework.h"
#include <string>
#include <vector>
#include <cstdint>


/////////////////////////////////////////////
// renderer
enum WrapMode
{
    STRETCH,
    TILE,
    TILEFLIP
};

struct TextMetrics
{
    int width;
    int height;
    int count;
};

struct TextStyle
{
    LOGFONTW font;
    COLORREF textColor;
    COLORREF backgroundColor;
    bool haveBackgroundColor;

    TextStyle() {}
    TextStyle(const LOGFONTW& f, COLORREF c) : textColor(c), backgroundColor(0), haveBackgroundColor(false)
    {
        memcpy(&font, &f, sizeof(LOGFONTW));
    }
    TextStyle(const LOGFONTW& f, COLORREF c, COLORREF b) : textColor(c), backgroundColor(b), haveBackgroundColor(true)
    {
        memcpy(&font, &f, sizeof(LOGFONTW));
    }
    bool operator==(const TextStyle& other) const
    {
        return memcmp(&font, &other.font, sizeof(LOGFONTW)) == 0 &&
            textColor == other.textColor &&
            backgroundColor == other.backgroundColor &&
            haveBackgroundColor == other.haveBackgroundColor;
    }
    bool operator!=(const TextStyle& other) const
    {
        return !(*this == other);
    }
};

typedef std::vector<TextStyle> TextStyles;


/////////////////////////////////////////////
// layout
struct LayoutConfig
{
    enum Alignment
    {
        LEFT,
        CENTER
    };

    int  borderL;
    int  borderT;
    int  borderR;
    int  borderB;
    int  charGap;
    int  lineGap;
    int  paragraphGap;
    bool indent;
    bool noBlansLine;
    bool wordWrap;
    bool splitChapter;
    Alignment alignment;

    bool operator==(const LayoutConfig& rhs) const
    {
        return borderL == rhs.borderL &&
            borderT == rhs.borderT &&
            borderR == rhs.borderR &&
            borderB == rhs.borderB &&
            charGap == rhs.charGap &&
            lineGap == rhs.lineGap &&
            paragraphGap == rhs.paragraphGap &&
            indent == rhs.indent &&
            noBlansLine == rhs.noBlansLine &&
            wordWrap == rhs.wordWrap &&
            splitChapter == rhs.splitChapter &&
            alignment == rhs.alignment;
    }
    bool operator!=(const LayoutConfig& rhs) const
    {
        return !(*this == rhs);
    }
};

struct TextStyleConfig
{
    int textIdx;
    int titleIdx;
#if ENABLE_TAG
    std::vector<int> tagIdx;
#endif
};

struct CharInfo
{
    int index; // index for book
    int tsIdx; // index for text style
    int w;
    int h;
};

struct LineInfo
{
    int start; // index for book
    int length;
    int x;
    int w;
    int h;
    int gap;   // line/paragraph spacing
    std::vector<CharInfo> chars;
};

struct PageInfo
{
    int start; // index for book
    int length;
    int x;
    int y;
    int cGap;  // character spacing
    std::vector<LineInfo> lines;
    PageInfo() : start(0), length(0), x(0), y(0), cGap(0) {}
    bool empty() const { return length == 0; }
    void clear()
    {
        start = 0;
        length = 0;
        x = 0;
        y = 0;
        cGap = 0;
        lines.clear();
    }
};


/////////////////////////////////////////////
// book
enum class BookType
{
    UNKNOWN,
    TEXT,
    EPUB,
    MOBI,
    ONLINE,
    WEB
};

enum class BookErrno
{
    SUCC,
    FAIL,
    NOT_EXIST,
    UNKNOWN_FORMAT,
    EMPTY,
    TOO_LARGE,
    OPEN_R_FAIL,
    OPEN_W_FAIL,
    READ_FAIL,
    WRITE_FAIL,
    NOT_SUPPORT,
    BOOKSOURCE_NOT_FOUND,
    PARSE_FAIL,
    REQUEST_FAIL,
    HTTP_STATUS_ERROR,
    XPATH_FAIL,
    JSON_FAIL,
    JSON_NODATA,
};

typedef std::vector<int> Marks;

struct BookInfo
{
    std::wstring fileName;
    int index;
    Marks marks;
    bool hasNew;
};

struct Chapter
{
    int index;
    int titleLineLen;
    std::wstring title;
    std::string url;    // for online book
    int textLen;        // for online book, current chapter content size
};
typedef std::vector<Chapter> Chapters;

struct Cover
{
    char* data;
    int   size;
};

struct ChapterRule
{
    enum Rule
    {
        DEFAULT = 0,
        KEYWORD = 1,
        REGEX = 2,
    } rule;
    std::wstring keyword;
    std::wstring regex;
};


/////////////////////////////////////////////
// web book
struct WebShelfItem
{
    std::wstring name;
    std::wstring author;
    std::wstring durChapterTitle;
    std::wstring latestChapterTitle;
    std::string  origin;        // Hostname of the online book source, or "Local Phone"
    std::string  bookUrl;       // HTTP(S) URL for online books, or content:// / /storage/... path for local phone books
    int          durChapterIndex;
    int          durChapterPos;
    int          totalChapterNum;
    long long    durChapterTime;

    bool operator==(const WebShelfItem& other) const
    {
        return name == other.name
            && author == other.author
            && durChapterTitle == other.durChapterTitle
            && latestChapterTitle == other.latestChapterTitle
            && origin == other.origin
            && bookUrl == other.bookUrl
            && durChapterIndex == other.durChapterIndex
            && durChapterPos == other.durChapterPos
            && totalChapterNum == other.totalChapterNum
            && durChapterTime == other.durChapterTime;
    }
    bool operator!=(const WebShelfItem& other) const
    {
        return !(*this == other);
    }
};

struct WebShelfList
{
    std::vector<WebShelfItem> data;
    std::string errorMsg;
    bool isSuccess;
};

struct WebChapterItem
{
    int index;
    std::string title;
    std::string  url;
};

struct WebChapterList
{
    std::vector<WebChapterItem> data;
    std::string errorMsg;
    bool isSuccess;
};

struct WebContent
{
    std::string data;
    std::string errorMsg;
    bool isSuccess;
};

struct WebShelfInfo
{
    std::string serverUrl;
};

struct WebSaveBookProgress
{
    std::wstring name;
    std::wstring author;
    std::wstring durChapterTitle;
    int          durChapterIndex;
    int          durChapterPos;
    long long    durChapterTime;
};


/////////////////////////////////////////////
// others
enum class DisplayMode
{
    NORMAL,
    BORDERLESS,
    FULLSCREEN,
};

struct WindowInfo
{
    DisplayMode mode;
    WINDOWPLACEMENT placement;
    RECT borderlessRect;
    DWORD style;
    DWORD exStyle;
    bool hideTaskbar;
    bool minToTray;
    BYTE alpha;
    BYTE textAlpha;
};

struct TextInfo
{
    bool unified;
    bool viewFont;
    LOGFONTW textFont;
    COLORREF textColor;
    LOGFONTW titleFont;
    COLORREF titleColor;

    bool operator==(const TextInfo& other) const
    {
        return unified == other.unified &&
               viewFont == other.viewFont &&
               memcmp(&textFont, &other.textFont, sizeof(LOGFONTW)) == 0 &&
               textColor == other.textColor &&
               memcmp(&titleFont, &other.titleFont, sizeof(LOGFONTW)) == 0 &&
               titleColor == other.titleColor;
    }
    bool operator!=(const TextInfo& other) const
    {
        return !(*this == other);
    }
};

struct BackgroundInfo
{
    bool enableImage;
    std::wstring fileName;
    WrapMode mode;
    COLORREF color;
};

struct NetworkInfo
{
    // proxy
    bool enableProxy;
    int port;
    std::wstring addr;
    std::wstring user;
    std::wstring pass;
    // Adapter 
    std::wstring adapterName;
};

enum class MousePageMode
{
    DISABLED,
    CLICK_ANYWHERE,
    CLICK_SIDES
};

enum class AutoPageMode
{
    PAGE_FLIP,
    LINE_SCROLL,
};

enum class AutoPageIntervalMode
{
    FIXED_TIME,
    BY_CHAR_COUNT,
};

enum class ScrollMode
{
    LINE,
    PAGE
};

struct PagingInfo
{
    MousePageMode mouseMode;
    ScrollMode scrollMode;
    int preserveLines;
    // auto page
    AutoPageMode autoMode;
    AutoPageIntervalMode intervalMode;
    int intervalValue;
};

enum class KeyType
{
    HOTKEY,
    SHORTCUT,
};

enum class KeyId
{
    HIDE_WIN,
    HIDE_BORDER,
    FULL_SCREEN,
    TOP_MOST,
    OPEN_FILE,
    ADD_MARK,
    AUTO_PAGE,
    SEARCH,
    SEEK,
    EDIT_MODE,
    PAGE_UP,
    PAGE_DOWN,
    LINE_UP,
    LINE_DOWN,
    CHAPTER_UP,
    CHAPTER_DOWN,
    ZOOM_IN,
    ZOOM_OUT,
    REFRESH_CHAPTER,
    REFRESH_CONTENT,
    MOUSE_HIDE,
    MAX_COUNT,
};

enum class MouseBackAction
{
    NOTHING = 0,
    RESTORE,
    RESTORE_WITH_CTRL,
};

struct KeyItem
{
    KeyType type;
    KeyId id;
    DWORD value;
    bool enable;
    UINT nameId;
    UINT hkCtrlId;
    UINT ckCtrlId;
};

struct KeysetInfo
{
    bool enableMouseLRHide;
    bool enableEscHide;
    MouseBackAction mouseBackAction;
    std::vector<KeyItem> keys;
};

#if ENABLE_TAG
struct TagInfo
{
    bool enable;
    std::wstring keyword;
    LOGFONTW font;
    COLORREF textColor;
    COLORREF backgroundColor;
};
#endif

struct BookSource
{
    std::wstring title;
    std::string host;
    std::string queryUrl;
    int queryMethod; // 0: GET, 1: POST
    std::string queryParams; // POST params
    int queryCharset; // 0: auto, 1: utf8, 2: gbk
    // query page
    std::string bookNameXpath;
    std::string bookMainpageXpath;
    std::string bookAuthorXpath;

    // main page
    bool enableChapterPage;
    std::string chapterPageXpath;

    // chapter page
    std::string chapterTitleXpath;
    std::string chapterUrlXpath;
    bool enableChapterNext;
    std::string chapterNextUrlXpath;
    std::string chapterNextKeywordXpath;
    std::string chapterNextKeyword;

    // content page
    std::string contentXpath;
    bool enableContentNext;
    std::string contentNextUrlXpath;
    std::string contentNextKeywordXpath;
    std::string contentNextKeyword;
    int contentFilterType; // 0: disable, 1: keyword, 2: regex
    std::wstring contentFilterKeyword;

    bool operator==(const BookSource& other) const
    {
        return title == other.title
         && host == other.host
         && queryUrl == other.queryUrl
         && queryMethod == other.queryMethod
         && queryParams == other.queryParams
         && queryCharset == other.queryCharset
         && bookNameXpath == other.bookNameXpath
         && bookMainpageXpath == other.bookMainpageXpath
         && bookAuthorXpath == other.bookAuthorXpath
         && enableChapterPage == other.enableChapterPage
         && chapterPageXpath == other.chapterPageXpath
         && chapterTitleXpath == other.chapterTitleXpath
         && chapterUrlXpath == other.chapterUrlXpath
         && enableChapterNext == other.enableChapterNext
         && chapterNextUrlXpath == other.chapterNextUrlXpath
         && chapterNextKeywordXpath == other.chapterNextKeywordXpath
         && chapterNextKeyword == other.chapterNextKeyword
         && contentXpath == other.contentXpath
         && enableContentNext == other.enableContentNext
         && contentNextUrlXpath == other.contentNextUrlXpath
         && contentNextKeywordXpath == other.contentNextKeywordXpath
         && contentNextKeyword == other.contentNextKeyword
         && contentFilterType == other.contentFilterType
         && contentFilterKeyword == other.contentFilterKeyword;
    }

    bool operator!=(const BookSource& other) const
    {
        return !(*this == other);
    }
};

struct UpgradeInfo
{
    std::wstring ignoreVersion;
    unsigned long long lastCheckTime;
};

struct Data
{
    std::wstring            version;
    WindowInfo              window;
    BackgroundInfo          background;
    NetworkInfo             network;
    PagingInfo              paging;
    KeysetInfo              keyset;
    TextInfo                styles;
    LayoutConfig            layout;
    ChapterRule             chapterRule;
    UpgradeInfo             upgrade;
    WebShelfInfo            webShelf;
    std::vector<COLORREF>   customColors;
#if ENABLE_TAG
    std::vector<TagInfo>    tags;
#endif
    std::vector<BookSource> sources;
    std::vector<BookInfo>   books;
};

struct VersionInfo
{
    std::wstring version;
    std::wstring downloadurl;
    std::wstring description;
    bool force;
};

struct VersionList
{
    std::vector<VersionInfo> list;
};


/////////////////////////////////////////////
// windows message type
#ifndef _WINDEF_

#ifndef _WPARAM_DEFINED
#define _WPARAM_DEFINED
#if defined(_WIN64) || defined(_M_X64) || defined(__x86_64__)
using WPARAM = unsigned long long;
#else
using WPARAM = unsigned int;
#endif
#endif

#ifndef _LPARAM_DEFINED
#define _LPARAM_DEFINED
#if defined(_WIN64) || defined(_M_X64) || defined(__x86_64__)
using LPARAM = long long;
#else
using LPARAM = long;
#endif
#endif

#ifndef _LRESULT_DEFINED
#define _LRESULT_DEFINED
#if defined(_WIN64) || defined(_M_X64) || defined(__x86_64__)
using LRESULT = long long;
#else
using LRESULT = long;
#endif
#endif

#endif // _WINDEF_