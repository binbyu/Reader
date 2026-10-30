#ifdef ENABLE_NETWORK
#include "BookSourceMgrDlg.h"
#include "resource.h"
#include "Utils.h"
#include "JsonSerialize.h"
#include "conv.h"
#include <regex>

BookSourceMgrDlg::BookSourceMgrDlg(HINSTANCE hInstance, HWND hParent)
    : Dialog(hInstance, hParent, IDD_BOOKSOURCEMGR)
    , m_currentIndex(0)
    , m_hSyncReq(nullptr)
    , m_list(nullptr)
{
}

BookSourceMgrDlg::~BookSourceMgrDlg()
{
    if (m_list)
    {
        delete m_list;
        m_list = nullptr;
    }
}

void BookSourceMgrDlg::setCurrentBookSourceIndex(int index)
{
    if (index >= 0 && index < (int)Settings::instance().data().sources.size())
    {
        m_currentIndex = index;
    }
}

int BookSourceMgrDlg::currentBookSourceIndex() const
{
    return m_currentIndex;
}

LRESULT BookSourceMgrDlg::onInitDialog()
{
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_FILTER), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_NULL).c_str());
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_FILTER), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_KEYWORD).c_str());
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_FILTER), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_REGEX).c_str());
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_METHOD), CB_ADDSTRING, 0, (LPARAM)L"GET");
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_METHOD), CB_ADDSTRING, 0, (LPARAM)L"POST");
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_CHARSET), CB_ADDSTRING, 0, (LPARAM)L"AUTO");
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_CHARSET), CB_ADDSTRING, 0, (LPARAM)L"UTF-8");
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_CHARSET), CB_ADDSTRING, 0, (LPARAM)L"GBK");
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_LISTPAGE), CB_ADDSTRING, 0, (LPARAM)L"Disabled");
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_LISTPAGE), CB_ADDSTRING, 0, (LPARAM)L"Enabled");
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_CPT_NEXT), CB_ADDSTRING, 0, (LPARAM)L"Disabled");
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_CPT_NEXT), CB_ADDSTRING, 0, (LPARAM)L"Enabled");
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_CTX_NEXT), CB_ADDSTRING, 0, (LPARAM)L"Disabled");
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_CTX_NEXT), CB_ADDSTRING, 0, (LPARAM)L"Enabled");

    HWND hList = GetDlgItem(m_hWnd, IDC_LIST_BOOKSRC);
    m_list = new ListView(hList, this);
    m_list->setContextMenuType(CONTEXT_MENU_DELETE | CONTEXT_MENU_CLEAR | CONTEXT_MENU_MOVE);
    m_list->addHeader(IDS_TITLE, 146);
    int index = m_currentIndex;
    m_currentIndex = -1;
    updateBookSourceList(index);

    centerParent();
    return TRUE;
}

LRESULT BookSourceMgrDlg::onCloseButtonClick()
{
    if (m_hSyncReq)
    {
        https_cancel(m_hSyncReq);
        m_hSyncReq = nullptr;
    }

    EndDialog(m_hWnd, IDCANCEL);
    return (INT_PTR)TRUE;
}

