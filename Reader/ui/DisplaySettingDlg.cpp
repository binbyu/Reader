#include "DisplaySettingDlg.h"
#include "IRenderer.h"
#include "resource.h"
#include "DpiAwareness.h"
#include "Utils.h"

DisplaySettingDlg::DisplaySettingDlg(HINSTANCE hInstance, HWND hParent)
    : Dialog(hInstance, hParent, IDD_DISPLAYSETTING)
    , m_pickerCursor(LoadCursor(hInstance, MAKEINTRESOURCE(IDC_CURSOR_PICKER)))
    , m_isPickingColor(false)
    , m_changed(false)
    , m_ignoreAlphaEditChange(false)
{
    auto &data = Settings::instance().data();
    m_styles = data.styles;
    m_alpha = data.window.alpha;
    m_textAlpha = data.window.textAlpha;
    m_background = data.background;
    m_layout = data.layout;
}

DisplaySettingDlg::~DisplaySettingDlg()
{
}

bool DisplaySettingDlg::isChanged() const
{
    return m_changed;
}

LRESULT DisplaySettingDlg::onInitDialog()
{
    // font settings
    auto &data = Settings::instance().data();
    CheckDlgButton(m_hWnd, IDC_CHECK_SAMETEXTSTYLE, m_styles.unified ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hWnd, IDC_CHECK_MENU_FONT, m_styles.viewFont ? BST_CHECKED : BST_UNCHECKED);
    EnableWindow(GetDlgItem(m_hWnd, IDC_BUTTON_CHAPTER_FONT), !m_styles.unified);
    bindSliderEditPair(IDC_SLIDER_TEXTALPHA, IDC_EDIT_TEXTALPHA, m_textAlpha, MIN_TEXT_ALPHA, MAX_TEXT_ALPHA);

    // background settings
    CheckDlgButton(m_hWnd, IDC_CHECK_BIENABLE, m_background.enableImage ? BST_CHECKED : BST_UNCHECKED);
    SetDlgItemTextW(m_hWnd, IDC_STATIC_RGB, Utils::toRGB(m_background.color).c_str());
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_BIMODE), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_STRETCH).c_str());
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_BIMODE), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_TILE).c_str());
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_BIMODE), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_TILEFLIP).c_str());
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_BIMODE), CB_SETCURSEL, (int)m_background.mode, NULL);
    SetDlgItemTextW(m_hWnd, IDC_EDIT_BIFILE, m_background.fileName.c_str());
    EnableWindow(GetDlgItem(m_hWnd, IDC_COMBO_BIMODE), m_background.enableImage);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_BIFILE), m_background.enableImage);
    EnableWindow(GetDlgItem(m_hWnd, IDC_BUTTON_BISEL), m_background.enableImage);
    bindSliderEditPair(IDC_SLIDER_BGALPHA, IDC_EDIT_BGALPHA, m_alpha, MIN_ALPHA, MAX_ALPHA);

    // layout settings
    SetDlgItemInt(m_hWnd, IDC_EDIT_CHARGAP, m_layout.charGap, FALSE);
    SetDlgItemInt(m_hWnd, IDC_EDIT_LINEGAP, m_layout.lineGap, FALSE);
    SetDlgItemInt(m_hWnd, IDC_EDIT_PARAGRAPHGAP, m_layout.paragraphGap, FALSE);
    SetDlgItemTextW(m_hWnd, IDC_EDIT_BORDER, getBorder({m_layout.borderL, m_layout.borderT, m_layout.borderR, m_layout.borderB}).c_str());
    CheckDlgButton(m_hWnd, IDC_CHECK_WORDWRAP, m_layout.wordWrap ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hWnd, IDC_CHECK_INDENT, m_layout.indent ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hWnd, IDC_CHECK_BLANKLINES, m_layout.noBlansLine ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(m_hWnd, IDC_CHECK_SPLITCHAPTER, m_layout.splitChapter ? BST_CHECKED : BST_UNCHECKED);
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_ALIGNMENT), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_LEFT).c_str());
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_ALIGNMENT), CB_ADDSTRING, 0, (LPARAM)loadString(IDS_CENTER).c_str());
    SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_ALIGNMENT), CB_SETCURSEL, (int)m_layout.alignment, NULL);

    centerParent();
    return TRUE;
}

