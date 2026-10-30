#include "Settings.h"
#include "Utils.h"
#include "Logger.h"
#include "DpiAwareness.h"
#include "JsonSerialize.h"
#include "resource.h"
#include "framework.h"
#include "common.h"

Settings& Settings::instance()
{
    static Settings _inst;
    return _inst;
}

void Settings::setHwnd(HWND hWnd)
{
    m_hWnd = hWnd;
}

void Settings::save(int delayms)
{
    if (delayms <= 0)
    {
        saveReal();
    }
    else
    {
        if (m_hWnd)
        {
            KillTimer(m_hWnd, IDT_SAVESETTING);
            SetTimer(m_hWnd, IDT_SAVESETTING, delayms, NULL);
        }
        else
        {
            saveReal();
        }
    }
}

Data& Settings::data()
{
    return m_data;
}

void Settings::restoreDefault()
{
    defaultData(m_data);
    toPhysical(m_data);
}

Settings::Settings()
    : m_hWnd(nullptr)
    , m_stop(false)
{
    load();
    m_thread = std::thread(&Settings::writer, this);
}

Settings::~Settings()
{
    flush();
}

void Settings::saveReal()
{
    std::shared_ptr<Data> data = std::make_shared<Data>(m_data);
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_pending = data;
    }
    m_cv.notify_one();
}

void Settings::load()
{
    defaultData(m_data);
    FILE* fp = _wfopen(fileName().c_str(), L"rb");
    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        int size = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        std::vector<uint8_t> bytes(size);
        fread(bytes.data(), 1, size, fp);
        fclose(fp);

        if (JsonSerialize::deserialize(bytes, m_data))
        {
            m_bytes = std::move(bytes);
        }
        else
        {
            defaultData(m_data);
        }
        m_data.version = Utils::appVersion();
        fixedWindowInfo(m_data.window);
    }
    toPhysical(m_data);
    ASSERT(m_data.customColors.size() == MAX_CUSTOMCOLORS_COUNT);
    ASSERT(m_data.keyset.keys.size() == (int)KeyId::MAX_COUNT);
}

void Settings::flush()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pending = std::make_shared<Data>(m_data);
        m_stop = true;
        m_cv.notify_one();
    }
    if (m_thread.joinable())
    {
        m_thread.join();
    }
}

void Settings::writer()
{
    while (true)
    {
        std::shared_ptr<Data> data;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] { return m_pending || m_stop; });
            if (m_stop && !m_pending)
            {
                break;
            }

            if (m_pending)
            {
                data = m_pending;
                m_pending = nullptr;
            }
        }

        if (data)
        {
            toLogical(*data);
            std::vector<uint8_t> bytes;
            if (JsonSerialize::enserialize(*data, bytes))
            {
                if (bytes != m_bytes)
                {
                    SetFileAttributesW(fileName().c_str(), FILE_ATTRIBUTE_NORMAL);

                    FILE *fp = _wfopen(fileName().c_str(), L"wb");
                    if (fp)
                    {
                        if (bytes.size() != fwrite(bytes.data(), 1, bytes.size(), fp))
                        {
                            DWORD error = GetLastError();
                            lgerror("Write setting file failed. Error code: %lu", error);
                            notify(error);
                        }
                        fclose(fp);

                        SetFileAttributesW(fileName().c_str(), FILE_ATTRIBUTE_HIDDEN);
                        m_bytes = std::move(bytes);
                    }
                    else
                    {
                        DWORD error = GetLastError();
                        lgerror("Open setting file failed. Error code: %lu", error);
                        notify(error);
                    }
                }
            }
        }
    }
}

void Settings::notify(DWORD error)
{
    static int tipsCount = 3;
    if (m_hWnd && error != ERROR_SUCCESS)
    {
        if (tipsCount > 0)
        {
            tipsCount--;
            PostMessage(m_hWnd, WM_SAVESETTING, error, (LPARAM)&tipsCount);
        }
    }
}