LRESULT BookSourceMgrDlg::onExportButtonClick()
{
    wchar_t fileName[MAX_PATH] = { 0 };
    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = L"Book Source (*.json)\0*.json\0\0";
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"json";
    ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    if (!GetSaveFileName(&ofn))
    {
        return (INT_PTR)TRUE;
    }

    std::vector<uint8_t> bytes;
    if (!JsonSerialize::enserialize(Settings::instance().data().sources, bytes))
    {
        messageBox(IDS_EXPORT_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
        return (INT_PTR)TRUE;
    }
    FILE* fp = _wfopen(fileName, L"wb");
    if (!fp)
    {
        messageBox(IDS_OPEN_FILE_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
        return (INT_PTR)TRUE;
    }
    if (bytes.size() != fwrite(bytes.data(), 1, bytes.size(), fp))
    {
        fclose(fp);
        messageBox(IDS_EXPORT_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
        return (INT_PTR)TRUE;
    }
    fclose(fp);
    messageBox(IDS_EXPORT_COMPLETED, IDS_TIPS, MB_ICONINFORMATION | MB_OK);
    return (INT_PTR)TRUE;
}

LRESULT BookSourceMgrDlg::onImportButtonClick()
{
    wchar_t fileName[MAX_PATH] = { 0 };
    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = L"Book Source (*.json)\0*.json\0All Files\0*.*\0\0";
    ofn.lpstrInitialDir = nullptr;
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L"json";
    ofn.nFilterIndex = 0;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    if (!GetOpenFileName(&ofn))
    {
        return (INT_PTR)TRUE;
    }

    FILE* fp = _wfopen(fileName, L"rb");
    if (!fp)
    {
        messageBox(IDS_OPEN_FILE_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
        return (INT_PTR)TRUE;
    }
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    std::vector<uint8_t> bytes(size);
    if (size != fread(bytes.data(), 1, size, fp))
    {
        fclose(fp);
        messageBox(IDS_IMPORT_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
        return (INT_PTR)TRUE;
    }
    fclose(fp);

    std::vector<BookSource> sources;
    if (!JsonSerialize::deserialize(bytes, sources))
    {
        messageBox(IDS_IMPORT_FAILED, IDS_ERROR, MB_ICONERROR | MB_OK);
        return (INT_PTR)TRUE;
    }

    if (sources.empty())
    {
        messageBox(IDS_BS_NOT_FOUND, IDS_TIPS, MB_ICONINFORMATION | MB_OK);
        return (INT_PTR)TRUE;
    }

    if (mergeBookSourceList(sources) > 0)
    {
        Settings::instance().save();
        updateBookSourceList(0);
    }
    messageBox(IDS_IMPORT_COMPLETED, IDS_SUCC, MB_ICONINFORMATION | MB_OK);
    return (INT_PTR)TRUE;
}

LRESULT BookSourceMgrDlg::onManualButtonClick()
{
    ShellExecuteW(NULL, L"open", BOOKSOURCE_MANUAL_DOC, NULL, NULL, SW_SHOWNORMAL);
    return (INT_PTR)TRUE;
}

LRESULT BookSourceMgrDlg::onSyncButtonClick()
{
    if (m_hSyncReq)
    {
        // cancel request
        https_cancel(m_hSyncReq);
        m_hSyncReq = nullptr;
        enableControls(true);
        return (INT_PTR)TRUE; 
    }

    request_t req;
    memset(&req, 0, sizeof(request_t));
    req.method = GET;
    req.url = (char *)BOOKSOURCE_SAMPLE_FILE;
    req.param1 = m_hWnd;
    req.param2 = NULL;
    req.completer = [](request_result_t *result) -> unsigned int {
        
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

    enableControls(false);
    m_hSyncReq = https_request(&req);
    return (INT_PTR)TRUE;
}

LRESULT BookSourceMgrDlg::onSaveButtonClick()
{
    BookSource source;
    getDataFromUI(source);

    auto& sources = Settings::instance().data().sources;
    if (m_currentIndex < 0 || m_currentIndex >= (int)sources.size())
    {
        messageBox(IDS_NO_BOOKSOURCE, IDS_ERROR, MB_ICONERROR | MB_OK);
        return (INT_PTR)TRUE;
    }

    if (isBookSourceEmpty(source))
    {
        messageBox(IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
        return (INT_PTR)TRUE;
    }
    if (!isBookSourceValid(source))
    {
        messageBox(IDS_INVALID_REGEX, IDS_ERROR, MB_ICONERROR | MB_OK);
        return (INT_PTR)TRUE;
    }
    if (isBookSourceExist(source, m_currentIndex))
    {
        messageBox(IDS_BOOKSOURCE_EXIST, IDS_ERROR, MB_ICONERROR | MB_OK);
        return (INT_PTR)TRUE;
    }

    const auto& target = sources[m_currentIndex];
    if (target == source)
    {
        messageBox(IDS_SAVE_COMPLETED, IDS_SUCC, MB_ICONINFORMATION | MB_OK);
        return (INT_PTR)TRUE;
    }

    // check if want to add new book source
    if (target.title != source.title && target.host != source.host)
    {
        if (IDYES != messageBoxFmt(IDS_BS_SAVE_TIP, IDS_TIPS, MB_ICONQUESTION | MB_YESNO, target.title.c_str()))
        {
            return (INT_PTR)TRUE;
        }
    }

    sources[m_currentIndex] = source;
    Settings::instance().save();
    m_list->setItem(m_currentIndex, 0, source.title.c_str());
    messageBox(IDS_SAVE_COMPLETED, IDS_SUCC, MB_ICONINFORMATION | MB_OK);
    return (INT_PTR)TRUE;
}

LRESULT BookSourceMgrDlg::onAddButtonClick()
{
    BookSource source;
    getDataFromUI(source);

    if (isBookSourceEmpty(source))
    {
        messageBox(IDS_FIELED_EMPTY, IDS_ERROR, MB_ICONERROR | MB_OK);
        return (INT_PTR)TRUE;
    }
    if (!isBookSourceValid(source))
    {
        messageBox(IDS_INVALID_REGEX, IDS_ERROR, MB_ICONERROR | MB_OK);
        return (INT_PTR)TRUE;
    }
    if (isBookSourceExist(source, -1))
    {
        messageBox(IDS_BOOKSOURCE_EXIST, IDS_ERROR, MB_ICONERROR | MB_OK);
        return (INT_PTR)TRUE;
    }

    auto& sources = Settings::instance().data().sources;
    sources.push_back(source);
    Settings::instance().save();
    m_list->addItem((int)sources.size() - 1, source.title.c_str());
    m_list->setItemSelected((int)sources.size() - 1, true);
    m_currentIndex = (int)sources.size() - 1;

    messageBox(IDS_ADD_COMPLETED, IDS_SUCC, MB_ICONINFORMATION | MB_OK);
    return (INT_PTR)TRUE;
}

LRESULT BookSourceMgrDlg::onListSelected(int index, BOOL selected)
{
    auto PreventChange = [this](int curnIndex)
        {
            BookSource source;
            getDataFromUI(source);
            if (isBookSourceModified(source))
            {
                int ret = messageBox(IDS_BS_DISCARD_CONFIRM, IDS_TIPS, MB_ICONQUESTION | MB_YESNO);
                if (IDYES != ret)
                {
                    m_list->setItemSelected(curnIndex, true, false);
                    return TRUE;
                }
            }
            return FALSE;
        };

    auto& sources = Settings::instance().data().sources;
    if (selected)
    {
        if (m_currentIndex != index && index >= 0 && index < (int)sources.size())
        {
            if (PreventChange(m_currentIndex))
            {
                return FALSE;
            }

            m_currentIndex = index;
            setDataToUI(sources[index]);
        }
    }
    else
    {
        if (PreventChange(m_currentIndex))
        {
            return FALSE;
        }

        m_currentIndex = -1;
        clearUI();
    }
    return TRUE;
}

LRESULT BookSourceMgrDlg::onListMoveUp(int index)
{
    auto& sources = Settings::instance().data().sources;
    if (index > 0 && index < (int)sources.size())
    {
        std::swap(sources[index], sources[index - 1]);
        Settings::instance().save();
        m_currentIndex = index - 1;
    }
    return (INT_PTR)TRUE;
}

LRESULT BookSourceMgrDlg::onListMoveDown(int index)
{
    auto& sources = Settings::instance().data().sources;
    if (index >= 0 && index < (int)sources.size() - 1)
    {
        std::swap(sources[index], sources[index + 1]);
        Settings::instance().save();
        m_currentIndex = index + 1;
    }
    return (INT_PTR)TRUE;
}

LRESULT BookSourceMgrDlg::onListMoveTop(int index)
{
    auto& sources = Settings::instance().data().sources;
    if (index > 0 && index < (int)sources.size())
    {
        std::rotate(sources.begin(), sources.begin() + index, sources.begin() + index + 1);
        Settings::instance().save();
        m_currentIndex = 0;
    }
    return (INT_PTR)TRUE;
}

LRESULT BookSourceMgrDlg::onListMoveBottom(int index)
{
    auto& sources = Settings::instance().data().sources;
    if (index >= 0 && index < (int)sources.size() - 1)
    {
        std::rotate(sources.begin() + index, sources.begin() + index + 1, sources.end());
        Settings::instance().save();
        m_currentIndex = (int)sources.size() - 1;
    }
    return (INT_PTR)TRUE;
}

LRESULT BookSourceMgrDlg::onListDelete(int index)
{
    auto& sources = Settings::instance().data().sources;
    if (index >= 0 && index < (int)sources.size())
    {
        sources.erase(sources.begin() + index);
        Settings::instance().save();
        if (index == m_currentIndex) m_currentIndex = -1;
    }
    return (INT_PTR)TRUE;
}

LRESULT BookSourceMgrDlg::onListClear()
{
    onListSelected(-1, FALSE);
    Settings::instance().data().sources.clear();
    Settings::instance().save();
    m_currentIndex = -1;
    return (INT_PTR)TRUE;
}

LRESULT BookSourceMgrDlg::onComboBoxChanged()
{
    int queryMethod = (int)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_METHOD), CB_GETCURSEL, 0, NULL);
    int queryCharset = (int)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_CHARSET), CB_GETCURSEL, 0, NULL);
    bool enableChapterPage = (bool)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_LISTPAGE), CB_GETCURSEL, 0, NULL);
    bool enableChapterNext = (bool)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_CPT_NEXT), CB_GETCURSEL, 0, NULL);
    bool enableContentNext = (bool)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_CTX_NEXT), CB_GETCURSEL, 0, NULL);
    int contentFilterType = (int)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_FILTER), CB_GETCURSEL, 0, NULL);

    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_POST), queryMethod == 1);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_LISTPAGE), enableChapterPage);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CPT_NEXT), enableChapterNext);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CPT_KEYWORD), enableChapterNext);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CPT_KEYWORD_TXT), enableChapterNext);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CTX_NEXT), enableContentNext);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CTX_KEYWORD), enableContentNext);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CTX_KEYWORD_TXT), enableContentNext);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_FILTER), contentFilterType != 0);
    return (INT_PTR)TRUE;
}