LRESULT DisplaySettingDlg::onSave()
{
    if (!loadData())
    {
        return FALSE;
    }

    auto &data = Settings::instance().data();
    if (m_alpha != data.window.alpha)
    {
        m_changed = true;
        data.window.alpha = m_alpha;
    }
    if (m_textAlpha != data.window.textAlpha)
    {
        m_changed = true;
        data.window.textAlpha = m_textAlpha;
    }
    if (m_background.enableImage != data.background.enableImage
        || (m_background.enableImage && (m_background.fileName != data.background.fileName || m_background.mode != data.background.mode))
        || (!m_background.enableImage && m_background.color != data.background.color))
    {
        m_changed = true;
    }
    data.background = m_background;
    if (m_layout != data.layout)
    {
        m_changed = true;
        data.layout = m_layout;
    }
    if (m_styles != data.styles)
    {
        m_changed = true;
        data.styles = m_styles;
    }

    Settings::instance().save();
    EndDialog(m_hWnd, IDOK);
    return TRUE;
}

LRESULT DisplaySettingDlg::onUnifiedTextStyle()
{
    m_styles.unified = IsDlgButtonChecked(m_hWnd, IDC_CHECK_SAMETEXTSTYLE) == BST_CHECKED;
    EnableWindow(GetDlgItem(m_hWnd, IDC_BUTTON_CHAPTER_FONT), !m_styles.unified);
    updatePreview();
    return TRUE;
}

LRESULT DisplaySettingDlg::onTextFont()
{
    if (openFontDialog(m_hWnd, m_styles.textFont, m_styles.textColor))
    {
        updatePreview();
    }
    return TRUE;
}

LRESULT DisplaySettingDlg::onTitleFont()
{
    if (openFontDialog(m_hWnd, m_styles.titleFont, m_styles.titleColor))
    {
        updatePreview();
    }
    return TRUE;
}

LRESULT DisplaySettingDlg::onBackgroundColor()
{
    if (openColorDialog(m_hWnd, m_background.color))
    {
        updatePreview();
    }
    return TRUE;
}

LRESULT DisplaySettingDlg::onBackgroundImageEnable()
{
    m_background.enableImage = IsDlgButtonChecked(m_hWnd, IDC_CHECK_BIENABLE) == BST_CHECKED;
    EnableWindow(GetDlgItem(m_hWnd, IDC_COMBO_BIMODE), m_background.enableImage);
    EnableWindow(GetDlgItem(m_hWnd, IDC_EDIT_BIFILE), m_background.enableImage);
    EnableWindow(GetDlgItem(m_hWnd, IDC_BUTTON_BISEL), m_background.enableImage);
    updatePreview();
    return TRUE;
}

LRESULT DisplaySettingDlg::onBrowserBackgroundImage()
{
    if (openFileDialog(m_background.fileName))
    {
        SetDlgItemTextW(m_hWnd, IDC_EDIT_BIFILE, m_background.fileName.c_str());
    }
    return TRUE;
}

LRESULT DisplaySettingDlg::onBackgroundImageFileChanged()
{
    m_background.fileName = getDlgItemTextW(IDC_EDIT_BIFILE);
    updatePreview();
    return TRUE;
}

LRESULT DisplaySettingDlg::onBackgroundImageModeChanged()
{
    m_background.mode = (WrapMode)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_BIMODE), CB_GETCURSEL, 0, NULL);
    updatePreview();
    return TRUE;
}

LRESULT DisplaySettingDlg::onBackgroundAlphaChanged()
{
    if (m_ignoreAlphaEditChange)
    {
        return TRUE;
    }
    int value = GetDlgItemInt(m_hWnd, IDC_EDIT_BGALPHA, NULL, FALSE);
    if (value < 1) value = 1;
    if (value > 255) value = 255;
    m_alpha = (BYTE)value;
    handleEditChange(IDC_EDIT_BGALPHA, IDC_SLIDER_BGALPHA, m_alpha, 1, 255);
    return TRUE;
}

