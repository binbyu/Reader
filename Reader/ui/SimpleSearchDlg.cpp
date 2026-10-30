#ifdef ENABLE_NETWORK
#include "SimpleSearchDlg.h"
#include "resource.h"
#include "conv.h"
#include "HtmlParser.h"
#include "Settings.h"
#include "Logger.h"
#include "OLFile.h"
#include "Utils.h"
#include <regex>

SimpleSearchDlg::SimpleSearchDlg(HINSTANCE hInstance, HWND hParent)
    : Dialog(hInstance, hParent, IDD_SIMPLESEARCH)
    , m_list(nullptr)
    , m_hRequest(nullptr)
    , m_state(State::IDLE)
    , m_filter(nullptr)
{
}

SimpleSearchDlg::~SimpleSearchDlg()
{
    if (m_filter)
    {
        delete m_filter;
        m_filter = nullptr;
    }
    if (m_list)
    {
        delete m_list;
        m_list = nullptr;
    }
}

std::wstring SimpleSearchDlg::openFileName() const
{
    return m_fileName;
}

LRESULT SimpleSearchDlg::onInitDialog()
{
    HWND hList = GetDlgItem(m_hWnd, IDC_LIST_CHAPTER);
    m_list = new ListView(hList, this);
    m_list->setCheckBox(true);
    m_list->setContextMenuType(CONTEXT_MENU_PREVIEW | CONTEXT_MENU_MOVE);
    m_list->addHeader(IDS_TITLE, 150);

#ifndef _USING_V110_SDK71_
    Edit_SetExtendedStyle(GetDlgItem(m_hWnd, IDC_EDIT_CONTENT), ES_EX_ALLOWEOL_ALL, ES_EX_ALLOWEOL_ALL);
#endif

    centerParent();
    return TRUE;
}

LRESULT SimpleSearchDlg::onCloseButtonClick()
{
    //EndDialog(m_hWnd, IDCANCEL);
    PostMessage(m_hParentWnd, WM_CLOSE, 0, 0);
    return (INT_PTR)TRUE;
}

LRESULT SimpleSearchDlg::onParseButtonClick()
{
    if (m_hRequest)
    {
        https_cancel(m_hRequest);
        m_hRequest = nullptr;
        m_state = State::IDLE;
        enableControls();
        return TRUE;
    }

    std::wstring wurl = getDlgItemTextW(IDC_EDIT_URL);
    if (wurl.empty())
    {
        messageBox(IDS_EMPTY_URL, IDS_TIPS, MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_URL));
        return TRUE;
    }
    m_url = toUtf8(wurl);

    request_t req;
    memset(&req, 0, sizeof(request_t));
    req.method = GET;
    req.url = (char *)m_url.c_str();
    req.param1 = m_hWnd;
    req.param2 = NULL;
    req.completer = completer;

    std::wstring filter = getDlgItemTextW(IDC_EDIT_FILTER);
    if (m_filter)
    {
        delete m_filter;
        m_filter = nullptr;
    }
    if (!filter.empty())
    {
        try
        {
            m_filter = new std::wregex(filter, std::regex::optimize);
        }
        catch (...)
        {
            m_filter = nullptr;
            messageBox(IDS_INVALID_REGEX, IDS_WARN, MB_OK | MB_ICONWARNING);
            SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_FILTER));
            return false;
        }
    }

    m_chapters.clear();
    m_content.clear();
    m_state = State::PARSING;
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CONTENT, L"");
    SetDlgItemTextW(m_hWnd, IDC_EDIT_NAME, L"");
    buildListView(0);
    enableControls();
    m_hRequest = https_request(&req);
    return (INT_PTR)TRUE;
}

LRESULT SimpleSearchDlg::onFilterButtonClick()
{
    std::wstring filter = getDlgItemTextW(IDC_EDIT_FILTER);

    if (m_filter)
    {
        delete m_filter;
        m_filter = nullptr;
    }

    std::wregex* newFilter = nullptr;
    if (!filter.empty())
    {
        try
        {
            newFilter = new std::wregex(filter, std::regex::optimize);
        }
        catch (...)
        {
            messageBox(IDS_INVALID_REGEX, IDS_WARN, MB_OK | MB_ICONWARNING);
            SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_FILTER));
            return TRUE;
        }
    }
    m_filter = newFilter;
    buildListView(0);
    return TRUE;
}

