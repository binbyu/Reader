#include "DisplaySet.h"
#include "resource.h"
#include "types.h"
#include "Book.h"
#include <CommDlg.h>
#include <shlwapi.h>
#include <WindowsX.h>

typedef struct display_set_data_t
{
    LOGFONT font;
    u32 font_color;
    LOGFONT font_title;
    u32 font_color_title;
    int use_same_font;
    u32 bg_color;
    bg_image_t bg_image;
    int char_gap;
    int line_gap;
    int paragraph_gap;
    RECT internal_border;
    int meun_font_follow;
    int word_wrap;
    int line_indent;
    int blank_lines;
    int chapter_page;
    int is_save;
} display_set_data_t;

static display_set_data_t _display;
static LOGFONT *_p_font = NULL;
static u32 *_p_font_color = NULL;
static HCURSOR _hCursor = NULL;
static BOOL _is_capturing = FALSE;

extern header_t *_header;
extern HWND _hWnd;
extern HINSTANCE hInst;
extern Book *_Book;

extern int MessageBox_(HWND hWnd, UINT textId, UINT captionId, UINT uType);
extern int MessageBoxFmt_(HWND hWnd, UINT captionId, UINT uType, UINT formatId, ...);
extern BOOL FileExists(TCHAR *file);
extern void Save(HWND hWnd);
extern VOID Invalidate(HWND hWnd, BOOL bOnlyClient, BOOL bErase);
extern void SetTreeviewFont();

static INT_PTR CALLBACK DisplaySetDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
static void _init_font_set(HWND hDlg);
static void _init_bg_color_set(HWND hDlg);
static void _init_bg_image_set(HWND hDlg);
static void _init_layout_set(HWND hDlg);
static void _enable_font_set(HWND hDlg, BOOL enable);
static void _enable_bg_image_set(HWND hDlg, BOOL enable);
static BOOL _valid_font_set(HWND hDlg);
static BOOL _valid_bg_color_set(HWND hDlg);
static BOOL _valid_bg_image_set(HWND hDlg);
static BOOL _valid_layout_set(HWND hDlg);
static void _bg_image_browse(HWND hDlg);
static void _open_fontdlg(HWND hDlg, int id);
static void _open_colordlg(HWND hDlg);
static void _start_color_picker(HWND hDlg);
static void _stop_color_picker(HWND hDlg);
static void _update_bg_rgb(HWND hDlg);
static void _update_preview(HDC hDC, RECT *rc);

#define COPY_DISPLAY_PARAMS(s, d) \
    (d)->font = (s)->font; \
    (d)->font_color = (s)->font_color; \
    (d)->font_title = (s)->font_title; \
    (d)->font_color_title = (s)->font_color_title; \
    (d)->use_same_font = (s)->use_same_font; \
    (d)->bg_color = (s)->bg_color; \
    (d)->bg_image = (s)->bg_image; \
    (d)->char_gap = (s)->char_gap; \
    (d)->line_gap = (s)->line_gap; \
    (d)->paragraph_gap = (s)->paragraph_gap; \
    (d)->internal_border = (s)->internal_border; \
    (d)->meun_font_follow = (s)->meun_font_follow; \
    (d)->word_wrap = (s)->word_wrap; \
    (d)->line_indent = (s)->line_indent; \
    (d)->blank_lines = (s)->blank_lines; \
    (d)->chapter_page = (s)->chapter_page;