void Settings::defaultData(Data& data)
{
    int logicalWidth = GetSystemMetrics(SM_CXSCREEN);
    int logicalHeight = GetSystemMetrics(SM_CYSCREEN);
    DpiAwareness::instance().physicalToLogical(logicalWidth, logicalWidth);
    DpiAwareness::instance().physicalToLogical(logicalHeight, logicalHeight);

    data.version = Utils::appVersion();
    data.window.mode = DisplayMode::NORMAL;
    data.window.placement.length = sizeof(WINDOWPLACEMENT);
    data.window.placement.flags = 0;
    data.window.placement.showCmd = SW_SHOWNORMAL;
    data.window.placement.ptMinPosition.x = -1;
    data.window.placement.ptMinPosition.y = -1;
    data.window.placement.ptMaxPosition.x = -1;
    data.window.placement.ptMaxPosition.y = -1;
    data.window.placement.rcNormalPosition.left = (logicalWidth - DEFAULT_APP_WIDTH) / 2;
    data.window.placement.rcNormalPosition.top = (logicalHeight - DEFAULT_APP_HEIGHT) / 2;
    data.window.placement.rcNormalPosition.right = data.window.placement.rcNormalPosition.left + DEFAULT_APP_WIDTH;
    data.window.placement.rcNormalPosition.bottom = data.window.placement.rcNormalPosition.top + DEFAULT_APP_HEIGHT;
    data.window.borderlessRect = data.window.placement.rcNormalPosition;
    data.window.style = WS_OVERLAPPEDWINDOW;
    data.window.exStyle = WS_EX_ACCEPTFILES | WS_EX_LAYERED;
    data.window.hideTaskbar = false;
    data.window.minToTray = false;
    data.window.alpha = 255;
    data.window.textAlpha = 255;

    data.background.enableImage = false;
    data.background.fileName = L"";
    data.background.mode = WrapMode::TILEFLIP;
    data.background.color = RGB(255, 255, 255);

    data.network.enableProxy = false;
    data.network.port = 0;
    data.network.addr = L"";
    data.network.user = L"";
    data.network.pass = L"";
    data.network.adapterName = L"";

    data.paging.mouseMode = MousePageMode::CLICK_ANYWHERE;
    data.paging.scrollMode = ScrollMode::LINE;
    data.paging.preserveLines = 0;
    data.paging.autoMode = AutoPageMode::PAGE_FLIP;
    data.paging.intervalMode = AutoPageIntervalMode::FIXED_TIME;
    data.paging.intervalValue = 5000;

    defaultKeys(data.keyset);

    data.styles.unified = true;
    data.styles.viewFont = false;
    data.styles.textColor = DEFAULT_TEXT_COLOR;
    data.styles.titleColor = DEFAULT_TEXT_COLOR;
    defaultFont(data.styles.textFont);
    defaultFont(data.styles.titleFont);

    data.layout.borderL = 2;
    data.layout.borderT = 2;
    data.layout.borderR = 2;
    data.layout.borderB = 2;
    data.layout.charGap = 2;
    data.layout.lineGap = 5;
    data.layout.paragraphGap = 15;
    data.layout.indent = true;
    data.layout.noBlansLine = true;
    data.layout.wordWrap = false;
    data.layout.splitChapter = false;
    data.layout.alignment = LayoutConfig::CENTER;

    data.chapterRule.rule = ChapterRule::Rule::DEFAULT;
    data.chapterRule.keyword = L"";
    data.chapterRule.regex = L"";

    data.upgrade.ignoreVersion = L"";
    data.upgrade.lastCheckTime = 0;

    data.customColors.resize(MAX_CUSTOMCOLORS_COUNT);
    for (int i = 0; i < MAX_CUSTOMCOLORS_COUNT; i++)
    {
        data.customColors[i] = RGB(255, 255, 255);
    }
}