LRESULT SimpleSearchDlg::onOpenBookButtonClick()
{
    Chapters chapters;
    int cnt = m_list->itemCount();
    for (int i = 0; i < cnt; i++)
    {
        int idx = (int)(INT_PTR)m_list->itemUserData(i);
        if (idx >= 0 && idx < (int)m_chapters.size())
        {
            ChapterItem& item = m_chapters[idx];
            if (item.checked)
            {
                Chapter c;
                c.index = -1;
                c.textLen = 0;
                c.titleLineLen = (int)item.title.size();
                c.title = item.title;
                c.url = item.url;
                chapters.push_back(c);
            }
        }
    }

    if (m_url.empty())
    {
        messageBox(IDS_NOT_PARSE, IDS_WARN, MB_OK | MB_ICONWARNING);
        return TRUE;
    }
    if (chapters.empty())
    {
        messageBox(IDS_CHAPTER_EMPTY, IDS_WARN, MB_OK | MB_ICONWARNING);
        return TRUE;
    }
    if (m_content.empty())
    {
        messageBox(IDS_CONTENT_EMPTY, IDS_WARN, MB_OK | MB_ICONWARNING);
        return TRUE;
    }
    std::wstring bookName = getDlgItemTextW(IDC_EDIT_NAME);
    if (bookName.empty())
    {
        messageBox(IDS_BOOKNAME_EMPTY, IDS_WARN, MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_NAME));
        return TRUE;
    }
    if (IDYES != messageBox(IDS_CONFIM_BOOKNAME, IDS_TIPS, MB_ICONQUESTION | MB_YESNO))
    {
        SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_NAME));
        return TRUE;
    }

    std::wstring savePath = Utils::appPath() + ONLINE_FILE_SAVE_PATH;
    if (!Utils::makeDir(savePath))
    {
        lgerror("Create online file save path failed. Error code: %lu", GetLastError());
        messageBox(IDS_OPEN_FILE_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
        return TRUE;
    }
    SetFileAttributesW(savePath.c_str(), FILE_ATTRIBUTE_HIDDEN);

    bookName = Utils::dirtyFileName(bookName);
    if (bookName.empty()) bookName = DEFAULT_BOOK_NAME;
    std::wstring fileName = Utils::appPath() + ONLINE_FILE_SAVE_PATH + bookName + L".ol";
    if (Utils::isFileExist(fileName))
    {
        int ret = messageBox(IDS_FILE_ISEXIST, IDS_WARN, MB_ICONWARNING | MB_YESNOCANCEL);
        if (ret == IDCANCEL)
        {
            return TRUE;
        }
        if (ret == IDNO)
        {
            m_fileName = fileName;
            //EndDialog(m_hWnd, IDOK);
            PostMessage(m_hParentWnd, WM_CLOSE, 0, 0);
            return TRUE;
        }

        Utils::removeFile(fileName);
        auto& books = Settings::instance().data().books;
        auto it = std::find_if(books.begin(), books.end(), [&fileName](const BookInfo& bi) { return bi.fileName == fileName; });
        if (it != books.end()) books.erase(it);
    }

    url_parts_t parts;
    https_parse_url_parts(m_url.c_str(), &parts);
    std::string host = std::string(parts.protocol) + "://" + std::string(parts.host);

    OLFile olFile(bookName, m_url, host, chapters, 1, 1);
    const OLHeader* header = olFile.header();

    FILE* fp = _wfopen(fileName.c_str(), L"wb");
    if (!fp)
    {
        lgerror("Open online file failed. Error code: %lu", GetLastError());
        messageBox(IDS_OPEN_FILE_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
        return TRUE;
    }
    if (header->headerSize != fwrite(header, 1, header->headerSize, fp))
    {
        fclose(fp);
        lgerror("Write online file failed. Error code: %lu", GetLastError());
        messageBox(IDS_WRITE_FILE_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
        return TRUE;
    }
    fclose(fp);

    m_fileName = fileName;
    //EndDialog(m_hWnd, IDOK);
    PostMessage(m_hParentWnd, WM_CLOSE, 0, 0);
    return TRUE;
}

LRESULT SimpleSearchDlg::onSize(int width, int height)
{
    const int BORDER = 8;

    HWND hStaticURL = GetDlgItem(m_hWnd, IDC_STATIC_URL);
    HWND hEditURL = GetDlgItem(m_hWnd, IDC_EDIT_URL);
    HWND hButtonParse = GetDlgItem(m_hWnd, IDC_BUTTON_PARSE);
    HWND hStaticName = GetDlgItem(m_hWnd, IDC_STATIC_NAME);
    HWND hEditName = GetDlgItem(m_hWnd, IDC_EDIT_NAME);
    HWND hStaticFilter = GetDlgItem(m_hWnd, IDC_STATIC_FILTER);
    HWND hEditFilter = GetDlgItem(m_hWnd, IDC_EDIT_FILTER);
    HWND hButtonFilter = GetDlgItem(m_hWnd, IDC_BUTTON_FILTER);
    HWND hButtonOpen = GetDlgItem(m_hWnd, IDOK);
    HWND hStaticChapter = GetDlgItem(m_hWnd, IDC_STATIC_CHAPTER);
    HWND hListChapter = GetDlgItem(m_hWnd, IDC_LIST_CHAPTER);
    HWND hStaticContent = GetDlgItem(m_hWnd, IDC_STATIC_CONTENT);
    HWND hEditContent = GetDlgItem(m_hWnd, IDC_EDIT_CONTENT);
    

    RECT rcTemp;
    GetWindowRect(hButtonOpen, &rcTemp);
    int BTN_W = rcTemp.right - rcTemp.left;
    int BTN_H = rcTemp.bottom - rcTemp.top;
    GetWindowRect(hStaticURL, &rcTemp);
    int STC_W = rcTemp.right - rcTemp.left;
    int STC_H = rcTemp.bottom - rcTemp.top;
    GetWindowRect(hListChapter, &rcTemp);

    int x = BORDER;
    int y = BORDER + ((BTN_H - STC_H) / 2);
    int w = STC_W;
    int h = STC_H;
    SetWindowPos(hStaticURL, NULL, x, y, w, h, SWP_NOZORDER);
    x += w + BORDER;
    y = BORDER;
    w = width - x - BORDER - BTN_W - BORDER;
    h = BTN_H;
    SetWindowPos(hEditURL, NULL, x, y, w, h, SWP_NOZORDER);
    x += w + BORDER;
    y = BORDER;
    w = BTN_W;
    h = BTN_H;
    SetWindowPos(hButtonParse, NULL, x, y, w, h, SWP_NOZORDER);
    x = BORDER;
    y = BORDER + BTN_H + BORDER + ((BTN_H - STC_H) / 2);
    w = STC_W;
    h = STC_H;
    SetWindowPos(hStaticName, NULL, x, y, w, h, SWP_NOZORDER);
    x += w + BORDER;
    y = y;
    w = width - x - BORDER - BTN_W - BORDER;
    h = BTN_H;
    SetWindowPos(hEditName, NULL, x, y, w, h, SWP_NOZORDER);
    x += w + BORDER;
    y = y;
    w = BTN_W;
    h = BTN_H;
    SetWindowPos(hButtonOpen, NULL, x, y, w, h, SWP_NOZORDER);

    x = BORDER;
    y = (BORDER + BTN_H) * 2 + BORDER + ((BTN_H - STC_H) / 2);
    w = STC_W;
    h = STC_H;
    SetWindowPos(hStaticFilter, NULL, x, y, w, h, SWP_NOZORDER);
    x += w + BORDER;
    y = y;
    w = width - x - BORDER - BTN_W - BORDER;
    h = BTN_H;
    SetWindowPos(hEditFilter, NULL, x, y, w, h, SWP_NOZORDER);
    x += w + BORDER;
    y = y;
    w = BTN_W;
    h = BTN_H;
    SetWindowPos(hButtonFilter, NULL, x, y, w, h, SWP_NOZORDER);
    
    x = BORDER;
    y = (BORDER + BTN_H) * 3 + BORDER + ((BTN_H - STC_H) / 2);
    w = STC_W;
    h = STC_H;
    SetWindowPos(hStaticChapter, NULL, x, y, w, h, SWP_NOZORDER);
    x = BORDER;
    y += h + (BORDER / 2);
    w = (width - BORDER * 3) / 4;
    h = height - BORDER - y;
    SetWindowPos(hListChapter, NULL, x, y, w, h, SWP_NOZORDER);

    x += w + BORDER;
    y = (BORDER + BTN_H) * 3 + BORDER + ((BTN_H - STC_H) / 2);
    w = STC_W;
    h = STC_H;
    SetWindowPos(hStaticContent, NULL, x, y, w, h, SWP_NOZORDER);
    x = x;
    y += h + (BORDER / 2);
    w = width - BORDER - x;
    h = height - BORDER - y;
    SetWindowPos(hEditContent, NULL, x, y, w, h, SWP_NOZORDER);
    return TRUE;
}

LRESULT SimpleSearchDlg::onReqCompleter(WPARAM wParam, LPARAM lParam)
{
    int htmlLen = (int)wParam;
    char* html = (char*)lParam;

    m_hRequest = nullptr;
    if (m_state == State::PARSING)
    {
        m_state = State::IDLE;
        if (!html)
        {
            enableControls();
            // htmlLen use for error code
            if (htmlLen == 0) messageBox(IDS_NETWORK_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
            else if (htmlLen < 0) messageBox(IDS_PARSE_FAIL2, IDS_ERROR, MB_ICONERROR | MB_OK);
            else messageBoxFmt(IDS_REQUEST_ERROR, IDS_ERROR, MB_ICONERROR | MB_OK, htmlLen);
            return (INT_PTR)TRUE;
        }

        std::vector<std::string> titles;
        std::vector<std::string> urls;
        std::vector<std::string> texts;
        ParserContext ctx;
        HtmlParser::instance().parseHtmlBegin(html, htmlLen, ctx);
        HtmlParser::instance().parseHtmlNext(ctx, DEFAULT_BOOK_NAME_XPATH, titles);
        HtmlParser::instance().parseHtmlNext(ctx, DEFAULT_CHAPTER_URL_XPATH, urls);
        HtmlParser::instance().parseHtmlNext(ctx, DEFAULT_CHAPTER_NAME_XPATH, texts);
        HtmlParser::instance().parseHtmlEnd(ctx);
        conv_free(html);

        if (titles.empty() || urls.empty() || urls.size() != texts.size())
        {
            messageBox(IDS_PARSE_FAIL2, IDS_ERROR, MB_ICONERROR | MB_OK);
            return (INT_PTR)TRUE;
        }

        char buffer[2048] = {0};
        for (int i = 0; i < (int)urls.size(); i++)
        {
            std::string url = urls[i];
            std::wstring text = Utils::trim(toUtf16(texts[i]));
            if (!text.empty() && !isExist(url, text) && Utils::isValidHref(url))
            {
                ChapterItem item;
                item.title = text;
                https_combine_url(m_url.c_str(), url.c_str(), buffer);
                item.url = buffer;
                item.checked = TRUE;
                m_chapters.push_back(item);
            }
        }

        if (m_chapters.empty())
        {
            messageBox(IDS_PARSE_FAIL2, IDS_ERROR, MB_ICONERROR | MB_OK);
            return (INT_PTR)TRUE;
        }

        std::wstring fileName = Utils::dirtyFileName(Utils::trim(toUtf16(titles[0])));
        if (fileName.empty()) fileName = DEFAULT_BOOK_NAME;
        SetDlgItemTextW(m_hWnd, IDC_EDIT_NAME, fileName.c_str());

        buildListView(0);
    }
    else if (m_state == State::PREVIEW)
    {
        m_state = State::IDLE;
        if (!html)
        {
            enableControls();
            messageBox(IDS_PREVIEW_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
            return (INT_PTR)TRUE;
        }

        std::vector<std::string> contents;

        HtmlParser::instance().tidyHtml(html, htmlLen);
        HtmlParser::instance().parseHtml(html, htmlLen, DEFAULT_CONTENT_XPATH, contents);
        conv_free(html);

        if (contents.empty())
        {
            messageBox(IDS_PREVIEW_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
            return (INT_PTR)TRUE;
        }

        m_content.clear();
        for (auto& i : contents)
        {
            m_content += toUtf16(i);
            m_content += L"\n";
        }
        Utils::formatText(m_content);

#ifdef _USING_V110_SDK71_
        Utils::replace(m_content, L"\n", L"\r\n");
#endif

        SetDlgItemTextW(m_hWnd, IDC_EDIT_CONTENT, m_content.c_str());
    }

    enableControls();
    return (INT_PTR)TRUE;
}

LRESULT SimpleSearchDlg::onListChecked(int index, BOOL checked)
{
    if (index >= 0)
    {
        int idx = (int)(INT_PTR)m_list->itemUserData(index);
        if (idx >= 0 && idx < (int)m_chapters.size())
        {
            m_chapters[idx].checked = checked;
        }
    }
    return (INT_PTR)TRUE;
}

LRESULT SimpleSearchDlg::onListPreview(int index)
{
    int idx = (int)(INT_PTR)m_list->itemUserData(index);
    if (idx < 0 || idx >= (int)m_chapters.size())
        return (INT_PTR)TRUE;
    auto& chapter = m_chapters[idx];

    request_t req;
    memset(&req, 0, sizeof(request_t));
    req.method = GET;
    req.url = (char*)chapter.url.c_str();
    req.param1 = m_hWnd;
    req.param2 = NULL;
    req.completer = completer;

    m_content.clear();
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CONTENT, m_content.c_str());
    m_state = State::PREVIEW;
    enableControls();
    m_hRequest = https_request(&req);
    return (INT_PTR)TRUE;
}

LRESULT SimpleSearchDlg::onListMoveUp(int index)
{
    int idx1 = (int)(INT_PTR)m_list->itemUserData(index);
    int idx2 = (int)(INT_PTR)m_list->itemUserData(index - 1);
    if (idx1 < 0 || idx1 >= (int)m_chapters.size() || idx2 < 0 || idx2 >= (int)m_chapters.size()) return TRUE;
    std::swap(m_chapters[idx1], m_chapters[idx2]);
    return (INT_PTR)TRUE;
}

LRESULT SimpleSearchDlg::onListMoveDown(int index)
{
    int idx1 = (int)(INT_PTR)m_list->itemUserData(index);
    int idx2 = (int)(INT_PTR)m_list->itemUserData(index + 1);
    if (idx1 < 0 || idx1 >= (int)m_chapters.size() || idx2 < 0 || idx2 >= (int)m_chapters.size()) return TRUE;
    std::swap(m_chapters[idx1], m_chapters[idx2]);
    return (INT_PTR)TRUE;
}

LRESULT SimpleSearchDlg::onListMoveTop(int index)
{
    for (int i = index; i > 0; i--)
    {
        int idx1 = (int)(INT_PTR)m_list->itemUserData(i);
        int idx2 = (int)(INT_PTR)m_list->itemUserData(i - 1);
        std::swap(m_chapters[idx1], m_chapters[idx2]);
    }
    return (INT_PTR)TRUE;
}

LRESULT SimpleSearchDlg::onListMoveBottom(int index)
{
    int count = m_list->itemCount();
    for (int i = index; i < count - 1; i++)
    {
        int idx1 = (int)(INT_PTR)m_list->itemUserData(i);
        int idx2 = (int)(INT_PTR)m_list->itemUserData(i + 1);
        std::swap(m_chapters[idx1], m_chapters[idx2]);
    }
    return (INT_PTR)TRUE;
}

void SimpleSearchDlg::buildListView(int index)
{
    m_list->clearItems();
    int row = 0;
    for (int i = 0; i < (int)m_chapters.size(); i++)
    {
        if (isChapter(m_chapters[i].title))
        {
            m_list->addItem(row, m_chapters[i].title.c_str(), (void*)(INT_PTR)i);
            m_list->setItemChecked(row, m_chapters[i].checked);
            row++;
        }
    }
    m_list->setItemSelected(index, true);
}

void SimpleSearchDlg::enableControls()
{
    if (m_state == State::IDLE)
    {
        EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_URL), TRUE);
        EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_NAME), TRUE);
        EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_FILTER), TRUE);
        EnableWindow(GetDlgItem(m_hWnd, IDC_BUTTON_FILTER), TRUE);
        EnableWindow(GetDlgItem(m_hWnd, IDC_LIST_CHAPTER), TRUE);
        EnableWindow(GetDlgItem(m_hWnd, IDOK), TRUE);
        SetDlgItemText(m_hWnd, IDC_BUTTON_PARSE, loadString(IDS_START_PARSE).c_str());
    }
    else if (m_state == State::PARSING)
    {
        EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_URL), FALSE);
        EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_NAME), FALSE);
        EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_FILTER), FALSE);
        EnableWindow(GetDlgItem(m_hWnd, IDC_BUTTON_FILTER), FALSE);
        EnableWindow(GetDlgItem(m_hWnd, IDC_LIST_CHAPTER), FALSE);
        EnableWindow(GetDlgItem(m_hWnd, IDOK), FALSE);
        SetDlgItemText(m_hWnd, IDC_BUTTON_PARSE, loadString(IDS_STOP_PARSE).c_str());
    }
    else if (m_state == State::PREVIEW)
    {
        EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_URL), FALSE);
        EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_NAME), FALSE);
        EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_FILTER), FALSE);
        EnableWindow(GetDlgItem(m_hWnd, IDC_BUTTON_FILTER), FALSE);
        EnableWindow(GetDlgItem(m_hWnd, IDC_LIST_CHAPTER), FALSE);
        EnableWindow(GetDlgItem(m_hWnd, IDOK), FALSE);
        SetDlgItemText(m_hWnd, IDC_BUTTON_PARSE, loadString(IDS_STOP_PREVIEW).c_str());
    }
}

