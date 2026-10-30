#if ENABLE_TAG
#include "TagItemDlg.h"
#include "resource.h"
#include "DisplaySettingDlg.h"

TagItemDlg::TagItemDlg(HINSTANCE hInstance, HWND hParent)
    : Dialog(hInstance, hParent, IDD_TAGITEM)
    , m_index(-1)
{
    m_item.enable = true;
    Setting::instance().defaultFont(m_item.font);
    m_item.textColor = DEFAULT_TEXT_COLOR;
    m_item.backgroundColor = DEFAULT_TEXTBG_COLOR;
}

TagItemDlg::~TagItemDlg()
{
}

void TagItemDlg::SetTagIndex(int index)
{
    m_index = index;
    const auto& tags = Setting::instance().data().tags;
    if (m_index >= 0 && m_index < (int)tags.size())
    {
        m_item = tags[m_index];
    }
}

const TagInfo& TagItemDlg::GetTagItem() const
{
    return m_item;
}

LRESULT TagItemDlg::onInitDialog()
{
    SetDlgItemTextW(m_hWnd, IDC_EDIT_TAGKW, m_item.keyword.c_str());
    centerParent();
    return TRUE;
}

LRESULT TagItemDlg::onCloseButtonClick(WPARAM wParam)
{
    EndDialog(m_hWnd, LOWORD(wParam));
    return TRUE;
}

LRESULT TagItemDlg::onSaveButtonClick()
{
    if (!isValid())
    {
        return TRUE;
    }
    if (isExist())
    {
        return TRUE;
    }

    auto& tags = Setting::instance().data().tags;
    if (m_index >= 0 && m_index < (int)tags.size()) // edit
    {
        tags[m_index] = m_item;
        Setting::instance().save();
    }
    else // add
    {
        tags.push_back(m_item);
        Setting::instance().save();
    }

    EndDialog(m_hWnd, IDOK);
    return TRUE;
}

LRESULT TagItemDlg::onTextFontButtonClick()
{
    if (DisplaySettingDlg::openFontDialog(m_hWnd, m_item.font, m_item.textColor))
    {
        updatePreview();
    }
    return TRUE;
}

LRESULT TagItemDlg::onBGColorButtonClick()
{
    if (DisplaySettingDlg::openColorDialog(m_hWnd, m_item.backgroundColor))
    {
        updatePreview();
    }
    return TRUE;
}

LRESULT TagItemDlg::onKeywordChanged()
{
    m_item.keyword = getDlgItemTextW(IDC_EDIT_TAGKW);
    updatePreview();
    return TRUE;
}

LRESULT TagItemDlg::onUpdatePreview(HDC hdc, RECT rect)
{
    TextStyle ts(m_item.font, m_item.textColor | 0xff000000, m_item.backgroundColor | 0xff000000);

    // fill background
    HDC hMemdc = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top);
    SelectObject(hMemdc, hBitmap);
    FillRect(hMemdc, &rect, GetSysColorBrush(COLOR_BTNFACE));

    IRenderer::instance()->beginPaint(m_hWnd, hMemdc, rect, { ts }, true);

    IMeasurer* measurer = IRenderer::instance()->measurer();
    auto tmTile = measurer->measureText(m_item.keyword.c_str(), (int)m_item.keyword.size(), rect.right - rect.left, 0);
    RECT rcDraw;
    rcDraw.left = (rect.right - rect.left - tmTile.width) / 2;
    rcDraw.right = rcDraw.left + tmTile.width;
    rcDraw.top = (rect.bottom - rect.top - tmTile.height) / 2;
    rcDraw.bottom = rcDraw.top + tmTile.height;
    IRenderer::instance()->drawText(m_item.keyword.c_str(), (int)m_item.keyword.size(), rcDraw, 0);

    IRenderer::instance()->endPaint();

    BitBlt(hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hMemdc, 0, 0, SRCCOPY);
    DeleteObject(hBitmap);
    DeleteDC(hMemdc);
    return TRUE;
}

void TagItemDlg::updatePreview()
{
    InvalidateRect(GetDlgItem(m_hWnd, IDC_STATIC_PREVIEW), NULL, FALSE);
}

bool TagItemDlg::isExist()
{
    const auto& tags = Setting::instance().data().tags;
    for (int i = 0; i < (int)tags.size();i++)
    {
        if (i != m_index && tags[i].keyword == m_item.keyword)
        {
            messageBox(IDS_TAGITEM_EXIST, IDS_ERROR, MB_ICONERROR | MB_OK);
            SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_TAGKW));
            return true;
        }
    }
    return false;
}

bool TagItemDlg::isValid()
{
    if (m_item.keyword.empty())
    {
        messageBox(IDS_EMPTY_KEYWORD, IDS_ERROR, MB_ICONERROR | MB_OK);
        SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_TAGKW));
        return false;
    }
    return true;
}

LRESULT TagItemDlg::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return onInitDialog();

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            return onSaveButtonClick();
        case IDCANCEL:
            return onCloseButtonClick(wParam);
        case IDC_BUTTON_TAGFONT:
            return onTextFontButtonClick();
        case IDC_BUTTON_TAGBG:
            return onBGColorButtonClick();
        case IDC_EDIT_TAGKW:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                return onKeywordChanged();
            }
            break;
        }
        break;

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