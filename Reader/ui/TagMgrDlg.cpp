#if ENABLE_TAG
#include "TagMgrDlg.h"
#include "resource.h"
#include "TagItemDlg.h"
#include "Settings.h"
#include "IRenderer.h"

TagMgrDlg::TagMgrDlg(HINSTANCE hInstance, HWND hParent)
    : Dialog(hInstance, hParent, IDD_TAGMGR)
    , m_list(nullptr)
{
}

TagMgrDlg::~TagMgrDlg()
{
    if (m_list)
    {
        delete m_list;
        m_list = nullptr;
    }
}

LRESULT TagMgrDlg::onInitDialog()
{
    HWND hList = GetDlgItem(m_hWnd, IDC_LIST_TAGSET);
    m_list = new ListView(hList, this);
    m_list->setCheckBox(true);
    m_list->setContextMenuType(CONTEXT_MENU_DELETE | CONTEXT_MENU_EDIT | CONTEXT_MENU_CLEAR | CONTEXT_MENU_MOVE);
    m_list->addHeader(IDS_KEYWORD, 130);
    m_list->addHeader(IDS_FONT, 130);
    m_list->addHeader(IDS_TEXT_COLOR, 80);
    m_list->addHeader(IDS_BG_COLOR, 80);

    updateTagList(0);
    centerParent();
    return TRUE;
}

LRESULT TagMgrDlg::onCloseButtonClick(WPARAM wParam)
{
    Setting::instance().save();
    EndDialog(m_hWnd, LOWORD(wParam));
    return TRUE;
}

LRESULT TagMgrDlg::onAddButtonClick()
{
    TagItemDlg dlg(m_hInstance, m_hWnd);
    if (IDOK == dlg.exec())
    {
        updateTagList((int)Setting::instance().data().tags.size() - 1);
    }
    return TRUE;
}

LRESULT TagMgrDlg::onListSelected(int index, BOOL selected)
{
    updatePreview();
    return TRUE;
}

LRESULT TagMgrDlg::onListChecked(int index, BOOL checked)
{
    auto& tags = Setting::instance().data().tags;
    if (index >= 0 && index < (int)tags.size())
    {
        tags[index].enable = checked;
    }
    Setting::instance().save();
    return (INT_PTR)TRUE;
}

LRESULT TagMgrDlg::onListMoveUp(int index)
{
    auto& tags = Setting::instance().data().tags;
    if (index > 0 && index < (int)tags.size())
    {
        std::swap(tags[index], tags[index - 1]);
        Setting::instance().save();
    }
    return (INT_PTR)TRUE;
}

LRESULT TagMgrDlg::onListMoveDown(int index)
{
    auto& tags = Setting::instance().data().tags;
    if (index >= 0 && index < (int)tags.size() - 1)
    {
        std::swap(tags[index], tags[index + 1]);
        Setting::instance().save();
    }
    return (INT_PTR)TRUE;
}

LRESULT TagMgrDlg::onListMoveTop(int index)
{
    auto& tags = Setting::instance().data().tags;
    if (index > 0 && index < (int)tags.size())
    {
        std::rotate(tags.begin(), tags.begin() + index, tags.begin() + index + 1);
        Setting::instance().save();
    }
    return (INT_PTR)TRUE;
}

LRESULT TagMgrDlg::onListMoveBottom(int index)
{
    auto& tags = Setting::instance().data().tags;
    if (index >= 0 && index < (int)tags.size() - 1)
    {
        std::rotate(tags.begin() + index, tags.begin() + index + 1, tags.end());
        Setting::instance().save();
    }
    return (INT_PTR)TRUE;
}

LRESULT TagMgrDlg::onListDelete(int index)
{
    auto& tags = Setting::instance().data().tags;
    if (index >= 0 && index < (int)tags.size())
    {
        tags.erase(tags.begin() + index);
        Setting::instance().save();
    }
    return (INT_PTR)TRUE;
}

