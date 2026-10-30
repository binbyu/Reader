#ifdef ENABLE_NETWORK
#include "SourceSearchDlg.h"
#include "resource.h"
#include "BookSourceMgrDlg.h"
#include "Settings.h"
#include "https.h"
#include "conv.h"
#include "HtmlParser.h"
#include "OLFile.h"
#include "Logger.h"
#include "Utils.h"

SourceSearchDlg::SourceSearchDlg(HINSTANCE hInstance, HWND hParent)
    : Dialog(hInstance, hParent, IDD_SOURCESEARCH)
    , m_list(nullptr)
{
}

SourceSearchDlg::~SourceSearchDlg()
{
    if (m_list)
    {
        delete m_list;
        m_list = nullptr;
    }
}

std::wstring SourceSearchDlg::openFileName() const
{
    return m_fileName;
}

LRESULT SourceSearchDlg::onInitDialog()
{
    HWND hList = GetDlgItem(m_hWnd, IDC_LIST_QUERY);
    m_list = new ListView(hList, this);
    m_list->addHeader(IDS_BOOK_SOURCE, 150);
    m_list->addHeader(IDS_BOOK_NAME, 150);
    m_list->addHeader(IDS_AUTHOR, 100);
    m_list->addHeader(IDS_MAINPAGE, 230);

    updateBookSourceComboBox(0);

    centerParent();
    return TRUE;
}

LRESULT SourceSearchDlg::onCloseButtonClick()
{
    m_reqQue.stop();
    //EndDialog(m_hWnd, IDCANCEL);
    PostMessage(m_hParentWnd, WM_CLOSE, 0, 0);
    return (INT_PTR)TRUE;
}

LRESULT SourceSearchDlg::onBookSourceMgrButtonClick()
{
    auto& sources = Settings::instance().data().sources;
    int index = (int)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_BS_LIST), CB_GETCURSEL, 0, NULL);
    if (index <= 0 || index > (int)sources.size())
    {
        index = 0;
    }
    else
    {
        index--; // 0 is global search
    }

    BookSourceMgrDlg dlg(m_hInstance, m_hWnd);
    dlg.setCurrentBookSourceIndex(index);
    dlg.exec();
    index = dlg.currentBookSourceIndex();

    sources = Settings::instance().data().sources;
    if (index >= 0 && index < (int)sources.size())
    {
        index++;
    }
    else
    {
        index = 0;
    }
    updateBookSourceComboBox(index);
    return TRUE;
}

LRESULT SourceSearchDlg::onSearchButtonClick()
{
    auto& sources = Settings::instance().data().sources;
    if (sources.empty())
    {
        if (IDYES == messageBox(IDS_NOTEXIST_BOOKSOURCE, IDS_ERROR, MB_ICONERROR | MB_YESNO))
        {
            return onBookSourceMgrButtonClick();
        }
        return TRUE;
    }

    // stop search
    if (!m_reqQue.empty())
    {
        m_reqQue.stop();
        enableControls(true);
        return TRUE;
    }

    // start search
    std::wstring keyword = getDlgItemTextW(IDC_EDIT_QUERY_KEYWORD);
    if (keyword.empty())
    {
        messageBox(IDS_EMPTY_KEYWORD, IDS_ERROR, MB_ICONERROR | MB_OK);
        return TRUE;
    }

    enableControls(false);
    m_list->clearItems();
    m_bookItems.clear();
    m_reqQue.clear();

    int index = (int)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_BS_LIST), CB_GETCURSEL, 0, NULL);
    if (index == 0)
    {
        for (int i = 0; i < (int)sources.size(); i++)
        {
            doRequest(i, keyword, true);
        }
    }
    else if (index > 0 && index <= (int)sources.size())
    {
        doRequest(index - 1, keyword, false);
    }
    m_reqQue.start();
    return TRUE;
}

