/**
 * @file WebDlg.cpp
 * @brief Provides a LAN Web server to sync and read books with the Legado mobile app.
 *
 * Original Author: NanoKimi (https://github.com/NanoKimi)
 * Current  Author: binbyu   (https://github.com/binbyu)
 */

#ifdef ENABLE_NETWORK
#include "WebDlg.h"
#include "resource.h"
#include "Logger.h"
#include "conv.h"
#include "WebFile.h"
#include "JsonSerialize.h"
#include "Utils.h"
#include <algorithm>

WebDlg::WebDlg(HINSTANCE hInstance, HWND hParent)
    : Dialog(hInstance, hParent, IDD_WEB)
    , m_list(nullptr)
    , m_hRequest(nullptr)
{
}

WebDlg::~WebDlg()
{
    if (m_list)
    {
        delete m_list;
        m_list = nullptr;
    }
}

std::wstring WebDlg::openFileName() const
{
    return m_fileName;
}

LRESULT WebDlg::onInitDialog()
{
    SendMessage(m_hWnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_READER)));
    SendMessage(GetDlgItem(m_hWnd, IDC_WEB_SEARCH), 0x1501 /*EM_SETCUEBANNER*/, (WPARAM)TRUE, (LPARAM)loadString(IDS_SEARCH_TIPS).c_str());

    initListView();
    initServerUrl();
    centerParent();
    return TRUE;
}

