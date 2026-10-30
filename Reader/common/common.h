#pragma once

// for v141_xp toolset
#ifdef _USING_V110_SDK71_
#ifndef WM_DPICHANGED
#define WM_DPICHANGED               (0x02E0)
#endif 
#endif

#ifdef _DEBUG
#define ENABLE_TESTCASE             (1)
#define ENABLE_LOGGER               (1)
#else
#define ENABLE_TESTCASE             (0)
#define ENABLE_LOGGER               (0)
#endif
#define ENABLE_TAG                  (0)

// custom defines
#define SETTINGS_FILE_NAME          (L".settings.json")
#define ONLINE_FILE_SAVE_PATH       (L".online\\")
#define WEB_FILE_SAVE_PATH          (L".web\\")
#define LOG_FILE_NAME               (L"Reader.log")
#define LOG_BAK_FILE_NAME           (L"Reader.log.bak")
#define DEFAULT_BOOK_NAME           (L"book")
#define DEFAULT_APP_WIDTH           (300)
#define DEFAULT_APP_HEIGHT          (500)
#define MAX_CUSTOMCOLORS_COUNT      (16)
#define DEFAULT_TEXT_COLOR          (RGB(0, 0, 0))
#define DEFAULT_TEXTBG_COLOR        (RGB(255, 255, 255))
#define SOURCE_ADDRESS              (L"https://github.com/binbyu/Reader")
#define BOOKSOURCE_MANUAL_DOC       (L"https://github.com/binbyu/Reader/blob/master/doc/bs.md")
#define BOOKSOURCE_SAMPLE_FILE      ("https://raw.githubusercontent.com/binbyu/Reader/refs/heads/master/misc/bs.json")
#define VERSION_INFO_URL            ("https://raw.githubusercontent.com/binbyu/Reader/refs/heads/master/misc/version.json")
#define MAX_LOADSTRING              (256)
#define MAX_URL_LENGTH              (1024)
#define DEFAULT_DPI                 (96)
#define MIN_LAYERED_ALPHA           (0x64)
#define MIN_ALPHA                   (0x01)
#define MAX_ALPHA                   (0xff)
#define MIN_TEXT_ALPHA              (0x16)
#define MAX_TEXT_ALPHA              (0xff)
#define UNIT_STEP                   (0x05)
#define MAX_REQUEST_COUNT           (16)
#define DEFAULT_BOOK_NAME_XPATH     ("/html/head/title")
#define DEFAULT_CHAPTER_NAME_XPATH  ("//a[@href]")
#define DEFAULT_CHAPTER_URL_XPATH   ("//a/@href")
#define DEFAULT_CONTENT_XPATH       ("(//p | //div)//text()[not(ancestor::a) and not(ancestor::script) and not(ancestor::style)]")
#define CHECK_VERSION_TIMEOUT       (3600 * 24) // one day
#define CHECK_BOOK_TIMEOUT          (3600)      // one hour

// custom message ids
#define WM_USER_BEGIN               (WM_USER + 1)
#define WM_SYSTRAY                  (WM_USER_BEGIN + 1)
#define WM_SAVESETTING              (WM_USER_BEGIN + 2)
#define WM_REQCOMPLETER             (WM_USER_BEGIN + 3)
#define WM_LIST_SELECTIONSETTLED    (WM_USER_BEGIN + 4)
#define WM_LIST_CHECKED             (WM_USER_BEGIN + 5)
#define WM_LIST_SELECTED            (WM_USER_BEGIN + 6)
#define WM_LIST_DBLCLK              (WM_USER_BEGIN + 7)
#define WM_LIST_MOVEUP              (WM_USER_BEGIN + 8)
#define WM_LIST_MOVEDOWN            (WM_USER_BEGIN + 9)
#define WM_LIST_MOVETOP             (WM_USER_BEGIN + 10)
#define WM_LIST_MOVEBOTTOM          (WM_USER_BEGIN + 11)
#define WM_LIST_ADD                 (WM_USER_BEGIN + 12)
#define WM_LIST_DELETE              (WM_USER_BEGIN + 13)
#define WM_LIST_EDIT                (WM_USER_BEGIN + 14)
#define WM_LIST_CLEAR               (WM_USER_BEGIN + 15)
#define WM_LIST_OPEN                (WM_USER_BEGIN + 16)
#define WM_LIST_PREVIEW             (WM_USER_BEGIN + 17)
#define WM_NEWVERSION               (WM_USER_BEGIN + 18)
#define WM_BOOKSTATECHANGED         (WM_USER_BEGIN + 19)
#define WM_BOOKEVENT                (WM_USER_BEGIN + 20)
#define WM_REDRAW                   (WM_USER_BEGIN + 21)
#define WM_UPDATEBOOKS              (WM_USER_BEGIN + 22)
#define WM_UPDATECHAPTERS           (WM_USER_BEGIN + 23)
#define WM_UPDATEMARKS              (WM_USER_BEGIN + 24)
#define WM_USER_END                 (WM_USER_BEGIN + 0xff)

// custom menu ids
#define IDM_USER_BEGIN              (0x9000)
#define IDM_OPEN_BEGIN              (IDM_USER_BEGIN + 1)
#define IDM_OPEN_END                (IDM_OPEN_BEGIN + 0x2000)
#define IDM_SYSTRAY                 (IDM_OPEN_END + 1)
#define IDM_ST_OPEN                 (IDM_OPEN_END + 2)
#define IDM_ST_EXIT                 (IDM_OPEN_END + 3)
#define IDM_ADD                     (IDM_OPEN_END + 4)
#define IDM_DELETE                  (IDM_OPEN_END + 5)
#define IDM_EDIT                    (IDM_OPEN_END + 6)
#define IDM_MOVEUP                  (IDM_OPEN_END + 7)
#define IDM_MOVEDOWN                (IDM_OPEN_END + 8)
#define IDM_MOVETOP                 (IDM_OPEN_END + 9)
#define IDM_MOVEBOTTOM              (IDM_OPEN_END + 10)
#define IDM_PREVIEW                 (IDM_OPEN_END + 11)
#define IDM_USER_END                (IDM_USER_BEGIN + 0x3000)

// custom timer ids
#define IDT_USER_BEGIN              (0x400)
#define IDT_UPGRADE                 (IDT_USER_BEGIN + 1)
#define IDT_CHECKBOOK               (IDT_USER_BEGIN + 2)
#define IDT_PLAYLOADING             (IDT_USER_BEGIN + 3)
#define IDT_SAVESETTING             (IDT_USER_BEGIN + 4)
#define IDT_AUTOPAGING              (IDT_USER_BEGIN + 5)
#define IDT_BOOKSYNCPROGRESS        (IDT_USER_BEGIN + 6)
#define IDT_BOOKSAVEFILE            (IDT_USER_BEGIN + 7)
#define IDT_BOOKIDLEFETCH           (IDT_USER_BEGIN + 8)
#define IDT_USER_END                (IDT_USER_BEGIN + 0xff)