void OpenDisplaySetDlg(void)
{
    BOOL reset = FALSE;
    BOOL reset_view = FALSE;
    memset(&_display, 0, sizeof(display_set_data_t));
    COPY_DISPLAY_PARAMS(_header, &_display);

    DialogBox(hInst, MAKEINTRESOURCE(IDD_DISPLAYSET), _hWnd, DisplaySetDlgProc);

    if (_display.is_save)
    {
        // check is need reset&reset_view
        if ((0 != memcmp(&_display.font, &_header->font, sizeof(LOGFONT)) && _display.meun_font_follow)
            || _display.meun_font_follow !=  _header->meun_font_follow)
        {
            reset = TRUE;
            reset_view = TRUE;
        }
        if (!reset)
        {
            if (0 != memcmp(&_display.font_title, &_header->font_title, sizeof(LOGFONT))
                || _display.use_same_font != _header->use_same_font
                || _display.char_gap != _header->char_gap
                || _display.line_gap != _header->line_gap
                || _display.paragraph_gap != _header->paragraph_gap
                || 0 != memcmp(&_display.internal_border, &_header->internal_border, sizeof(RECT))
                || _display.word_wrap != _header->word_wrap
                || _display.line_indent != _header->line_indent
                || _display.blank_lines != _header->blank_lines
                || _display.chapter_page != _header->chapter_page)
            {
                reset = TRUE;
            }
        }

        COPY_DISPLAY_PARAMS(&_display, _header);

        Save(_hWnd);
        Invalidate(_hWnd, TRUE, FALSE);

        if (reset_view)
            SetTreeviewFont();
    }
}