LRESULT DisplaySettingDlg::onTextAlphaChanged()
{
    if (m_ignoreAlphaEditChange)
    {
        return TRUE;
    }
    int value = GetDlgItemInt(m_hWnd, IDC_EDIT_TEXTALPHA, NULL, FALSE);
    if (value < 1) value = 1;
    if (value > 255) value = 255;
    m_textAlpha = (BYTE)value;
    handleEditChange(IDC_EDIT_TEXTALPHA, IDC_SLIDER_TEXTALPHA, m_textAlpha, 1, 255);
    return TRUE;
}

LRESULT DisplaySettingDlg::onUpdatePreview(HDC hdc, RECT rect)
{
    // fill background
    HDC hMemdc = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top);
    SelectObject(hMemdc, hBitmap);
    FillRect(hMemdc, &rect, GetSysColorBrush(COLOR_BTNFACE));

    const wchar_t* TITLE1 = L"\u6807\u9898\u9884\u89c8";
    const wchar_t* TITLE2 = L"Title preview";
    const wchar_t* TEXT1  = L"\u6b63\u6587\u9884\u89c8\r\n";
    const wchar_t* TEXT2  = L"Text preview";

    TextStyles textStyles(2);
    textStyles[0] = TextStyle(m_styles.textFont, m_textAlpha << 24 | m_styles.textColor);
    textStyles[1] = TextStyle(m_styles.titleFont, m_textAlpha << 24 | m_styles.titleColor);

    IRenderer::instance()->beginPaint(m_hWnd, hMemdc, rect, textStyles, true);
    if (m_background.enableImage)
    {
        Image* image1 = IRenderer::instance()->loadImageFromPath(m_background.fileName.c_str());
        Image* image2 = IRenderer::instance()->createImageFromBGImage(image1, m_alpha, rect.right - rect.left, rect.bottom - rect.top, m_background.mode);
        delete image1;
        IRenderer::instance()->drawImage(image2, rect);
        delete image2;
    }
    else
    {
        IRenderer::instance()->fillRect(m_alpha << 24 | m_background.color, rect);
    }

    int borderL = m_layout.borderL;
    int borderT = m_layout.borderT;
    int borderR = m_layout.borderR;
    int borderB = m_layout.borderB;
    int charGap = m_layout.charGap;
    int lineGap = m_layout.lineGap;
    int paragraphGap = m_layout.paragraphGap;
    DpiAwareness::instance().logicalToPhysical(borderL, borderL);
    DpiAwareness::instance().logicalToPhysical(borderT, borderT);
    DpiAwareness::instance().logicalToPhysical(borderR, borderR);
    DpiAwareness::instance().logicalToPhysical(borderB, borderB);
    DpiAwareness::instance().logicalToPhysical(charGap, charGap);
    DpiAwareness::instance().logicalToPhysical(lineGap, lineGap);
    DpiAwareness::instance().logicalToPhysical(paragraphGap, paragraphGap);

    int maxWidth = rect.right - rect.left - borderL - borderR;
    int maxHeight = rect.bottom - rect.top - borderT - borderB;
    IMeasurer* measurer = IRenderer::instance()->measurer();
    auto tmTile1 = measurer->measureText(TITLE1, -1, maxWidth, m_styles.unified ? 0 : 1);
    auto tmTile2 = measurer->measureText(TITLE2, -1, maxWidth, m_styles.unified ? 0 : 1);
    auto tmText1 = measurer->measureText(TEXT1, -1, maxWidth, 0);
    auto tmText2 = measurer->measureText(TEXT2, -1, maxWidth, 0);
    int totalHeight = tmTile1.height + tmTile2.height + tmText1.height + tmText2.height + lineGap * 3;

    RECT rcDraw1, rcDraw2, rcDraw3, rcDraw4;
    rcDraw1.left = (rect.right - rect.left - tmTile1.width) / 2;
    rcDraw1.right = rcDraw1.left + tmTile1.width;
    rcDraw1.top = m_layout.borderT + (rect.bottom - rect.top - totalHeight) / 2;
    rcDraw1.bottom = rcDraw1.top + tmTile1.height;
    rcDraw2.left = (rect.right - rect.left - tmText1.width) / 2;
    rcDraw2.right = rcDraw2.left + tmText1.width;
    rcDraw2.top = rcDraw1.bottom + lineGap;
    rcDraw2.bottom = rcDraw2.top + tmText1.height;
    rcDraw3.left = (rect.right - rect.left - tmTile2.width) / 2;
    rcDraw3.right = rcDraw3.left + tmTile2.width;
    rcDraw3.top = rcDraw2.bottom + lineGap;
    rcDraw3.bottom = rcDraw3.top + tmTile2.height;
    rcDraw4.left = (rect.right - rect.left - tmText2.width) / 2;
    rcDraw4.right = rcDraw4.left + tmText2.width;
    rcDraw4.top = rcDraw3.bottom + lineGap;
    rcDraw4.bottom = rcDraw4.top + tmText2.height;

    IRenderer::instance()->drawText(TITLE1, -1, rcDraw1, m_styles.unified ? 0 : 1);
    IRenderer::instance()->drawText(TEXT1, -1, rcDraw2, 0);
    IRenderer::instance()->drawText(TITLE2, -1, rcDraw3, m_styles.unified ? 0 : 1);
    IRenderer::instance()->drawText(TEXT2, -1, rcDraw4, 0);
    IRenderer::instance()->endPaint();

    BitBlt(hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hMemdc, 0, 0, SRCCOPY);
    DeleteObject(hBitmap);
    DeleteDC(hMemdc);
    return (INT_PTR)TRUE;
}