LRESULT WebDlg::onOpenButtonClick()
{
    int index = m_list->selectedItem();
    if (index < 0 || index >= (int)m_data.data.size()) return TRUE;

    WebShelfItem* item = (WebShelfItem*)m_list->itemUserData(index);
    if (!item) return TRUE;

    std::wstring savePath = Utils::appPath() + WEB_FILE_SAVE_PATH;
    if (!Utils::makeDir(savePath))
    {
        lgerror("Create web file save path failed. Error code: %lu", GetLastError());
        messageBox(IDS_OPEN_FILE_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
        return TRUE;
    }
    SetFileAttributesW(savePath.c_str(), FILE_ATTRIBUTE_HIDDEN);

    std::wstring bookName = Utils::dirtyFileName(item->name);
    if (bookName.empty()) bookName = DEFAULT_BOOK_NAME;
    std::wstring hash = Utils::fnv1a32(item->bookUrl.c_str(), item->bookUrl.size());
    std::wstring fileName = Utils::appPath() + WEB_FILE_SAVE_PATH + bookName + L"-" + hash + L".web";
    if (Utils::isFileExist(fileName))
    {
#if 0
        int ret = messageBox(IDS_FILE_ISEXIST, IDS_WARN, MB_ICONWARNING | MB_YESNOCANCEL);
        if (ret == IDCANCEL)
        {
            return TRUE;
        }
        if (ret == IDNO)
        {
            m_fileName = fileName;
            EndDialog(m_hWnd, IDOK);
            return TRUE;
        }

        Utils::removeFile(fileName);
        auto& books = Settings::instance().data().books;
        auto it = std::find_if(books.begin(), books.end(), [&fileName](const BookInfo& bi) { return bi.fileName == fileName; });
        if (it != books.end()) books.erase(it);
#else
        m_fileName = fileName;
        EndDialog(m_hWnd, IDOK);
        return TRUE;
#endif
    }

    WebFile webFile(bookName, *item);
    const WebHeader* header = webFile.header();

    FILE* fp = _wfopen(fileName.c_str(), L"wb");
    if (!fp)
    {
        lgerror("Open web file failed. Error code: %lu", GetLastError());
        messageBox(IDS_OPEN_FILE_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
        return TRUE;
    }
    if (header->headerSize != fwrite(header, 1, header->headerSize, fp))
    {
        fclose(fp);
        lgerror("Write web file failed. Error code: %lu", GetLastError());
        messageBox(IDS_WRITE_FILE_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
        return TRUE;
    }
    fclose(fp);

    m_fileName = fileName;
    EndDialog(m_hWnd, IDOK);
    return TRUE;
}

LRESULT WebDlg::onCloseButtonClick()
{
    EndDialog(m_hWnd, IDCANCEL);
    return (INT_PTR)TRUE;
}

LRESULT WebDlg::onGoButtonClick()
{
    // cancel last request
    if (m_hRequest)
    {
        https_cancel(m_hRequest);
        m_hRequest = nullptr;
        SetDlgItemTextW(m_hWnd, IDC_WEB_STATUS, L"");
        enableControls(TRUE);
        return TRUE;
    }

    std::string serverUrl = getDlgItemTextA(IDC_WEB_URL);
    if (serverUrl.empty())
    {
        messageBox(IDS_EMPTY_SERVER_URL, IDS_ERROR, MB_ICONERROR | MB_OK);
        return TRUE;
    }

    char normalized[1024];
    https_normalize_url(serverUrl.c_str(), normalized);
    if (!normalized[0])
    {
        messageBox(IDS_EMPTY_SERVER_URL, IDS_ERROR, MB_ICONERROR | MB_OK);
        return TRUE;
    }

    Settings::instance().data().webShelf.serverUrl = serverUrl;
    Settings::instance().save();

    std::string url = std::string(normalized) + "getBookshelf";

    request_t r;
    memset(&r, 0, sizeof(request_t));
    r.method = GET;
    r.url = (char*)url.c_str();
    r.param1 = m_hWnd;
    r.param2 = NULL;
    r.noproxy = 1;
    r.completer = [](request_result_t *result) -> unsigned int
    {
        HWND hWnd = (HWND)result->param1;
        if (result->errno_ == ECAN)
        {
            return 1;
        }
        if (result->errno_ != SUCC || result->status_code != 200)
        {
            PostMessage(hWnd, WM_REQCOMPLETER, 0, NULL);
            return 1;
        }
        char* html = nullptr;
        int htmlLen = conv_to_utf8(result->body, result->bodylen, &html);
        if (!html)
        {
            PostMessage(hWnd, WM_REQCOMPLETER, 0, NULL);
            return 1;
        }
        if (!PostMessage(hWnd, WM_REQCOMPLETER, htmlLen, (LPARAM)html))
        {
            conv_free(html);
            return 1;
        }
        return 0;
    };

    SetDlgItemTextW(m_hWnd, IDC_WEB_STATUS, loadString(IDS_FETCHING_SHELF).c_str());
    enableControls(FALSE);
    m_hRequest = https_request(&r);
    return TRUE;
}

LRESULT WebDlg::onRefreshButtonClick()
{
    SetDlgItemTextW(m_hWnd, IDC_WEB_SEARCH, L"");
    refreshListView();
    SetFocus(GetDlgItem(m_hWnd, IDC_WEB_SEARCH));
    return TRUE;
}

LRESULT WebDlg::onSyncButtonClick()
{
    std::string serverUrl = getDlgItemTextA(IDC_WEB_URL);
    if (serverUrl.empty())
    {
        messageBox(IDS_EMPTY_SERVER_URL, IDS_ERROR, MB_ICONERROR | MB_OK);
        return TRUE;
    }

    char normalized[1024];
    https_normalize_url(serverUrl.c_str(), normalized);
    if (!normalized[0])
    {
        messageBox(IDS_EMPTY_SERVER_URL, IDS_ERROR, MB_ICONERROR | MB_OK);
        return TRUE;
    }

    std::string url = std::string(normalized) + "vue/index.html#/";
    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
    return TRUE;
}

LRESULT WebDlg::onReqCompleter(WPARAM wParam, LPARAM lParam)
{
    int htmlLen = (int)wParam;
    char* html = (char*)lParam;

    m_hRequest = nullptr;
    enableControls(TRUE);

    if (!html || htmlLen == 0)
    {
        if (html) conv_free(html);
        SetDlgItemTextW(m_hWnd, IDC_WEB_STATUS, loadString(IDS_FETCH_SHELF_FAIL).c_str());
        messageBox(IDS_FETCH_SHELF_FAIL1, IDS_ERROR, MB_ICONERROR | MB_OK);
        return TRUE;
    }

    std::vector<uint8_t> bytes;
    bytes.assign(html, html + htmlLen);
    conv_free(html);

    WebShelfList data;
    if (!JsonSerialize::deserialize(bytes, data))
    {
        SetDlgItemTextW(m_hWnd, IDC_WEB_STATUS, loadString(IDS_FETCH_SHELF_FAIL).c_str());
        messageBox(IDS_FETCH_SHELF_FAIL2, IDS_ERROR, MB_ICONERROR | MB_OK);
        return TRUE;
    }
    SetFocus(GetDlgItem(m_hWnd, IDC_WEB_SEARCH));

    SetDlgItemTextW(m_hWnd, IDC_WEB_STATUS, loadString(IDS_FETCH_SHELF_SUCC).c_str());
    m_data = std::move(data);
    std::sort(m_data.data.begin(), m_data.data.end(), [](const WebShelfItem& a, const WebShelfItem& b){ return a.durChapterTime > b.durChapterTime; });

    refreshListView();

    return TRUE;
}

LRESULT WebDlg::onSize(int width, int height)
{
    const int BORDER = 8;

    HWND hEditUrl = GetDlgItem(m_hWnd, IDC_WEB_URL);
    HWND hButtonGo = GetDlgItem(m_hWnd, IDC_WEB_GO);
    HWND hButtonSync = GetDlgItem(m_hWnd, IDC_WEB_SYNC);
    HWND hEditSearch = GetDlgItem(m_hWnd, IDC_WEB_SEARCH);
    HWND hButtonRefresh = GetDlgItem(m_hWnd, IDC_WEB_REFRESH);
    HWND hListBooks = GetDlgItem(m_hWnd, IDC_WEB_LIST);
    HWND hStaticStatus = GetDlgItem(m_hWnd, IDC_WEB_STATUS);
    HWND hButtonOpen = GetDlgItem(m_hWnd, IDOK);
    HWND hButtonCancel = GetDlgItem(m_hWnd, IDCANCEL);

    RECT rcTemp;
    GetWindowRect(hButtonOpen, &rcTemp);
    int BTN_W = rcTemp.right - rcTemp.left;
    int BTN_H = rcTemp.bottom - rcTemp.top;

    // Row 1: URL, GO, SYNC
    int x = BORDER;
    int y = BORDER;
    int w = width - BORDER - BORDER - BTN_W - BORDER - BTN_W - BORDER;
    int h = BTN_H;
    SetWindowPos(hEditUrl, NULL, x, y, w, h, SWP_NOZORDER);
    x += w + BORDER;
    w = BTN_W;
    SetWindowPos(hButtonGo, NULL, x, y, w, h, SWP_NOZORDER);
    x += w + BORDER;
    SetWindowPos(hButtonSync, NULL, x, y, w, h, SWP_NOZORDER);

    // Row 2: SEARCH, REFRESH
    x = BORDER;
    y = BORDER + BTN_H + BORDER;
    w = width - BORDER - BORDER - (BTN_W * 2 + BORDER) - BORDER;
    h = BTN_H;
    SetWindowPos(hEditSearch, NULL, x, y, w, h, SWP_NOZORDER);
    x += w + BORDER;
    w = BTN_W;
    SetWindowPos(hButtonRefresh, NULL, x, y, w, h, SWP_NOZORDER);

    // Row 3: LIST
    x = BORDER;
    y = BORDER + (BTN_H + BORDER) * 2;
    w = width - BORDER * 2;
    h = height - y - BORDER - BTN_H - BORDER;
    SetWindowPos(hListBooks, NULL, x, y, w, h, SWP_NOZORDER);

    // Row 4: STATUS, OK, CANCEL
    x = BORDER;
    y = height - BORDER - BTN_H;
    w = width - BORDER - BORDER - BTN_W - BORDER - BTN_W - BORDER;
    h = BTN_H;
    SetWindowPos(hStaticStatus, NULL, x, y, w, h, SWP_NOZORDER);
    x = width - BORDER - BTN_W;
    w = BTN_W;
    SetWindowPos(hButtonOpen, NULL, x, y, w, h, SWP_NOZORDER);

    InvalidateRect(m_hWnd, NULL, TRUE);
    return TRUE;
}

void WebDlg::initListView()
{
    HWND hList = GetDlgItem(m_hWnd, IDC_WEB_LIST);
    m_list = new ListView(hList, this);
    m_list->addHeader(IDS_BOOK_NAME, 150);
    m_list->addHeader(IDS_AUTHOR, 90);
    m_list->addHeader(IDS_PROGRESS, 90);
    m_list->addHeader(IDS_LATEST_CHAPTER, 110);
    m_list->addHeader(IDS_LATEST_READ, 130);
    m_list->addHeader(IDS_SOURCE, 80);
}

void WebDlg::refreshListView()
{
    std::wstring filter = getDlgItemTextW(IDC_WEB_SEARCH);
    std::wstring progress;
    std::wstring latestChapter;
    std::wstring latestRead;
    std::wstring source;
    int row = 0;

    m_list->clearItems();
    for (auto& i : m_data.data)
    {
        if (!isMatched(filter, i)) continue;

        if (i.totalChapterNum > 0) progress = std::to_wstring(i.durChapterIndex + 1) + L" / " + std::to_wstring(i.totalChapterNum) + L" " + loadString(IDS_CHAPTER);
        else if (!i.durChapterTitle.empty()) progress = i.durChapterTitle;
        else progress = L"-";

        if (i.durChapterTime > 0) latestRead = Utils::timeStr(i.durChapterTime / 1000);
        else latestRead = L"-";

        if (i.origin.compare(0, 3, "loc")) source = loadString(IDS_MOBILOCAL);
        else source = loadString(IDS_NETSOURCE);

        m_list->addItem(row, { i.name, i.author, progress, latestChapter, latestRead, source }, &i);
        row++;
    }
}

void WebDlg::initServerUrl()
{
    auto serverUrl = Settings::instance().data().webShelf.serverUrl;
    if (serverUrl.empty())
    {
        SetDlgItemTextW(m_hWnd, IDC_WEB_STATUS, loadString(IDS_WEB_SERVER_TIPS).c_str());
    }
    else
    {
        wchar_t* utf16 = nullptr;
        conv_to_utf16(serverUrl.c_str(), (int)serverUrl.size(), &utf16);
        SetDlgItemTextW(m_hWnd, IDC_WEB_URL, utf16);
        conv_free(utf16);
        PostMessage(m_hWnd, WM_COMMAND, IDC_WEB_GO, 0);
    }
}

void WebDlg::enableControls(BOOL enable)
{
    EnableWindow(GetDlgItem(m_hWnd, IDC_WEB_URL), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_WEB_SYNC), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_WEB_SEARCH), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_WEB_REFRESH), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_WEB_LIST), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDOK), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDCANCEL), enable);
    if (enable)
    {
        SetDlgItemTextW(m_hWnd, IDC_WEB_GO, loadString(IDS_CONNECT).c_str());
    }
    else
    {
        SetDlgItemTextW(m_hWnd, IDC_WEB_GO, loadString(IDS_CANCEL).c_str());
    }
}