static INT_PTR CALLBACK DisplaySetDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    DRAWITEMSTRUCT* draw;
    int res;
    POINT pt;
    HWND hWnd;
    RECT rc;
    HDC hdc;
    static RECT preview_rc = {0};

    switch (message)
    {
    case WM_INITDIALOG:
        _init_font_set(hDlg);
        _init_bg_color_set(hDlg);
        _init_bg_image_set(hDlg);
        _init_layout_set(hDlg);
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if (!_valid_font_set(hDlg))
                return (INT_PTR)FALSE;
            if (!_valid_bg_color_set(hDlg))
                return (INT_PTR)FALSE;
            if (!_valid_bg_image_set(hDlg))
                return (INT_PTR)FALSE;
            if (!_valid_layout_set(hDlg))
                return (INT_PTR)FALSE;
            _display.is_save = 1;
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_CHECK_SAME:
            res = (int)SendMessage(GetDlgItem(hDlg, IDC_CHECK_SAME), BM_GETCHECK, 0, NULL);
            _display.use_same_font = BST_CHECKED == res ? 1 : 0;
            _enable_font_set(hDlg, _display.use_same_font == 0);
            InvalidateRect(hDlg, NULL, FALSE);
            break;
        case IDC_BUTTON_TEXT_FONT:
            _open_fontdlg(hDlg, IDC_BUTTON_TEXT_FONT);
            InvalidateRect(hDlg, NULL, FALSE);
            break;
        case IDC_BUTTON_CHAPTER_FONT:
            _open_fontdlg(hDlg, IDC_BUTTON_CHAPTER_FONT);
            InvalidateRect(hDlg, NULL, FALSE);
            break;
        case IDC_BUTTON_BGCOLOR:
            _open_colordlg(hDlg);
            InvalidateRect(hDlg, NULL, FALSE);
            break;
        case IDC_CHECK_BIENABLE:
            res = (int)SendMessage(GetDlgItem(hDlg, IDC_CHECK_BIENABLE), BM_GETCHECK, 0, NULL);
            _display.bg_image.enable = BST_CHECKED == res ? 1 : 0;
            _enable_bg_image_set(hDlg, _display.bg_image.enable);
            InvalidateRect(hDlg, NULL, FALSE);
            break;
        case IDC_BUTTON_BISEL:
            _bg_image_browse(hDlg);
            break;
        case IDC_COMBO_BIMODE:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                res = (int)SendMessage(GetDlgItem(hDlg, IDC_COMBO_BIMODE), CB_GETCURSEL, 0, NULL);
                if (res != -1)
                    _display.bg_image.mode = res;
                InvalidateRect(hDlg, NULL, FALSE);
            }
            break;
        case IDC_EDIT_BIFILE:
            if (HIWORD(wParam) == EN_CHANGE)
            {
                GetDlgItemText(hDlg, IDC_EDIT_BIFILE, _display.bg_image.file_name, MAX_PATH-1);
                InvalidateRect(hDlg, NULL, FALSE);
            }
            break;
        case IDC_CHECK_MENU_FONT:
            res = (int)SendMessage(GetDlgItem(hDlg, IDC_CHECK_MENU_FONT), BM_GETCHECK, 0, NULL);
            _display.meun_font_follow = BST_CHECKED == res ? 1 : 0;
        case IDC_CHECK_WORD_WRAP:
            res = (int)SendMessage(GetDlgItem(hDlg, IDC_CHECK_WORD_WRAP), BM_GETCHECK, 0, NULL);
            _display.word_wrap = BST_CHECKED == res ? 1 : 0;
        case IDC_CHECK_INDENT:
            res = (int)SendMessage(GetDlgItem(hDlg, IDC_CHECK_INDENT), BM_GETCHECK, 0, NULL);
            _display.line_indent = BST_CHECKED == res ? 1 : 0;
        case IDC_CHECK_BLANKLINES:
            res = (int)SendMessage(GetDlgItem(hDlg, IDC_CHECK_BLANKLINES), BM_GETCHECK, 0, NULL);
            _display.blank_lines = BST_CHECKED == res ? 1 : 0;
            break;
        case IDC_CHECK_CHAPTER_PAGE:
            res = (int)SendMessage(GetDlgItem(hDlg, IDC_CHECK_CHAPTER_PAGE), BM_GETCHECK, 0, NULL);
            _display.chapter_page = BST_CHECKED == res ? 1 : 0;
        default:
            break;
        }
        break;
    case WM_DRAWITEM:
        draw = (DRAWITEMSTRUCT*)lParam;
        if (draw->CtlID == IDC_STATIC_PREVIEW)
        {
            preview_rc = draw->rcItem;
            _update_preview(draw->hDC, &draw->rcItem);
        }
        break;
    case WM_LBUTTONDOWN:
        pt.x = LOWORD(lParam);
        pt.y = HIWORD(lParam);
        hWnd = GetDlgItem(hDlg, IDC_STATIC_PICKER);
        ClientToScreen(hDlg, &pt);
        GetWindowRect(hWnd, &rc);
        if (PtInRect(&rc, pt))
        {
            _start_color_picker(hDlg);
        }
        break;
    case WM_LBUTTONUP:
        _stop_color_picker(hDlg);
        break;
    case WM_MOUSEMOVE:
        if (_is_capturing)
        {
            pt.x = GET_X_LPARAM(lParam);
            pt.y = GET_Y_LPARAM(lParam);
            ClientToScreen(hDlg, &pt);
            hWnd = GetDlgItem(hDlg, IDC_STATIC_PREVIEW);
            hdc = GetDC(NULL);
            _display.bg_color = GetPixel(hdc, pt.x, pt.y);
            ReleaseDC(NULL, hdc);
            UpdateWindow(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
            _update_bg_rgb(hDlg);
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

static void _init_font_set(HWND hDlg)
{
    //EnableWindow(GetDlgItem(hDlg, IDC_CHECK_SAME), FALSE); // this feature is not ready yet, so disable it.
    SendMessage(GetDlgItem(hDlg, IDC_CHECK_SAME), BM_SETCHECK, _display.use_same_font ? BST_CHECKED : BST_UNCHECKED, NULL);
    _enable_font_set(hDlg, _display.use_same_font == 0);
}

static void _init_bg_color_set(HWND hDlg)
{
    _hCursor = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSOR_PICKER));
    _update_bg_rgb(hDlg);
}

static void _init_bg_image_set(HWND hDlg)
{
    TCHAR text[MAX_PATH];
    SendMessage(GetDlgItem(hDlg, IDC_CHECK_BIENABLE), BM_SETCHECK, _display.bg_image.enable ? BST_CHECKED : BST_UNCHECKED, NULL);
    SetWindowText(GetDlgItem(hDlg, IDC_EDIT_BIFILE), _display.bg_image.file_name);
    LoadString(hInst, IDS_STRETCH, text, MAX_PATH);
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_BIMODE), CB_ADDSTRING, 0, (LPARAM)text);
    LoadString(hInst, IDS_TILE, text, MAX_PATH);
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_BIMODE), CB_ADDSTRING, 1, (LPARAM)text);
    LoadString(hInst, IDS_TILEFLIP, text, MAX_PATH);
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_BIMODE), CB_ADDSTRING, 2, (LPARAM)text);
    SendMessage(GetDlgItem(hDlg, IDC_COMBO_BIMODE), CB_SETCURSEL, _display.bg_image.mode, NULL);
    _enable_bg_image_set(hDlg, _display.bg_image.enable);
}