void DisplaySettingDlg::updatePreview()
{
    InvalidateRect(GetDlgItem(m_hWnd, IDC_STATIC_PREVIEW), NULL, FALSE);
}

std::wstring DisplaySettingDlg::getBorder(const RECT &rect)
{
    wchar_t border[32] = {0};
    _snwprintf(border, 32, L"%d,%d,%d,%d", rect.left, rect.top, rect.right, rect.bottom);
    return border;
}

bool DisplaySettingDlg::getBorder(const wchar_t *str, RECT &rect, bool tips)
{
    if (swscanf(str, L"%d,%d,%d,%d", &rect.left, &rect.top, &rect.right, &rect.bottom) == 4)
    {
        return true;
    }
    if (tips)
    {
        messageBox(IDS_INVALID_INBORDER, IDS_WARN, MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_BORDER));
    }
    return false;
}

void DisplaySettingDlg::bindSliderEditPair(int sliderId, int editId, BYTE& value, int minValue, int maxValue)
{
    SendMessage(GetDlgItem(m_hWnd, sliderId), TBM_SETRANGE, TRUE, MAKELPARAM(minValue, maxValue));
    setEditValue(editId, value);
    SendMessage(GetDlgItem(m_hWnd, sliderId), TBM_SETPOS, TRUE, value);
}

void DisplaySettingDlg::setSliderValue(int sliderId, int value)
{
    SendMessage(GetDlgItem(m_hWnd, sliderId), TBM_SETPOS, TRUE, value);
}

void DisplaySettingDlg::setEditValue(int editId, int value)
{
    bool oldIgnore = m_ignoreAlphaEditChange;
    m_ignoreAlphaEditChange = true;

    HWND hEdit = GetDlgItem(m_hWnd, editId);
    wchar_t text[16] = {0};
    _snwprintf(text, ARRAYSIZE(text), L"%d", value);

    wchar_t oldText[16] = {0};
    GetWindowTextW(hEdit, oldText, ARRAYSIZE(oldText));
    if (wcscmp(oldText, text) != 0)
    {
        int selStart = 0;
        int selEnd = 0;
        SendMessage(hEdit, EM_GETSEL, (WPARAM)&selStart, (LPARAM)&selEnd);
        SetWindowTextW(hEdit, text);
        int newLen = lstrlenW(text);
        if (selStart > newLen) selStart = newLen;
        if (selEnd > newLen) selEnd = newLen;
        SendMessage(hEdit, EM_SETSEL, selStart, selEnd);
    }

    m_ignoreAlphaEditChange = oldIgnore;
}

bool DisplaySettingDlg::handleSliderChange(int sliderId, int editId, BYTE& value)
{
    value = (BYTE)SendMessage(GetDlgItem(m_hWnd, sliderId), TBM_GETPOS, 0, 0);
    setEditValue(editId, value);
    updatePreview();
    return true;
}