bool WebDlg::isMatched(const std::wstring& filter, const WebShelfItem& item)
{
    if (item.bookUrl.empty()) return false;
    std::wstring key = Utils::trim(filter);
    if (key.empty()) return true;

    std::wstring hay;
    hay += item.name + L"\n";
    hay += item.author + L"\n";
    hay += item.durChapterTitle + L"\n";
    hay += item.latestChapterTitle;

    size_t i, start = 0;
    for (i = 0; i < key.size(); i++) key[i] = towlower(key[i]);
    for (i = 0; i < hay.size(); i++) hay[i] = towlower(hay[i]);

    while (start <= key.size())
    {
        size_t sp = key.find(L' ', start);
        std::wstring word = (sp == std::wstring::npos) ? key.substr(start) : key.substr(start, sp - start);

        if (!word.empty() && hay.find(word) == std::wstring::npos)
            return false;

        if (sp == std::wstring::npos)
            break;
        start = sp + 1;
    }
    return true;
}

LRESULT WebDlg::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return onInitDialog();

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            return onOpenButtonClick();
        case IDCANCEL:
            return onCloseButtonClick();
        case IDC_WEB_GO:
            return onGoButtonClick();
        case IDC_WEB_SEARCH:
            if (EN_CHANGE == HIWORD(wParam)) refreshListView();
            return TRUE;
        case IDC_WEB_REFRESH:
            return onRefreshButtonClick();
        case IDC_WEB_SYNC:
            return onSyncButtonClick();
        }
        break;

    case WM_SIZE:
        return onSize(LOWORD(lParam), HIWORD(lParam));

    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* pMinMaxInfo = (MINMAXINFO*)lParam;
        pMinMaxInfo->ptMinTrackSize.x = 480; // min width
        pMinMaxInfo->ptMinTrackSize.y = 340; // min height
        break;
    }

    case WM_NOTIFY:
        if (LOWORD(wParam) == IDC_WEB_LIST)
        {
            if (m_list) return m_list->onNotify(wParam, lParam);
        }
        break;

    case WM_LIST_DBLCLK:
        return onOpenButtonClick();

    case WM_REQCOMPLETER:
        return onReqCompleter(wParam, lParam);

    case WM_DESTROY:
    {
        if (m_hRequest)
        {
            https_cancel(m_hRequest);
            m_hRequest = nullptr;
        }
        MSG msg;
        while (PeekMessage(&msg, m_hWnd, WM_REQCOMPLETER, WM_REQCOMPLETER, PM_REMOVE))
        {
            if (msg.lParam != NULL) conv_free((char*)msg.lParam);
        }
        break;
    }

    default:
        break;
    }
    return (INT_PTR)FALSE;
}
#endif