LRESULT BookSourceMgrDlg::onReqCompleter(WPARAM wParam, LPARAM lParam)
{
    int htmlLen = (int)wParam;
    char* html = (char*)lParam;

    m_hSyncReq = nullptr;

    if (!html)
    {
        enableControls(true);
        messageBox(IDS_SYNC_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
        return (INT_PTR)TRUE;
    }

    std::vector<uint8_t> bytes;
    bytes.assign(html, html + htmlLen);
    conv_free(html);

    std::vector<BookSource> sources;
    if (!JsonSerialize::deserialize(bytes, sources))
    {
        enableControls(true);
        messageBox(IDS_SYNC_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
        return (INT_PTR)TRUE;
    }

    if (sources.empty())
    {
        enableControls(true);
        messageBox(IDS_SYNC_FAIL, IDS_ERROR, MB_ICONERROR | MB_OK);
        return (INT_PTR)TRUE;
    }

    if (mergeBookSourceList(sources) > 0)
    {
        Settings::instance().save();
        updateBookSourceList(0);
    }

    enableControls(true);
    messageBox(IDS_SYNC_SUCC, IDS_TIPS, MB_ICONINFORMATION | MB_OK);
    return (INT_PTR)TRUE;
}

void BookSourceMgrDlg::setDataToUI(const BookSource& data)
{
    SetDlgItemTextW(m_hWnd, IDC_EDIT_TITLE, data.title.c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_HOST, toUtf16(data.host).c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_QUERY, toUtf16(data.queryUrl).c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_POST, toUtf16(data.queryParams).c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_NAME, toUtf16(data.bookNameXpath).c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_MAINPAGE, toUtf16(data.bookMainpageXpath).c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_AUTHOR, toUtf16(data.bookAuthorXpath).c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_LISTPAGE, toUtf16(data.chapterPageXpath).c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CPT_TITLE, toUtf16(data.chapterTitleXpath).c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CPT_URL, toUtf16(data.chapterUrlXpath).c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CPT_NEXT, toUtf16(data.chapterNextUrlXpath).c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CPT_KEYWORD, toUtf16(data.chapterNextKeywordXpath).c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CPT_KEYWORD_TXT, toUtf16(data.chapterNextKeyword).c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CTX, toUtf16(data.contentXpath).c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CTX_NEXT, toUtf16(data.contentNextUrlXpath).c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CTX_KEYWORD, toUtf16(data.contentNextKeywordXpath).c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CTX_KEYWORD_TXT, toUtf16(data.contentNextKeyword).c_str());
    SetDlgItemTextW(m_hWnd, IDC_EDIT_FILTER, data.contentFilterKeyword.c_str());
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_METHOD), CB_SETCURSEL, data.queryMethod, NULL);
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_CHARSET), CB_SETCURSEL, data.queryCharset, NULL);
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_LISTPAGE), CB_SETCURSEL, data.enableChapterPage, NULL);
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_CPT_NEXT), CB_SETCURSEL, data.enableChapterNext, NULL);
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_CTX_NEXT), CB_SETCURSEL, data.enableContentNext, NULL);
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_FILTER), CB_SETCURSEL, data.contentFilterType, NULL);

    onComboBoxChanged();
}