static void _init_layout_set(HWND hDlg)
{
    TCHAR buf[256] = { 0 };
    _stprintf(buf, _T("%d,%d,%d,%d"), _display.internal_border.left, _display.internal_border.top, _display.internal_border.right, _display.internal_border.bottom);
    SetDlgItemInt(hDlg, IDC_EDIT_CHARGAP, _display.char_gap, FALSE);
    SetDlgItemInt(hDlg, IDC_EDIT_LINEGAP, _display.line_gap, FALSE);
    SetDlgItemInt(hDlg, IDC_EDIT_PARAGRAPHGAP, _display.paragraph_gap, FALSE);
    SetDlgItemText(hDlg, IDC_EDIT_BORDER, buf);
    SendMessage(GetDlgItem(hDlg, IDC_CHECK_MENU_FONT), BM_SETCHECK, _display.meun_font_follow ? BST_CHECKED : BST_UNCHECKED, NULL);
    SendMessage(GetDlgItem(hDlg, IDC_CHECK_WORD_WRAP), BM_SETCHECK, _display.word_wrap ? BST_CHECKED : BST_UNCHECKED, NULL);
    SendMessage(GetDlgItem(hDlg, IDC_CHECK_INDENT), BM_SETCHECK, _display.line_indent ? BST_CHECKED : BST_UNCHECKED, NULL);
    SendMessage(GetDlgItem(hDlg, IDC_CHECK_BLANKLINES), BM_SETCHECK, _display.blank_lines ? BST_CHECKED : BST_UNCHECKED, NULL);
    SendMessage(GetDlgItem(hDlg, IDC_CHECK_CHAPTER_PAGE), BM_SETCHECK, _display.chapter_page ? BST_CHECKED : BST_UNCHECKED, NULL);
}

static void _enable_font_set(HWND hDlg, BOOL enable)
{
    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_CHAPTER_FONT), enable);
}

static void _enable_bg_image_set(HWND hDlg, BOOL enable)
{
    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_BIMODE), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_EDIT_BIFILE), enable);
    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON_BISEL), enable);
}

static BOOL _valid_font_set(HWND hDlg)
{
    return TRUE;
}

static BOOL _valid_bg_color_set(HWND hDlg)
{
    return TRUE;
}