LRESULT SourceSearchDlg::onOpenBookButtonClick()
{
    int index = m_list->selectedItem();
    if (index < 0 || index >= (int)m_bookItems.size())
    {
        return TRUE;
    }

    BookItem& item = m_bookItems[index];
    std::wstring savePath = Utils::appPath() + ONLINE_FILE_SAVE_PATH;
    if (!Utils::makeDir(savePath))
    {
        lgerror("Create online file save path failed. Error code: %lu", GetLastError());
        messageBox(IDS_OPEN_FILE_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
        return TRUE;
    }
    SetFileAttributesW(savePath.c_str(), FILE_ATTRIBUTE_HIDDEN);

    std::wstring bookName = Utils::dirtyFileName(item.bookName);
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

    OLFile olFile(bookName, item.bookMainPageUrl, item.bsHost);
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

LRESULT SourceSearchDlg::onSize(int width, int height)
{
    const int BORDER = 8;

    HWND hEditKeyword = GetDlgItem(m_hWnd, IDC_EDIT_QUERY_KEYWORD);
    HWND hComboSource = GetDlgItem(m_hWnd, IDC_COMBO_BS_LIST);
    HWND hButtonQuery = GetDlgItem(m_hWnd, IDC_BUTTON_OL_QUERY);
    HWND hListBooks = GetDlgItem(m_hWnd, IDC_LIST_QUERY);
    HWND hButtonMgr = GetDlgItem(m_hWnd, IDC_BUTTON_BS_MANAGER);
    HWND hButtonOpen = GetDlgItem(m_hWnd, IDOK);

    RECT rcTemp;
    GetWindowRect(hButtonOpen, &rcTemp);
    int BTN_W = rcTemp.right - rcTemp.left;
    int BTN_H = rcTemp.bottom - rcTemp.top;
    GetWindowRect(hComboSource, &rcTemp);
    int CBX_W = rcTemp.right - rcTemp.left;

    int x = BORDER;
    int y = BORDER;
    int w = CBX_W;
    int h = BTN_H;
    SetWindowPos(hComboSource, NULL, x, y, w, h, SWP_NOZORDER);
    x += w + BORDER;
    y = BORDER;
    w = width - x - BORDER - BTN_W - BORDER;
    h = BTN_H;
    SetWindowPos(hEditKeyword, NULL, x, y, w, h, SWP_NOZORDER);
    x += w + BORDER;
    y = BORDER;
    w = BTN_W;
    h = BTN_H;
    SetWindowPos(hButtonQuery, NULL, x, y, w, h, SWP_NOZORDER);
    x = BORDER;
    y = BORDER + BTN_H + BORDER;
    w = width - BORDER - x;
    h = height - BORDER - BTN_H - BORDER - y;
    SetWindowPos(hListBooks, NULL, x, y, w, h, SWP_NOZORDER);
    x = BORDER;
    y = height - BORDER - BTN_H;
    w = BTN_W;
    h = BTN_H;
    SetWindowPos(hButtonMgr, NULL, x, y, w, h, SWP_NOZORDER);
    x = width - BORDER - BTN_W;
    y = height - BORDER - BTN_H;
    w = BTN_W;
    h = BTN_H;
    SetWindowPos(hButtonOpen, NULL, x, y, w, h, SWP_NOZORDER);
    return TRUE;
}

LRESULT SourceSearchDlg::onReqCompleter(WPARAM wParam, LPARAM lParam)
{
    PostData* pd = (PostData*)lParam;
    if (!pd)
    {
        goto END;
    }
    if (pd->errCode < 0)
    {
        if (pd->isGlobal)
        {
            goto END;
        }
        if (pd->errCode == -1)
        {
            messageBox(IDS_NETWORK_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
            goto END;
        }
        else if (pd->errCode == -2)
        {
            messageBoxFmt(IDS_REQUEST_ERROR, IDS_ERROR, MB_ICONERROR | MB_OK, pd->statusCode);
            goto END;
        }
        else if (pd->errCode == -3)
        {
            messageBox(IDS_CONV_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
            goto END;
        }
        else
        {
            messageBox(IDS_ERROR, IDS_ERROR, MB_ICONERROR | MB_OK);
            goto END;
        }
    }
    else
    {
        auto& sources = Settings::instance().data().sources;
        if (pd->bsIndex < 0|| pd->bsIndex >= (int)sources.size())
        {
            if (!pd->isGlobal)
            {
                messageBox(IDS_SELECT_BOOKSOURCE, IDS_ERROR, MB_ICONERROR | MB_OK);
            }
            goto END;
        }

        BookSource& source = sources[pd->bsIndex];
        std::vector<std::string> table_name;
        std::vector<std::string> table_url;
        std::vector<std::string> table_author;
        ParserContext parser;
        HtmlParser::instance().parseHtmlBegin(pd->html, pd->htmlLen, parser);
        HtmlParser::instance().parseHtmlNext(parser, source.bookNameXpath, table_name);
        HtmlParser::instance().parseHtmlNext(parser, source.bookMainpageXpath, table_url);
        if (!source.bookAuthorXpath.empty())
        {
            HtmlParser::instance().parseHtmlNext(parser, source.bookAuthorXpath, table_author);
        }
        HtmlParser::instance().parseHtmlEnd(parser);

        if (table_url.empty() || table_name.size() != table_url.size())
        {
            Utils::dumpHtmlParseErrorFile(pd->html, pd->htmlLen);
            if (!pd->isGlobal)
            {
                messageBox(IDS_PARSE_FAIL, IDS_WARN, MB_ICONWARNING | MB_OK);
            }
            goto END;
        }

        char buffer[2048] = { 0 };
        size_t start = m_bookItems.size();
        for (size_t i = 0; i < table_name.size(); i++)
        {
            https_combine_url(source.queryUrl.c_str(), table_url[i].c_str(), buffer);

            BookItem item;
            item.bsHost = source.host;
            item.bsName = source.title;
            item.bookName = toUtf16(table_name[i]);
            if (i < table_author.size()) item.bookAuthor = toUtf16(table_author[i]);
            item.bookMainPageUrl = buffer;
            m_bookItems.push_back(item);
        }
        appendListView(&m_bookItems[start], (int)(m_bookItems.size() - start));
    }

END:
    freePostData(pd);
    if (m_reqQue.empty())
    {
        enableControls(true);
    }
    return TRUE;
}

void SourceSearchDlg::updateBookSourceComboBox(int index)
{
    HWND hCombo = GetDlgItem(m_hWnd, IDC_COMBO_BS_LIST);
    auto& sources = Settings::instance().data().sources;
    SendMessage(hCombo, CB_RESETCONTENT, 0, NULL);
    if (sources.size() > 0) // first add global search
    {
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)loadString(IDS_GLOBAL_SEARCH).c_str());
    }
    for (auto& source : sources)
    {
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)source.title.c_str());
    }
    if (sources.size() > 0 && index >= 0 && index <= (int)sources.size())
    {
        SendMessage(hCombo, CB_SETCURSEL, index, NULL);
    }
}