bool DisplaySettingDlg::handleEditChange(int editId, int sliderId, BYTE& value, int minValue, int maxValue)
{
    if (m_ignoreAlphaEditChange)
    {
        return true;
    }

    int rawValue = GetDlgItemInt(m_hWnd, editId, NULL, FALSE);
    int newValue = rawValue;
    if (newValue < minValue) newValue = minValue;
    if (newValue > maxValue) newValue = maxValue;
    BYTE newByte = (BYTE)newValue;
    value = newByte;
    setSliderValue(sliderId, value);
    if (newValue != rawValue)
    {
        setEditValue(editId, value);
    }
    updatePreview();
    return true;
}

void DisplaySettingDlg::startPickColor()
{
    if (!m_isPickingColor)
    {
        m_isPickingColor = true;
        SetCapture(m_hWnd);
        SetCursor(m_pickerCursor);
    }
}

void DisplaySettingDlg::stopPickColor()
{
    if (m_isPickingColor)
    {
        m_isPickingColor = false;
        ReleaseCapture();
        SetCursor(NULL);
    }
}

void DisplaySettingDlg::updatePickColor(COLORREF color)
{
    m_background.color = color;
    SetDlgItemTextW(m_hWnd, IDC_STATIC_RGB, Utils::toRGB(m_background.color).c_str());
    updatePreview();
}

bool DisplaySettingDlg::openFileDialog(std::wstring& fileName)
{
    wchar_t szFile[MAX_PATH] = { 0 };
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = L"Image Files\0*.bmp;*.jpg;*.jpeg;*.png;*.gif\0All Files\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    if (GetOpenFileNameW(&ofn))
    {
        fileName = szFile;
        return true;
    }
    return false;
}

bool DisplaySettingDlg::loadData()
{
    std::wstring borderText = getDlgItemTextW(IDC_EDIT_BORDER);
    RECT borderRect;
    if (!getBorder(borderText.c_str(), borderRect, true))
    {
        messageBox(IDS_INVALID_INBORDER, IDS_WARN, MB_OK | MB_ICONWARNING);
        SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_BORDER));
        return false;
    }
    std::wstring imageFile = getDlgItemTextW(IDC_EDIT_BIFILE);

    m_layout.borderL = borderRect.left;
    m_layout.borderT = borderRect.top;
    m_layout.borderR = borderRect.right;
    m_layout.borderB = borderRect.bottom;
    BOOL result = FALSE;
    m_layout.charGap = GetDlgItemInt(m_hWnd, IDC_EDIT_CHARGAP, &result, FALSE);
    if (!result)
    {
        messageBox(IDS_INVALID_CHARGAP, IDS_WARN, MB_OK|MB_ICONWARNING);
        SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_CHARGAP));
        return false;
    }
    m_layout.lineGap = GetDlgItemInt(m_hWnd, IDC_EDIT_LINEGAP, &result, FALSE);
    if (!result)
    {
        messageBox(IDS_INVALID_LINEGAP, IDS_WARN, MB_OK|MB_ICONWARNING);
        SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_LINEGAP));
        return false;
    }
    m_layout.paragraphGap = GetDlgItemInt(m_hWnd, IDC_EDIT_PARAGRAPHGAP, &result, FALSE);
    if (!result)
    {
        messageBox(IDS_INVALID_PARAGRAPHGAP, IDS_WARN, MB_OK|MB_ICONWARNING);
        SetFocus(GetDlgItem(m_hWnd, IDC_EDIT_PARAGRAPHGAP));
        return false;
    }
    m_layout.indent = IsDlgButtonChecked(m_hWnd, IDC_CHECK_INDENT) == BST_CHECKED;
    m_layout.noBlansLine = IsDlgButtonChecked(m_hWnd, IDC_CHECK_BLANKLINES) == BST_CHECKED;
    m_layout.wordWrap = IsDlgButtonChecked(m_hWnd, IDC_CHECK_WORDWRAP) == BST_CHECKED;
    m_layout.splitChapter = IsDlgButtonChecked(m_hWnd, IDC_CHECK_SPLITCHAPTER) == BST_CHECKED;
    m_layout.alignment = (LayoutConfig::Alignment)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_ALIGNMENT), CB_GETCURSEL, 0, NULL);

    m_styles.unified = IsDlgButtonChecked(m_hWnd, IDC_CHECK_SAMETEXTSTYLE) == BST_CHECKED;
    m_styles.viewFont = IsDlgButtonChecked(m_hWnd, IDC_CHECK_MENU_FONT) == BST_CHECKED;
    m_background.enableImage = IsDlgButtonChecked(m_hWnd, IDC_CHECK_BIENABLE) == BST_CHECKED;
    m_background.fileName = imageFile;
    if (m_background.enableImage)
    {
        if (m_background.fileName.empty())
        {
            messageBox(IDS_PLS_SEL_IMG, IDS_WARN, MB_OK|MB_ICONWARNING);
            return false;
        }
        std::wstring ext = Utils::fileExt(m_background.fileName);
        if (ext != L".jpg" && ext != L".png" && ext != L".bmp" && ext != L".jpeg")
        {
            messageBox(IDS_INVALID_IMG, IDS_WARN, MB_OK | MB_ICONWARNING);
            return false;
        }
        if (!Utils::isFileExist(m_background.fileName))
        {
            messageBox(IDS_INVALID_IMG, IDS_WARN, MB_OK|MB_ICONWARNING);
            return false;
        }
    }
    m_background.mode = (WrapMode)SendMessage(GetDlgItem(m_hWnd, IDC_COMBO_BIMODE), CB_GETCURSEL, 0, NULL);
    // m_background.color is updated in real time in UpdatePickColor and OpenColorDialog, no need to get it here
    m_alpha = (BYTE)GetDlgItemInt(m_hWnd, IDC_EDIT_BGALPHA, NULL, FALSE);
    m_textAlpha = (BYTE)GetDlgItemInt(m_hWnd, IDC_EDIT_TEXTALPHA, NULL, FALSE);
    // m_textStyle is updated in real time in OpenFontDialog, no need to get it here
    return true;
}