static BOOL _valid_bg_image_set(HWND hDlg)
{
    TCHAR *ext;

    if (_display.bg_image.enable)
    {
        if (!_display.bg_image.file_name[0])
        {
            MessageBox_(hDlg, IDS_PLS_SEL_IMG, IDS_ERROR, MB_OK|MB_ICONERROR);
            return FALSE;
        }

        ext = PathFindExtension(_display.bg_image.file_name);
        if (_tcscmp(ext, _T(".jpg")) != 0
            && _tcscmp(ext, _T(".png")) != 0
            && _tcscmp(ext, _T(".bmp")) != 0
            && _tcscmp(ext, _T(".jpeg")) != 0)
        {
            MessageBox_(hDlg, IDS_INVALID_IMG, IDS_ERROR, MB_OK|MB_ICONERROR);
            return FALSE;
        }

        if (!FileExists(_display.bg_image.file_name))
        {
            MessageBox_(hDlg, IDS_IMG_NOT_EXIST, IDS_ERROR, MB_OK|MB_ICONERROR);
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL _valid_layout_set(HWND hDlg)
{
    BOOL bResult = FALSE;
    TCHAR buf[256] = {0};

    _display.char_gap = GetDlgItemInt(hDlg, IDC_EDIT_CHARGAP, &bResult, FALSE);
    if (!bResult)
    {
        MessageBox_(hDlg, IDS_INVALID_CHARGAP, IDS_ERROR, MB_OK|MB_ICONERROR);
        return FALSE;
    }
    _display.line_gap = GetDlgItemInt(hDlg, IDC_EDIT_LINEGAP, &bResult, FALSE);
    if (!bResult)
    {
        MessageBox_(hDlg, IDS_INVALID_LINEGAP, IDS_ERROR, MB_OK|MB_ICONERROR);
        return FALSE;
    }
    _display.paragraph_gap = GetDlgItemInt(hDlg, IDC_EDIT_PARAGRAPHGAP, &bResult, FALSE);
    if (!bResult)
    {
        MessageBox_(hDlg, IDS_INVALID_PARAGRAPHGAP, IDS_ERROR, MB_OK|MB_ICONERROR);
        return FALSE;
    }
    GetDlgItemText(hDlg, IDC_EDIT_BORDER, buf, 256);
    if (4 != _stscanf(buf, _T("%d,%d,%d,%d"), &_display.internal_border.left, &_display.internal_border.top, &_display.internal_border.right, &_display.internal_border.bottom))
    {
        MessageBox_(hDlg, IDS_INVALID_INBORDER, IDS_ERROR, MB_OK|MB_ICONERROR);
        return FALSE;
    }
    return TRUE;
}

static void _bg_image_browse(HWND hDlg)
{
    TCHAR szFileName[MAX_PATH] = {0};
    OPENFILENAME ofn = {0};
    ofn.lStructSize = sizeof(ofn);  
    ofn.hwndOwner = hDlg;
    ofn.lpstrFilter = _T("Images (*.jpg;*.jpeg;*.png;*.bmp)\0*.jpg;*.jpeg;*.png;*.bmp\0\0");
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrFile = szFileName; 
    ofn.nMaxFile = sizeof(szFileName)/sizeof(*szFileName);  
    ofn.nFilterIndex = 0;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    BOOL bSel = GetOpenFileName(&ofn);
    if (!bSel)
    {
        return;
    }
    SetWindowText(GetDlgItem(hDlg, IDC_EDIT_BIFILE), szFileName);
}

LRESULT CALLBACK ChooseFontComboProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubClass, DWORD_PTR udata)
{
    CHOOSECOLOR cc;
    COLORREF co;

    switch (message)
    {
    case WM_LBUTTONDOWN:
        ZeroMemory(&cc, sizeof(cc));
        cc.lStructSize = sizeof(cc);
        cc.hwndOwner = hWnd;
        cc.lpCustColors = (LPDWORD)_header->cust_colors;
        cc.rgbResult = (COLORREF)SendMessage(hWnd, CB_GETITEMDATA, 0, 0);
        cc.Flags = CC_FULLOPEN | CC_RGBINIT | CC_ANYCOLOR;
        if (ChooseColor(&cc))
        {
            SendMessage(hWnd, CB_SETITEMDATA, 0, cc.rgbResult);
            SendMessage(hWnd, CB_SETCURSEL, 0, cc.rgbResult);
            InvalidateRect(GetParent(hWnd), NULL, TRUE);
        }
        return 0;
        break;
    case CB_GETLBTEXTLEN:
        return 7;
        break;
    case CB_GETLBTEXT:
        co = (COLORREF)SendMessage(hWnd, CB_GETITEMDATA, 0, 0);
        _stprintf((TCHAR*)lParam, _T("#%02X%02X%02X"), GetRValue(co), GetGValue(co), GetBValue(co));
        return 7;
        break;
    case WM_NCDESTROY:
        RemoveWindowSubclass(hWnd, ChooseFontComboProc, uIdSubClass);
        break;
    }
    return DefSubclassProc(hWnd, message, wParam, lParam);
}

static UINT_PTR CALLBACK ChooseFontProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hComb;
    switch (message)
    {
    case WM_INITDIALOG:
        hComb = GetDlgItem(hWnd, 0x473);
        SendMessage(hComb, CB_SETITEMDATA, 0, *_p_font_color);
        SendMessage(hComb, CB_SETCURSEL, 0, *_p_font_color);
        SetWindowSubclass(hComb, ChooseFontComboProc, 0, 0);
        break;
    }
    return 0;
};

static void _open_fontdlg(HWND hDlg, int id)
{
    CHOOSEFONT cf;            // common dialog box structure

    if (IDC_BUTTON_TEXT_FONT == id)
    {
        _p_font = &_display.font;
        _p_font_color = &_display.font_color;
    }
    else
    {
        _p_font = &_display.font_title;
        _p_font_color = &_display.font_color_title;
    }

    // Initialize CHOOSEFONT
    ZeroMemory(&cf, sizeof(cf));
    cf.lStructSize = sizeof (cf);
    cf.hwndOwner = hDlg;
    cf.lpLogFont = _p_font;
    cf.rgbColors = *_p_font_color;
    cf.lpfnHook = ChooseFontProc;
    cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_EFFECTS | CF_NOVERTFONTS | CF_ENABLEHOOK;

    if (ChooseFont(&cf))
    {
        *_p_font_color = cf.rgbColors;
        _p_font->lfQuality = PROOF_QUALITY;
    }
}

static void _open_colordlg(HWND hDlg)
{
    CHOOSECOLOR cc;                 // common dialog box structure 

    ZeroMemory(&cc, sizeof(cc));
    cc.lStructSize = sizeof(cc);
    cc.hwndOwner = hDlg;
    cc.lpCustColors = (LPDWORD)_header->cust_colors; // array of custom colors 
    cc.rgbResult = _display.bg_color;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColor(&cc))
    {
        _display.bg_color = cc.rgbResult;
        _update_bg_rgb(hDlg);
    }
}