void SourceSearchDlg::enableControls(bool enable)
{
    EnableWindow(GetDlgItem(m_hWnd, IDC_COMBO_BS_LIST), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_QUERY_KEYWORD), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_BUTTON_BS_MANAGER), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDOK), enable);
    // EnableWindow(GetDlgItem(m_hWnd, IDCANCEL), enable);
    SetDlgItemText(m_hWnd, IDC_BUTTON_OL_QUERY, loadString(enable ? IDS_QUERY : IDS_STOP_QUERY).c_str());
}

void SourceSearchDlg::appendListView(BookItem* items, int count)
{
    int row = m_list->itemCount();
    for (int i = 0; i < count; i++)
    {
        BookItem* item = &items[i];
        std::wstring mainPageUrl = toUtf16(item->bookMainPageUrl);

        m_list->addItem(row, { item->bsName, item->bookName, item->bookAuthor, mainPageUrl });
        row++;
    }
}

void SourceSearchDlg::doRequest(int bsIndex, const std::wstring& keyword, bool isGlobal)
{
    BookSource& source = Settings::instance().data().sources[bsIndex];
    Request req;
    buildUrl(bsIndex, source.queryCharset == 2 ? http_charset_t::GBK : http_charset_t::UTF8, keyword, req);

    if (source.queryCharset == 0) // detect charset
    {
        req.method = request_method_t::HEAD;
    }
    req.param = std::make_shared<RData>(m_hWnd, keyword, bsIndex, isGlobal);
    req.completer = std::bind(&SourceSearchDlg::requestCompleter, this, std::placeholders::_1, std::placeholders::_2);
    m_reqQue.pushBack(req);
}