LRESULT DisplaySettingDlg::handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        return onInitDialog();

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            return onSave();
        case IDCANCEL:
            EndDialog(m_hWnd, LOWORD(wParam));
            return (INT_PTR)TRUE;
        case IDC_CHECK_SAME:
            return onUnifiedTextStyle();
        case IDC_BUTTON_TEXT_FONT:
            return onTextFont();
        case IDC_BUTTON_CHAPTER_FONT:
            return onTitleFont();
        case IDC_BUTTON_BGCOLOR:
            return onBackgroundColor();
        case IDC_CHECK_BIENABLE:
            return onBackgroundImageEnable();
        case IDC_BUTTON_BISEL:
            return onBrowserBackgroundImage();
        case IDC_EDIT_BIFILE:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                return onBackgroundImageFileChanged();
            }
            break;
        case IDC_COMBO_BIMODE:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                return onBackgroundImageModeChanged();
            }
            break;
        case IDC_EDIT_BGALPHA:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                return onBackgroundAlphaChanged();
            }
            break;
        case IDC_EDIT_TEXTALPHA:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                return onTextAlphaChanged();
            }
            break;
        case IDC_EDIT_LINEGAP:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                m_layout.lineGap = GetDlgItemInt(m_hWnd, IDC_EDIT_LINEGAP, NULL, FALSE);
                updatePreview();
            }
            break;
        case IDC_EDIT_CHARGAP:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                m_layout.charGap = GetDlgItemInt(m_hWnd, IDC_EDIT_CHARGAP, NULL, FALSE);
                updatePreview();
            }
            break;
        case IDC_EDIT_PARAGRAPHGAP:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                m_layout.paragraphGap = GetDlgItemInt(m_hWnd, IDC_EDIT_PARAGRAPHGAP, NULL, FALSE);
                updatePreview();
            }
            break;
        case IDC_EDIT_BORDER:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                std::wstring borderText = getDlgItemTextW(IDC_EDIT_BORDER);
                RECT borderRect;
                if (getBorder(borderText.c_str(), borderRect))
                {
                    m_layout.borderL = borderRect.left;
                    m_layout.borderT = borderRect.top;
                    m_layout.borderR = borderRect.right;
                    m_layout.borderB = borderRect.bottom;
                    updatePreview();
                }
            }
            break;
        default:
            break;
        }
        break;
    case WM_HSCROLL:
        if ((HWND)lParam == GetDlgItem(m_hWnd, IDC_SLIDER_BGALPHA))
        {
            handleSliderChange(IDC_SLIDER_BGALPHA, IDC_EDIT_BGALPHA, m_alpha);
            return (INT_PTR)TRUE;
        }
        else if ((HWND)lParam == GetDlgItem(m_hWnd, IDC_SLIDER_TEXTALPHA))
        {
            handleSliderChange(IDC_SLIDER_TEXTALPHA, IDC_EDIT_TEXTALPHA, m_textAlpha);
            return (INT_PTR)TRUE;
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
    case WM_LBUTTONDOWN:
    {
        POINT pt = {LOWORD(lParam), HIWORD(lParam)};
        ClientToScreen(m_hWnd, &pt);
        RECT rect;
        GetWindowRect(GetDlgItem(m_hWnd, IDC_STATIC_PICKER), &rect);
        if (PtInRect(&rect, pt))
        {
            startPickColor();
        }
        break;
    }

    case WM_LBUTTONUP:
        stopPickColor();
        break;

    case WM_MOUSEMOVE:
        if (m_isPickingColor)
        {
            POINT pt = {LOWORD(lParam), HIWORD(lParam)};
            ClientToScreen(m_hWnd, &pt);
            HDC hdc = GetDC(NULL);
            COLORREF color = GetPixel(hdc, pt.x, pt.y);
            ReleaseDC(NULL, hdc);
            updatePickColor(color);
        }
        break;

    default:
        break;
    }
    return (INT_PTR)FALSE;
}