static void _start_color_picker(HWND hDlg)
{
    if (!_is_capturing)
    {
        SetCapture(hDlg);
        SetCursor(_hCursor);
        _is_capturing = TRUE;
    }
}

static void _stop_color_picker(HWND hDlg)
{
    if (_is_capturing)
    {
        ReleaseCapture();
        _is_capturing = FALSE;
        SetCursor(LoadCursor(hInst, IDC_ARROW));
    }
}

Gdiplus::Bitmap* _load_bg_image(int w, int h)
{
    Gdiplus::Bitmap *image = NULL, *bgimg = NULL;
    Gdiplus::Graphics *graphics = NULL;
    Gdiplus::ImageAttributes ImgAtt;
    Gdiplus::RectF rcDrawRect;

    if (!_display.bg_image.enable)
        return NULL;

    if (!_display.bg_image.file_name[0])
        return NULL;

    // load image file
    image = Gdiplus::Bitmap::FromFile(_display.bg_image.file_name);
    if (image == NULL)
        return NULL;
    if (Gdiplus::Ok != image->GetLastStatus())
    {
        delete image;
        return NULL;
    }

    // create bg image
    bgimg = new Gdiplus::Bitmap(w, h, PixelFormat32bppARGB);
    rcDrawRect.X=0.0;
    rcDrawRect.Y=0.0;
    rcDrawRect.Width=(float)w;
    rcDrawRect.Height=(float)h;
    graphics = Gdiplus::Graphics::FromImage(bgimg);
    graphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    switch (_display.bg_image.mode)
    {
    case Stretch:
        graphics->DrawImage(image,rcDrawRect,0.0,0.0,(float)image->GetWidth(),(float)image->GetHeight(), Gdiplus::UnitPixel,&ImgAtt);
        break;
    case Tile:
        ImgAtt.SetWrapMode(Gdiplus::WrapModeTile);
        graphics->DrawImage(image,rcDrawRect,0.0,0.0,(float)w,(float)h, Gdiplus::UnitPixel,&ImgAtt);
        break;
    case TileFlip:
        ImgAtt.SetWrapMode(Gdiplus::WrapModeTileFlipXY);
        graphics->DrawImage(image,rcDrawRect,0.0,0.0,(float)w,(float)h, Gdiplus::UnitPixel,&ImgAtt);
        break;
    default:
        graphics->DrawImage(image,rcDrawRect,0.0,0.0,(float)image->GetWidth(),(float)image->GetHeight(), Gdiplus::UnitPixel,&ImgAtt);
        break;
    }

    delete image;
    delete graphics;
    return bgimg;
}