void BookSourceMgrDlg::getDataFromUI(BookSource& data)
{
    data.title = getDlgItemTextW(IDC_EDIT_TITLE);
    data.host = getDlgItemTextA(IDC_EDIT_HOST);
    data.queryUrl = getDlgItemTextA(IDC_EDIT_QUERY);
    data.queryParams = getDlgItemTextA(IDC_EDIT_POST);
    data.bookNameXpath = getDlgItemTextA(IDC_EDIT_NAME);
    data.bookMainpageXpath = getDlgItemTextA(IDC_EDIT_MAINPAGE);
    data.bookAuthorXpath = getDlgItemTextA(IDC_EDIT_AUTHOR);
    data.chapterPageXpath = getDlgItemTextA(IDC_EDIT_LISTPAGE);
    data.chapterTitleXpath = getDlgItemTextA(IDC_EDIT_CPT_TITLE);
    data.chapterUrlXpath = getDlgItemTextA(IDC_EDIT_CPT_URL);
    data.chapterNextUrlXpath = getDlgItemTextA(IDC_EDIT_CPT_NEXT);
    data.chapterNextKeywordXpath = getDlgItemTextA(IDC_EDIT_CPT_KEYWORD);
    data.chapterNextKeyword = getDlgItemTextA(IDC_EDIT_CPT_KEYWORD_TXT);
    data.contentXpath = getDlgItemTextA(IDC_EDIT_CTX);
    data.contentNextUrlXpath = getDlgItemTextA(IDC_EDIT_CTX_NEXT);
    data.contentNextKeywordXpath = getDlgItemTextA(IDC_EDIT_CTX_KEYWORD);
    data.contentNextKeyword = getDlgItemTextA(IDC_EDIT_CTX_KEYWORD_TXT);
    data.contentFilterKeyword = getDlgItemTextW(IDC_EDIT_FILTER);
    data.queryMethod = (int)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_METHOD), CB_GETCURSEL, 0, NULL);
    data.queryCharset = (int)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_CHARSET), CB_GETCURSEL, 0, NULL);
    data.enableChapterPage = (bool)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_LISTPAGE), CB_GETCURSEL, 0, NULL);
    data.enableChapterNext = (bool)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_CPT_NEXT), CB_GETCURSEL, 0, NULL);
    data.enableContentNext = (bool)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_CTX_NEXT), CB_GETCURSEL, 0, NULL);
    data.contentFilterType = (int)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_FILTER), CB_GETCURSEL, 0, NULL);

    // format host
    if (!data.host.empty() && data.host.back() == '/')
    {
        data.host.pop_back();
    }

    // format filter keyword, replace '\r\n' to '\n'
    Utils::replace(data.contentFilterKeyword, L"\r\n", L"\n");
}