void SourceSearchDlg::buildUrl(int bsIndex, http_charset_t charset, const std::wstring& keyword, Request& req)
{
    BookSource& source = Settings::instance().data().sources[bsIndex];

    // encode keyword
    char* keyword_src = nullptr;
    if (charset == http_charset_t::GBK)
    {
       conv_utf16_to_ansi(keyword.c_str(), (int)keyword.size(), &keyword_src);
    }
    else
    {
       conv_utf16_to_utf8(keyword.c_str(), (int)keyword.size(), &keyword_src);
    }
    char keyword_enc[MAX_URL_LENGTH] = { 0 };
    https_url_encode(keyword_src, keyword_enc, MAX_URL_LENGTH);
    conv_free(keyword_src);

    // build query url & params
    if (source.queryMethod == 0) // GET
    {
       char buffer[MAX_URL_LENGTH] = { 0 };
       snprintf(buffer, MAX_URL_LENGTH, source.queryUrl.c_str(), keyword_enc);
       req.method = request_method_t::GET;
       req.url = buffer;
       req.content.clear();
    }
    else
    {
       char buffer[MAX_URL_LENGTH] = { 0 };
       snprintf(buffer, MAX_URL_LENGTH, source.queryParams.c_str(), keyword_enc);
       req.method = request_method_t::POST;
       req.url = source.queryUrl;
       req.content = buffer;
    }
}

void SourceSearchDlg::requestCompleter(request_result_t* result, const Request& r)
{
    std::shared_ptr<RData> data = std::static_pointer_cast<RData>(r.param);
    PostData* pd = new PostData(data->isGlobal);
    BOOL ret = TRUE;

    if (result->errno_ != SUCC)
    {
        pd->errCode = -1;
        ret = PostMessage(data->hWnd, WM_REQCOMPLETER, 0, (LPARAM)pd);
        goto END;
    }

    if (result->status_code != 200)
    {
        pd->errCode = -2;
        pd->statusCode = result->status_code;
        ret = PostMessage(data->hWnd, WM_REQCOMPLETER, 0, (LPARAM)pd);
        goto END;
    }

    if (result->req->method == request_method_t::HEAD)
    {
        auto charset = https_get_charset(result->header);
        Request req;
        buildUrl(data->bsIndex, charset, data->keyword, req);
        req.param = data;
        req.completer = std::bind(&SourceSearchDlg::requestCompleter, this, std::placeholders::_1, std::placeholders::_2);
        m_reqQue.pushFront(req);
        freePostData(pd);
        return;
    }
    else
    {
        char *html = nullptr;
        int htmlLen = conv_to_utf8(result->body, result->bodylen, &html);
        if (!html)
        {
            pd->errCode = -3;
            ret = PostMessage(data->hWnd, WM_REQCOMPLETER, 0, (LPARAM)pd);
            goto END;
        }
        pd->bsIndex = data->bsIndex;
        pd->htmlLen = htmlLen;
        pd->html = html;
        ret = PostMessage(data->hWnd, WM_REQCOMPLETER, 0, (LPARAM)pd);
    }

END:
    if (!ret)
    {
        freePostData(pd);
    }
}

void SourceSearchDlg::freePostData(PostData* pd)
{
    if (pd)
    {
        if (pd->html)
        {
            conv_free(pd->html);
        }
        delete pd;
    }
}

LRESULT SourceSearchDlg::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
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
        case IDC_BUTTON_OL_QUERY:
            return onSearchButtonClick();
        case IDC_BUTTON_BS_MANAGER:
            return onBookSourceMgrButtonClick();
        }
        break;

    case WM_SIZE:
        return onSize(LOWORD(lParam), HIWORD(lParam));

    case WM_NOTIFY:
        if (LOWORD(wParam) == IDC_LIST_QUERY)
        {
            if (m_list) return m_list->onNotify(wParam, lParam);
        }
        break;

    case WM_LIST_DBLCLK:
        return onOpenBookButtonClick();

    case WM_REQCOMPLETER:
        return onReqCompleter(wParam, lParam);

    case WM_DESTROY:
    {
        m_reqQue.stop();
        MSG msg;
        while (PeekMessage(&msg, m_hWnd, WM_REQCOMPLETER, WM_REQCOMPLETER, PM_REMOVE))
        {
            freePostData((PostData*)msg.lParam);
        }
        break;
    }

    default:
        break;
    }
    return (INT_PTR)FALSE;
}
#endif