static void _update_bg_rgb(HWND hDlg)
{
    TCHAR rgb[8] = {0};
    _sntprintf(rgb, 8, _T("#%02X%02X%02X"), GetRValue(_display.bg_color), GetGValue(_display.bg_color), GetBValue(_display.bg_color));
    SetDlgItemText(hDlg, IDC_STATIC_RGB, rgb);
}

static void _update_preview(HDC hDC, RECT *p_rc)
{
    const TCHAR* TEXT_CPT1 = _T("标题预览");
    const TCHAR* TEXT_CPT2 = _T("Title preview");
    const TCHAR* TEXT1 = _T("正文预览\r\n");
    const TCHAR* TEXT2 = _T("Text preview");

    HDC memdc;
    HBITMAP hBmp;
    HBRUSH hBrush = NULL;
    HFONT hFont;
    HFONT hCptFont = NULL;
    int height = 0;
    RECT rc;
    Gdiplus::Bitmap *image = NULL;

    rc = *p_rc;
    memdc = CreateCompatibleDC(hDC);
    hFont = CreateFontIndirect(&_display.font);
    if (_display.use_same_font)
    {
        SelectObject(memdc, hFont);
        SetTextColor(memdc, _display.font_color);
    }
    else
    {
        hCptFont = CreateFontIndirect(&_display.font_title);
    }
    image = _load_bg_image(rc.right-rc.left, rc.bottom-rc.top);
    if (image)
    {
        image->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hBmp);
        SelectObject(memdc, hBmp);
    }
    else
    {
        hBmp = CreateCompatibleBitmap(hDC, rc.right-rc.left, rc.bottom-rc.top);
        SelectObject(memdc, hBmp);
        hBrush = CreateSolidBrush(_display.bg_color);
        SelectObject(memdc, hBrush);
        FillRect(memdc, &rc, hBrush);
    }
    rc.top += height + _header->line_gap;
    SetBkMode(memdc, TRANSPARENT);
    if (!_display.use_same_font)
    {
        SelectObject(memdc, hCptFont);
        SetTextColor(memdc, _display.font_color_title);
    }
    height = DrawText(memdc, TEXT_CPT1, -1, &rc, DT_CENTER | DT_WORDBREAK);
    rc.top += height + _header->line_gap;
    if (!_display.use_same_font)
    {
        SelectObject(memdc, hFont);
        SetTextColor(memdc, _display.font_color);
    }
    height = DrawText(memdc, TEXT1, -1, &rc, DT_CENTER | DT_WORDBREAK);
    rc.top += height + _header->line_gap;
    if (!_display.use_same_font)
    {
        SelectObject(memdc, hCptFont);
        SetTextColor(memdc, _display.font_color_title);
    }
    height = DrawText(memdc, TEXT_CPT2, -1, &rc, DT_CENTER | DT_WORDBREAK);
    rc.top += height + _header->line_gap;
    if (!_display.use_same_font)
    {
        SelectObject(memdc, hFont);
        SetTextColor(memdc, _display.font_color);
    }
    height = DrawText(memdc, TEXT2, -1, &rc, DT_CENTER | DT_WORDBREAK);
    rc.top += height + _header->line_gap;
    rc = *p_rc;
    BitBlt(hDC, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, memdc, rc.left, rc.top, SRCCOPY);
    DeleteObject(hBmp);
    DeleteObject(hFont);
    DeleteObject(hCptFont);
    DeleteObject(hBrush);
    DeleteDC(memdc);
    if (image)
        delete image;
}