void Settings::defaultKeys(KeysetInfo& keyset)
{
    keyset.enableMouseLRHide = false;
    keyset.enableEscHide = true;
    keyset.mouseBackAction = MouseBackAction::NOTHING;
    keyset.keys.clear();
    keyset.keys.push_back({ KeyType::HOTKEY,   KeyId::HIDE_WIN,        MAKEWORD('H', MOD_ALT),                           true, IDS_HIDE_SHOW_WINDOW, IDC_HK_HIDE,            IDC_CHECK_HIDE            });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::HIDE_BORDER,     MAKEWORD(VK_F12, 0),                              true, IDS_HIDE_SHOW_BORDER, IDC_HK_BORDER,          IDC_CHECK_BORDER          });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::FULL_SCREEN,     MAKEWORD(VK_F11, 0),                              true, IDS_FULLSRCEEN,       IDC_HK_FULLSCREEN,      IDC_CHECK_FULLSCREEN      });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::TOP_MOST,        MAKEWORD('T', MOD_CONTROL),                       true, IDS_TOPMOST,          IDC_HK_TOP,             IDC_CHECK_TOP             });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::OPEN_FILE,       MAKEWORD('O', MOD_CONTROL),                       true, IDS_OPEN_FILE,        IDC_HK_OPEN,            IDC_CHECK_OPEN            });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::ADD_MARK,        MAKEWORD('M', MOD_CONTROL),                       true, IDS_ADD_BOOKMARK,     IDC_HK_ADDMARK,         IDC_CHECK_ADDMARK         });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::AUTO_PAGE,       MAKEWORD(VK_SPACE, 0),                            true, IDS_AUTO_PAGE,        IDC_HK_AUTOPAGE,        IDC_CHECK_AUTOPAGE        });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::SEARCH,          MAKEWORD('F', MOD_CONTROL),                       true, IDS_TEXT_SEARCH,      IDC_HK_SEARCH,          IDC_CHECK_SEARCH          });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::SEEK,            MAKEWORD('G', MOD_CONTROL),                       true, IDS_PROGRESS_JUMP,    IDC_HK_JUMP,            IDC_CHECK_JUMP            });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::EDIT_MODE,       MAKEWORD('E', MOD_CONTROL),                       true, IDS_EDIT_MODE,        IDC_HK_EDIT,            IDC_CHECK_EDIT            });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::PAGE_UP,         MAKEWORD(VK_LEFT, 0),                             true, IDS_PAGE_UP,          IDC_HK_PAGEUP,          IDC_CHECK_PAGEUP          });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::PAGE_DOWN,       MAKEWORD(VK_RIGHT, 0),                            true, IDS_PAGE_DOWN,        IDC_HK_PAGEDOWN,        IDC_CHECK_PAGEDOWN        });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::LINE_UP,         MAKEWORD(VK_UP, 0),                               true, IDS_LINE_UP,          IDC_HK_LINEUP,          IDC_CHECK_LINEUP          });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::LINE_DOWN,       MAKEWORD(VK_DOWN, 0),                             true, IDS_LINE_DOWN,        IDC_HK_LINEDOWN,        IDC_CHECK_LINEDOWN        });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::CHAPTER_UP,      MAKEWORD(VK_LEFT, MOD_CONTROL),                   true, IDS_CHAPTER_UP,       IDC_HK_CHAPTERUP,       IDC_CHECK_CHAPTERUP       });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::CHAPTER_DOWN,    MAKEWORD(VK_RIGHT, MOD_CONTROL),                  true, IDS_CHAPTER_DOWN,     IDC_HK_CHAPTERDOWN,     IDC_CHECK_CHAPTERDOWN     });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::ZOOM_IN,         MAKEWORD(VK_OEM_PLUS, MOD_CONTROL),               true, IDS_FONT_ZOOMIN,      IDC_HK_FONTZOOMIN,      IDC_CHECK_FONTZOOMIN      });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::ZOOM_OUT,        MAKEWORD(VK_OEM_MINUS, MOD_CONTROL),              true, IDS_FONT_ZOOMOUT,     IDC_HK_FONTZOOMOUT,     IDC_CHECK_FONTZOOMOUT     });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::REFRESH_CHAPTER, MAKEWORD(VK_F5, 0),                               true, IDS_REFRESH_CHAPTER,  IDC_HK_REFRESH_CHAPTER, IDC_CHECK_REFRESH_CHAPTER });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::REFRESH_CONTENT, MAKEWORD(VK_F5, MOD_CONTROL),                     true, IDS_REFRESH_CONTENT,  IDC_HK_REFRESH_CONTENT, IDC_CHECK_REFRESH_CONTENT });
    keyset.keys.push_back({ KeyType::SHORTCUT, KeyId::MOUSE_HIDE,      MAKEWORD('P', MOD_CONTROL | MOD_SHIFT | MOD_ALT), true, IDS_MOUSE_HIDE,       IDC_HK_MOUSEHIDE,       IDC_CHECK_MOUSEHIDE       });
}

void Settings::defaultFont(LOGFONTW& font)
{
    const int POINTSIZE = 11;

    memset(&font, 0, sizeof(LOGFONTW));
    font.lfHeight         = -MulDiv(POINTSIZE, /*DpiAwareness::instance().dpi()*/ DEFAULT_DPI, 72); // Logic Font for settings
    font.lfWidth          = 0;
    font.lfEscapement     = 0;
    font.lfOrientation    = 0;
    font.lfWeight         = FW_NORMAL;
    font.lfItalic         = FALSE;
    font.lfUnderline      = FALSE;
    font.lfStrikeOut      = FALSE;
    font.lfCharSet        = DEFAULT_CHARSET;
    font.lfOutPrecision   = OUT_OUTLINE_PRECIS;
    font.lfClipPrecision  = CLIP_DEFAULT_PRECIS;
    font.lfQuality        = CLEARTYPE_QUALITY;
    font.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;

    LANGID langId      = GetUserDefaultUILanguage();
    WORD   primaryLang = PRIMARYLANGID(langId);
    WORD   subLang     = SUBLANGID(langId);
    if (primaryLang == LANG_CHINESE)
    {
        if (subLang == SUBLANG_CHINESE_TRADITIONAL || subLang == SUBLANG_CHINESE_HONGKONG)
            wcscpy_s(font.lfFaceName, LF_FACESIZE, L"\u5fae\u8edf\u6b63\u9ed1\u9ad4"); // Microsoft JhengHei
        else wcscpy_s(font.lfFaceName, LF_FACESIZE, L"\u5fae\u8f6f\u96c5\u9ed1"); // Microsoft YaHei
    }
    else wcscpy_s(font.lfFaceName, LF_FACESIZE, L"Segoe UI");
}