bool SimpleSearchDlg::isChapter(const std::wstring& text)
{
    if (!m_filter)
        return true;

    try
    {
        return std::regex_search(text, *m_filter);
    }
    catch (...)
    {
    }
    return false;
}

bool SimpleSearchDlg::isExist(const std::string& url, const std::wstring& text)
{
    return std::find_if(m_chapters.begin(), m_chapters.end(), [&](const ChapterItem& item) { return item.url == url && item.title == text; }) != m_chapters.end();
}

LRESULT SimpleSearchDlg::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return onInitDialog();

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            return onOpenBookButtonClick();
        case IDCANCEL:
            return onCloseButtonClick();
        case IDC_BUTTON_PARSE:
            return onParseButtonClick();
        case IDC_BUTTON_FILTER:
            return onFilterButtonClick();
        }
        break;

    case WM_SIZE:
        return onSize(LOWORD(lParam), HIWORD(lParam));

    case WM_NOTIFY:
        if (LOWORD(wParam) == IDC_LIST_CHAPTER)
        {
            if (m_list) return m_list->onNotify(wParam, lParam);
        }
        break;

    case WM_LIST_CHECKED:
        return onListChecked((int)wParam, (BOOL)lParam);
    case WM_LIST_MOVEUP:
        return onListMoveUp((int)wParam);
    case WM_LIST_MOVEDOWN:
        return onListMoveDown((int)wParam);
    case WM_LIST_MOVETOP:
        return onListMoveTop((int)wParam);
    case WM_LIST_MOVEBOTTOM:
        return onListMoveBottom((int)wParam);
    case WM_LIST_PREVIEW:
        return onListPreview((int)wParam);

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

unsigned int SimpleSearchDlg::completer(request_result_t* result)
{
    HWND hWnd = (HWND)result->param1;
    if (result->errno_ == ECAN)
    {
        return 1;
    }
    if (result->errno_ != SUCC)
    {
        PostMessage(hWnd, WM_REQCOMPLETER, 0, NULL);
        return 1;
    }
    if (result->status_code != 200)
    {
        PostMessage(hWnd, WM_REQCOMPLETER, result->status_code, NULL);
        return 1;
    }
    char* html = nullptr;
    int htmlLen = conv_to_utf8(result->body, result->bodylen, &html);
    if (!html)
    {
        PostMessage(hWnd, WM_REQCOMPLETER, -1, NULL);
        return 1;
    }
    if (!PostMessage(hWnd, WM_REQCOMPLETER, htmlLen, (LPARAM)html))
    {
        conv_free(html);
        return 1;
    }
    return 0;
}
#endif
