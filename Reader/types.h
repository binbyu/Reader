#ifndef __TYPES_H__
#define __TYPES_H__

//#pragma pack(1)

#define CACHE_FILE_NAME             _T(".cache.dat")
#define ONLINE_FILE_SAVE_PATH       _T(".online\\")

#define DEFAULT_APP_WIDTH           (300)
#define DEFAULT_APP_HEIGHT          (500)

#define ENABLE_NETWORK              1
#define ENABLE_MD5                  0
#define ENABLE_TAG                  0
#define ENABLE_REALTIME_SAVE        1

#ifdef _DEBUG
#define TEST_MODEL                  1
#else
#define TEST_MODEL                  0
#endif
#define FAST_MODEL                  1

#define MAX_CHAPTER_LENGTH          256
#define MAX_MARK_COUNT              256
#define MAX_TAG_COUNT               10
#define MAX_BOOKSRC_COUNT           16

#define IDM_CUSTOM_BEGIN            (50000)

#define IDM_OPEN_BEGIN              (IDM_CUSTOM_BEGIN + 1)
#define IDM_OPEN_END                (IDM_OPEN_BEGIN + 2000)

#define IDI_SYSTRAY                 (IDM_OPEN_END + 1)
#define IDM_ST_OPEN                 (IDM_OPEN_END + 2)
#define IDM_ST_EXIT                 (IDM_OPEN_END + 3)
#define IDM_MK_DEL                  (IDM_OPEN_END + 4)
#define IDM_BS_DEL                  (IDM_OPEN_END + 5)

#if ENABLE_NETWORK
#define WM_NEW_VERSION              (WM_USER + 100)
#endif
#define WM_UPDATE_CHAPTERS          (WM_USER + 101)
#define WM_OPEN_BOOK                (WM_USER + 102)
#define WM_SYSTRAY                  (WM_USER + 103)
#define WM_BOOK_EVENT               (WM_USER + 104)
#define WM_SAVE_CACHE               (WM_USER + 105)
#define WM_TASKBAR_CREATED          (RegisterWindowMessage(_T("TaskbarCreated")))


#define ID_HOTKEY_SHOW_HIDE_WINDOW  100
#define IDT_TIMER_PAGE              102
#if ENABLE_NETWORK
#define IDT_TIMER_UPGRADE           103
#define IDT_TIMER_CHECKBOOK         104
#endif
#define IDT_TIMER_LOADING           105


typedef unsigned char       u8;
typedef unsigned int        u32;
typedef unsigned long long  u64;

#if ENABLE_MD5
typedef struct u128_t
{
    u8 data[16];
} u128_t;
#endif

typedef struct item_t
{
#if ENABLE_MD5
    u128_t md5;
#endif
    int id;
    int index; // save text current pos
    TCHAR file_name[MAX_PATH];
    int mark_size;
    int mark[MAX_MARK_COUNT]; // book mark
    int is_new;
} item_t;

typedef enum bg_image_mode_t
{
    Stretch,
    Tile,
    TileFlip
} bg_image_mode_t;

typedef enum auto_page_mode_t
{
    apm_page = 0x00,
    apm_line = 0x01,
    apm_fixed = 0x00,
    apm_count = 0x10
} auto_page_mode_t;

typedef enum wheel_speed_t
{
    ws_single_line = 0,
    ws_double_line,
    ws_three_line,
    ws_fullpage
} wheel_speed_t;

typedef struct bg_image_t
{
    BOOL enable;
    TCHAR file_name[MAX_PATH];
    int mode; // bg_image_mode_t
} bg_image_t;

typedef struct proxy_t
{
    BOOL enable;
    WCHAR addr[64];
    int port;
    WCHAR user[64];
    WCHAR pass[64];
} proxy_t;

typedef struct chapter_rule_t
{
    int rule; // 0: default, 1: keyword, 2: regex
    WCHAR keyword[256];
    WCHAR regex[256];
} chapter_rule_t;

#if ENABLE_TAG
typedef struct tagitem_t
{
    BOOL enable;
    LOGFONT font;
    u32 font_color;
    u32 bg_color;
    WCHAR keyword[64];
} tagitem_t;
#endif

typedef struct book_source_t
{
    TCHAR title[256];
    char host[1024];
    char query_url[1024];
    int query_method; // 0: GET, 1: POST
    char query_params[1024]; // POST params
    char book_name_xpath[1024];
    char book_mainpage_xpath[1024];
    char book_author_xpath[1024];
    char chapter_title_xpath[1024];
    char chapter_url_xpath[1024];
    char content_xpath[1024];
    int book_status_pos; // 0:null, 1:query_page, 2:main_page
    char book_status_xpath[1024];
    char book_status_keyword[256];
} book_sourc_t;

typedef struct header_t
{
    TCHAR version[16];
    int item_count;
    int item_id;
    RECT rect;
    LOGFONT font;
    u32 font_color;
    u32 bg_color;
    BYTE alpha;
    int line_gap;
    int paragraph_gap;
    int left_line_count;
    RECT internal_border;
    int wheel_speed; // wheel_speed_t
    int page_mode;
    int autopage_mode; // auto_page_mode_t
    bg_image_t bg_image;
    UINT uElapse;
    proxy_t proxy;
    TCHAR ingore_version[16];
    int hide_taskbar;
    int show_systray;
    int disable_lrhide;
    int word_wrap;
    int line_indent;
    u32 keyset[64];
    chapter_rule_t chapter_rule;
#if ENABLE_TAG
    tagitem_t tags[MAX_TAG_COUNT];
#endif
    BYTE meun_font_follow;
    int book_source_count;
    book_source_t book_sources[MAX_BOOKSRC_COUNT];
} header_t;

typedef struct body_t
{
    item_t items[1];
} body_t;


typedef enum type_t
{
    Unknown = 0,
    utf8,
    utf16_le,
    utf16_be,
    utf32_le,
    utf32_be
} type_t;

typedef struct upmenu_t
{
    BYTE op;        // 0: delete, 1: append, 2: insert
    HMENU hMenu;
    UINT uPosition;
    UINT uFlags;
    INT_PTR uIDNewItem;
    LPCTSTR lpNewItem;
} upmenu_t;

typedef struct window_info_t
{
    HMENU hMenu;
    HWND hStatusBar;
    BOOL bHideBorder;
    BOOL bFullScreen;

    // hide border restore data
    DWORD hbStyle;
    DWORD hbExStyle;
    RECT hbRect;

    // full screen restore data
    DWORD fsStyle;
    DWORD fsExStyle;
    RECT fsRect;
} window_info_t;

typedef struct writer_buffer_param_t
{
    unsigned char* buf;
    size_t total;
    size_t used;
} writer_buffer_param_t;

typedef void (*download_progress_cb_t)(void* param, double progress);
typedef struct writer_file_param_t
{
    FILE* fp;
    int fileSize;
    int downSize;
    download_progress_cb_t cb;
    void* param;
} writer_file_param_t;


typedef struct ol_chapter_info_t
{
    u32 index;
    u32 title_offset;
    u32 url_offset;
    u32 size;
} ol_chapter_info_t;

typedef struct ol_header_t
{
    u32 header_size;
    u32 book_name_offset;
    u32 main_page_offset;
    u32 host_offset;
    u64 update_time;
    u32 is_finished;
    u32 reserve[4]; // reserve
    u32 chapter_size;
    ol_chapter_info_t chapter_info_list[1];
} ol_header_t;

typedef struct redirect_kw_t
{
    const char* begin;
    const char* end;
} redirect_kw_t;


#endif