void BookSourceMgrDlg::clearUI()
{
    SetDlgItemTextW(m_hWnd, IDC_EDIT_TITLE, L"");
    SetDlgItemTextW(m_hWnd, IDC_EDIT_HOST, L"");
    SetDlgItemTextW(m_hWnd, IDC_EDIT_QUERY, L"");
    SetDlgItemTextW(m_hWnd, IDC_EDIT_POST, L"");
    SetDlgItemTextW(m_hWnd, IDC_EDIT_NAME, L"");
    SetDlgItemTextW(m_hWnd, IDC_EDIT_MAINPAGE, L"");
    SetDlgItemTextW(m_hWnd, IDC_EDIT_AUTHOR, L"");
    SetDlgItemTextW(m_hWnd, IDC_EDIT_LISTPAGE, L"");
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CPT_TITLE, L"");
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CPT_URL, L"");
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CPT_NEXT, L"");
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CPT_KEYWORD, L"");
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CPT_KEYWORD_TXT, L"");
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CTX, L"");
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CTX_NEXT, L"");
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CTX_KEYWORD, L"");
    SetDlgItemTextW(m_hWnd, IDC_EDIT_CTX_KEYWORD_TXT, L"");
    SetDlgItemTextW(m_hWnd, IDC_EDIT_FILTER, L"");
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_METHOD), CB_SETCURSEL, 0, NULL);
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_CHARSET), CB_SETCURSEL, 0, NULL);
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_LISTPAGE), CB_SETCURSEL, 0, NULL);
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_CPT_NEXT), CB_SETCURSEL, 0, NULL);
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_CTX_NEXT), CB_SETCURSEL, 0, NULL);
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_FILTER), CB_SETCURSEL, 0, NULL);

    onComboBoxChanged();
}