UINT_PTR CALLBACK DisplaySettingDlg::chooseFontProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
    {
        CHOOSEFONT* cf = (CHOOSEFONT*)lParam;
        COLORREF textColor = (COLORREF)cf->lCustData;
        HWND hComb = GetDlgItem(hWnd, 0x473);
        SendMessage(hComb, CB_SETITEMDATA, 0, textColor);
        SendMessage(hComb, CB_SETCURSEL, 0, textColor);
        SetWindowSubclass(hComb, chooseFontComboProc, 0, 0);
        break;
    }
    }
    return (INT_PTR)FALSE;
}

LRESULT CALLBACK DisplaySettingDlg::chooseFontComboProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubClass, DWORD_PTR udata)
{
    switch (message)
    {
    case WM_LBUTTONDOWN:
    {
        DisplaySettingDlg *pThis = getThis<DisplaySettingDlg>(GetParent(GetParent(hWnd)));
        COLORREF color = (COLORREF)SendMessage(hWnd, CB_GETITEMDATA, 0, 0);
        if (pThis->openColorDialog(hWnd, color))
        {
            SendMessage(hWnd, CB_SETITEMDATA, 0, color);
            SendMessage(hWnd, CB_SETCURSEL, 0, color);
            InvalidateRect(GetParent(hWnd), NULL, TRUE);
        }
        return 0;
    }
    case CB_GETLBTEXTLEN:
        return 7;
    case CB_GETLBTEXT:
    {
        COLORREF color = (COLORREF)SendMessage(hWnd, CB_GETITEMDATA, 0, 0);
        std::wstring rgb = Utils::toRGB(color);
        _swprintf((TCHAR *)lParam, rgb.c_str());
        return 7;
    }
    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, chooseFontComboProc, uIdSubClass);
        break;
    }
    return DefSubclassProc(hWnd, message, wParam, lParam);
}

bool DisplaySettingDlg::openFontDialog(HWND hWnd, LOGFONTW& font, COLORREF& textColor)
{
    CHOOSEFONT cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.lStructSize = sizeof(cf);
    cf.hwndOwner = hWnd;
    cf.lpLogFont = &font;
    cf.rgbColors = textColor;
    cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_EFFECTS | CF_NOVERTFONTS | CF_ENABLEHOOK;
    cf.lpfnHook = DisplaySettingDlg::chooseFontProc;
    cf.lCustData = (LPARAM)textColor;
    if (ChooseFontW(&cf))
    {
        textColor = cf.rgbColors;
        font.lfQuality = PROOF_QUALITY;
        return true;
    }
    return false;
}

bool DisplaySettingDlg::openColorDialog(HWND hWnd, COLORREF& color)
{
    CHOOSECOLOR cc;
    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hWnd;
    cc.lpCustColors = &(Settings::instance().data().customColors[0]);
    cc.rgbResult = color;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;
    if (ChooseColorW(&cc))
    {
        color = cc.rgbResult;
        return true;
    }
    return false;
}