void Settings::toLogical(Data& data)
{
#if 1 // placement is always logical
    DpiAwareness::instance().physicalToLogical(data.window.placement, data.window.placement);
#endif
    DpiAwareness::instance().physicalToLogical(data.window.borderlessRect, data.window.borderlessRect);
#if 0 // will recalc in Layout
    DpiAwareness::instance().physicalToLogical(data.layout.borderL, data.layout.borderL);
    DpiAwareness::instance().physicalToLogical(data.layout.borderR, data.layout.borderR);
    DpiAwareness::instance().physicalToLogical(data.layout.borderT, data.layout.borderT);
    DpiAwareness::instance().physicalToLogical(data.layout.borderB, data.layout.borderB);
    DpiAwareness::instance().physicalToLogical(data.layout.charGap, data.layout.charGap);
    DpiAwareness::instance().physicalToLogical(data.layout.lineGap, data.layout.lineGap);
    DpiAwareness::instance().physicalToLogical(data.layout.paragraphGap, data.layout.paragraphGap);
#endif
    DpiAwareness::instance().physicalToLogical(data.styles.textFont, data.styles.textFont);
    DpiAwareness::instance().physicalToLogical(data.styles.titleFont, data.styles.titleFont);
#if ENABLE_TAG
    for (auto& tag : data.tags)
    {
        DpiAwareness::instance().physicalToLogical(tag.font, tag.font);
    }
#endif
}

void Settings::toPhysical(Data& data)
{
#if 1 // placement is always logical
    DpiAwareness::instance().logicalToPhysical(data.window.placement, data.window.placement);
#endif
    DpiAwareness::instance().logicalToPhysical(data.window.borderlessRect, data.window.borderlessRect);
#if 0 // will recalc in Layout
    DpiAwareness::instance().logicalToPhysical(data.layout.borderL, data.layout.borderL);
    DpiAwareness::instance().logicalToPhysical(data.layout.borderR, data.layout.borderR);
    DpiAwareness::instance().logicalToPhysical(data.layout.borderT, data.layout.borderT);
    DpiAwareness::instance().logicalToPhysical(data.layout.borderB, data.layout.borderB);
    DpiAwareness::instance().logicalToPhysical(data.layout.charGap, data.layout.charGap);
    DpiAwareness::instance().logicalToPhysical(data.layout.lineGap, data.layout.lineGap);
    DpiAwareness::instance().logicalToPhysical(data.layout.paragraphGap, data.layout.paragraphGap);
#endif
    DpiAwareness::instance().logicalToPhysical(data.styles.textFont, data.styles.textFont);
    DpiAwareness::instance().logicalToPhysical(data.styles.titleFont, data.styles.titleFont);
#if ENABLE_TAG
    for (auto& tag : data.tags)
    {
        DpiAwareness::instance().logicalToPhysical(tag.font, tag.font);
    }
#endif
}

void Settings::fixedWindowInfo(WindowInfo& window)
{
    if (window.placement.showCmd == SW_HIDE || window.placement.showCmd == SW_MINIMIZE || window.placement.showCmd == SW_SHOWMINIMIZED)
    {
        window.placement.showCmd = SW_NORMAL;
    }
    window.placement.flags &= ~WPF_SETMINPOSITION;

    auto fixRect = [](const RECT& rcScreen, RECT& rc)
    {
        if (rc.right <= rc.left)
        {
            rc.right = rc.left + DEFAULT_APP_WIDTH;
        }
        if (rc.bottom <= rc.top)
        {
            rc.bottom = rc.top + DEFAULT_APP_HEIGHT;
        }
        if (rc.left < rcScreen.left)
        {
            rc.left = rcScreen.left;
        }
        if (rc.top < rcScreen.top)
        {
            rc.top = rcScreen.top;
        }
        if (rc.right > rcScreen.right)
        {
            rc.right = rcScreen.right;
        }
        if (rc.bottom > rcScreen.bottom)
        {
            rc.bottom = rcScreen.bottom;
        }
    };

    RECT rcScreen;
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rcScreen, 0);
    fixRect(rcScreen, window.placement.rcNormalPosition);
    fixRect(rcScreen, window.borderlessRect);
}

std::wstring Settings::fileName()
{
    return Utils::appPath() + SETTINGS_FILE_NAME;
}