void BookSourceMgrDlg::updateBookSourceList(int index)
{
    auto& sources = Settings::instance().data().sources;
    m_list->clearItems();
    for (int i = 0; i < (int)sources.size(); i++)
    {
        m_list->addItem(i, sources[i].title.c_str());
    }
    m_list->setItemSelected(index, true);
}

void BookSourceMgrDlg::enableControls(bool enable)
{
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_TITLE), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_HOST), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_QUERY), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_POST), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_NAME), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_MAINPAGE), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_AUTHOR), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_LISTPAGE), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CPT_TITLE), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CPT_URL), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CPT_NEXT), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CPT_KEYWORD), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CPT_KEYWORD_TXT), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CTX), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CTX_NEXT), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CTX_KEYWORD), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_CTX_KEYWORD_TXT), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_FILTER), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_COMBO_METHOD), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_COMBO_CHARSET), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_COMBO_LISTPAGE), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_COMBO_CPT_NEXT), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_COMBO_CTX_NEXT), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_COMBO_FILTER), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_LIST_BOOKSRC), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EXPORT), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_IMPORT), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_MANUAL), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDC_SAVE), enable);
    EnableWindow(GetDlgItem(m_hWnd, IDOK), enable);
    SetDlgItemText(m_hWnd, IDC_SYNC, loadString(enable ? IDS_AUTO_SYNC : IDS_STOP_SYNC).c_str());

    if (enable)
    {
        onComboBoxChanged();
    }
}

int BookSourceMgrDlg::mergeBookSourceList(const std::vector<BookSource>& newSources)
{
    int count = 0;
    auto& oldSources = Settings::instance().data().sources;

    for (auto& i : newSources)
    {
        auto it = std::find_if(oldSources.begin(), oldSources.end(), [&i](const BookSource& j) -> bool
        {
            return j.host == i.host;
        });

        if (it == oldSources.end())
        {
            count++;
            oldSources.push_back(i);
        }
        else
        {
            if (*it == i)
            {
                // same source
                continue;
            }
            int ret = messageBoxFmt(IDS_BS_EXIST_TIP, IDS_TIPS, MB_ICONQUESTION | MB_YESNO, i.title.c_str());
            if (IDYES == ret)
            {
                count++;
                *it = i;
            }
            else if (IDNO == ret)
            {
                continue;
            }
            else
            {
                // cancel
                break;
            }
        }
    }
    return count;
}