LRESULT TagMgrDlg::onListClear()
{
    Setting::instance().data().tags.clear();
    Setting::instance().save();
    return (INT_PTR)TRUE;
}

LRESULT TagMgrDlg::onListEdit(int index)
{
    auto& tags = Setting::instance().data().tags;
    if (index >= 0 && index < (int)tags.size())
    {
        TagItemDlg dlg(m_hInstance, m_hWnd);
        dlg.SetTagIndex(index);
        if (IDOK == dlg.exec())
        {
            updateTagList(index);
        }
    }
    return TRUE;
}

LRESULT TagMgrDlg::onUpdatePreview(HDC hdc, RECT rect)
{
    // fill background
    HDC hMemdc = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top);
    SelectObject(hMemdc, hBitmap);
    FillRect(hMemdc, &rect, GetSysColorBrush(COLOR_BTNFACE));

    const auto& tags = Setting::instance().data().tags;
    int index = m_list->selectedItem();
    if (index >= 0 && index < (int)tags.size())
    {
        const auto& tag = tags[index];
        TextStyle ts(tag.font, tag.textColor | 0xff000000, tag.backgroundColor | 0xff000000);
        IRenderer::instance()->beginPaint(m_hWnd, hMemdc, rect, { ts }, true);

        IMeasurer* measurer = IRenderer::instance()->measurer();
        auto tmTile = measurer->measureText(tag.keyword.c_str(), (int)tag.keyword.size(), rect.right - rect.left, 0);
        RECT rcDraw;
        rcDraw.left = (rect.right - rect.left - tmTile.width) / 2;
        rcDraw.right = rcDraw.left + tmTile.width;
        rcDraw.top = (rect.bottom - rect.top - tmTile.height) / 2;
        rcDraw.bottom = rcDraw.top + tmTile.height;
        IRenderer::instance()->drawText(tag.keyword.c_str(), (int)tag.keyword.size(), rcDraw, 0);

        IRenderer::instance()->endPaint();
    }

    BitBlt(hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hMemdc, 0, 0, SRCCOPY);
    DeleteObject(hBitmap);
    DeleteDC(hMemdc);
    return TRUE;
}

void TagMgrDlg::updateTagList(int index)
{
    m_list->clearItems();
    const auto& tags = Setting::instance().data().tags;
    for (int i = 0; i < (int)tags.size(); i++)
    {
        const TagInfo& tag = tags[i];
        m_list->addItem(i, { tag.keyword,tag.font.lfFaceName, Utils::toRGB(tag.textColor) ,Utils::toRGB(tag.backgroundColor) });
        m_list->setItemChecked(i, tag.enable);
    }
    if (index >= 0 && index < (int)tags.size())
    {
        m_list->setItemSelected(index, true);
    }
}

void TagMgrDlg::updatePreview()
{
    InvalidateRect(GetDlgItem(m_hWnd, IDC_STATIC_PREVIEW), NULL, FALSE);
}

LRESULT TagMgrDlg::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return onInitDialog();

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            return onCloseButtonClick(wParam);
        case IDC_BUTTON_ADD:
            return onAddButtonClick();
        default:
            break;
        }
        break;

    case WM_NOTIFY:
        if (LOWORD(wParam) == IDC_LIST_TAGSET)
        {
            if (m_list) return m_list->onNotify(wParam, lParam);
        }
        break;

    case WM_LIST_SELECTED:
        return onListSelected((int)wParam, (BOOL)lParam);
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
    case WM_LIST_DELETE:
        return onListDelete((int)wParam);
    case WM_LIST_CLEAR:
        return onListClear();
    case WM_LIST_EDIT:
        return onListEdit((int)wParam);

    case WM_DRAWITEM:
    {
        DRAWITEMSTRUCT* di = (DRAWITEMSTRUCT*)lParam;
        if (IDC_STATIC_PREVIEW == di->CtlID)
        {
            return onUpdatePreview(di->hDC, di->rcItem);
        }
        break;
    }

    default:
        break;
    }
    return (INT_PTR)FALSE;
}
#endif