bool BookSourceMgrDlg::isBookSourceEmpty(const BookSource& source)
{
    return (source.title.empty()
        || source.host.empty()
        || source.queryUrl.empty()
        || (source.queryMethod == 1 && source.queryParams.empty())
        || source.bookNameXpath.empty()
        || source.bookMainpageXpath.empty()
        // || source.bookAuthorXpath.empty()
        || (source.enableChapterPage && source.chapterPageXpath.empty())
        || source.chapterTitleXpath.empty()
        || source.chapterUrlXpath.empty()
        || (source.enableChapterNext && (source.chapterNextUrlXpath.empty() || source.chapterNextKeywordXpath.empty() || source.chapterNextKeyword.empty()))
        || source.contentXpath.empty()
        || (source.enableContentNext && (source.contentNextUrlXpath.empty() || source.contentNextKeywordXpath.empty() || source.contentNextKeyword.empty()))
        || (source.contentFilterType != 0 && source.contentFilterKeyword.empty())
        );
}

bool BookSourceMgrDlg::isBookSourceValid(const BookSource& source)
{
    if (source.contentFilterType == 2) // regex
    {
        try
        {
            std::wregex e(source.contentFilterKeyword);
        }
        catch (...)
        {
            return false;
        }
    }
    return true;
}

bool BookSourceMgrDlg::isBookSourceExist(const BookSource& source, int exceptIndex)
{
    auto& sources = Settings::instance().data().sources;

    for (int i = 0; i < (int)sources.size(); i++)
    {
        if (i == exceptIndex)
        {
            continue;
        }
        if (source.title == sources[i].title || source.host == sources[i].host)
        {
            return true;
        }
    }
    return false;
}

bool BookSourceMgrDlg::isBookSourceModified(const BookSource& source)
{
    auto& sources = Settings::instance().data().sources;
    if (m_currentIndex >= 0 && m_currentIndex < (int)sources.size())
    {
        if (source != sources[m_currentIndex])
        {
            return true;
        }
    }
    return false;
}

LRESULT BookSourceMgrDlg::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return onInitDialog();

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            return onAddButtonClick();
        case IDCANCEL:
            return onCloseButtonClick();
        case IDC_SAVE:
            return onSaveButtonClick();
        case IDC_EXPORT:
            return onExportButtonClick();
        case IDC_IMPORT:
            return onImportButtonClick();
        case IDC_MANUAL:
            return onManualButtonClick();
        case IDC_SYNC:
            return onSyncButtonClick();
        case IDC_COMBO_METHOD:
        case IDC_COMBO_LISTPAGE:
        case IDC_COMBO_CTX_NEXT:
        case IDC_COMBO_CPT_NEXT:
        case IDC_COMBO_FILTER:
            onComboBoxChanged();
            return TRUE;
        }
        break;

    case WM_NOTIFY:
        if (LOWORD(wParam) == IDC_LIST_BOOKSRC)
        {
            if (m_list) return m_list->onNotify(wParam, lParam);
        }
        break;

    case WM_LIST_SELECTED:
        return onListSelected((int)wParam, (BOOL)lParam);
    case WM_LIST_MOVEUP:
        return onListMoveUp((int)wParam);
    case WM_LIST_MOVEDOWN:
        return onListMoveDown((int)wParam);
    case WM_LIST_MOVETOP:
        return onListMoveTop((int)wParam);
    case WM_LIST_MOVEBOTTOM:
        return onListMoveBottom((int)wParam);
    case WM_LIST_DELETE:
        return onListDelete((int)wParam);
    case WM_LIST_CLEAR:
        return onListClear();

    case WM_REQCOMPLETER:
        return onReqCompleter(wParam, lParam);

    case WM_DESTROY:
    {
        MSG msg;
        while (PeekMessage(&msg, m_hWnd, WM_REQCOMPLETER, WM_REQCOMPLETER, PM_REMOVE))
        {
            if (msg.lParam != NULL)
            {
                conv_free((char*)msg.lParam);
            }
        }
        break;
    }

    default:
        break;
    }
    return (INT_PTR)FALSE;
}
#endif