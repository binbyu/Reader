#include "stdafx.h"
#include "tagset.h"

#if ENABLE_TAG
#include "resource.h"
#include <CommDlg.h>

static tagitem_t *g_tags = NULL;
static int checktagid_list[MAX_TAG_COUNT] = {IDC_CHECK_TAG1, IDC_CHECK_TAG2, IDC_CHECK_TAG3, IDC_CHECK_TAG4, IDC_CHECK_TAG5, IDC_CHECK_TAG6, IDC_CHECK_TAG7, IDC_CHECK_TAG8};
static int btnbgtagid_list[MAX_TAG_COUNT] = {IDC_BUTTON_TAGBG1, IDC_BUTTON_TAGBG2, IDC_BUTTON_TAGBG3, IDC_BUTTON_TAGBG4, IDC_BUTTON_TAGBG5, IDC_BUTTON_TAGBG6, IDC_BUTTON_TAGBG7, IDC_BUTTON_TAGBG8};
static int btnfonttagid_list[MAX_TAG_COUNT] = {IDC_BUTTON_TAGFONT1, IDC_BUTTON_TAGFONT2, IDC_BUTTON_TAGFONT3, IDC_BUTTON_TAGFONT4, IDC_BUTTON_TAGFONT5, IDC_BUTTON_TAGFONT6, IDC_BUTTON_TAGFONT7, IDC_BUTTON_TAGFONT8};
static int editkwtagid_list[MAX_TAG_COUNT] = {IDC_EDIT_TAGKW1, IDC_EDIT_TAGKW2, IDC_EDIT_TAGKW3, IDC_EDIT_TAGKW4, IDC_EDIT_TAGKW5, IDC_EDIT_TAGKW6, IDC_EDIT_TAGKW7, IDC_EDIT_TAGKW8};
static int staticpvtagid_list[MAX_TAG_COUNT] = {IDC_STATIC_PREVIEW1, IDC_STATIC_PREVIEW2, IDC_STATIC_PREVIEW3, IDC_STATIC_PREVIEW4, IDC_STATIC_PREVIEW5, IDC_STATIC_PREVIEW6, IDC_STATIC_PREVIEW7, IDC_STATIC_PREVIEW8};

static INT_PTR CALLBACK TS_DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

void TS_OpenDlg(HINSTANCE hInst, HWND hWnd, tagitem_t *tags)
{
    g_tags = tags;
    DialogBox(hInst, MAKEINTRESOURCE(IDD_TAGSET), hWnd, TS_DlgProc);
}

INT_PTR CALLBACK TS_DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int i = 0;
    int res = 0;
    static HFONT g_fonts[MAX_TAG_COUNT] = {NULL};

    switch (message)
    {
    case WM_INITDIALOG:
        for (i=0; i<MAX_TAG_COUNT; i++)
        {
            if (g_fonts[i])
            {
                DeleteObject(g_fonts[i]);
            }
            g_fonts[i] = CreateFontIndirect(&g_tags[i].font);
            SendMessage(GetDlgItem(hDlg, checktagid_list[i]), BM_SETCHECK, g_tags[i].enable ? BST_CHECKED : BST_UNCHECKED, NULL);
            SetWindowText(GetDlgItem(hDlg, editkwtagid_list[i]), g_tags[i].keyword);
            SetWindowText(GetDlgItem(hDlg, staticpvtagid_list[i]), g_tags[i].keyword);
            EnableWindow(GetDlgItem(hDlg, editkwtagid_list[i]), g_tags[i].enable);
            EnableWindow(GetDlgItem(hDlg, btnbgtagid_list[i]), g_tags[i].enable);
            EnableWindow(GetDlgItem(hDlg, btnfonttagid_list[i]), g_tags[i].enable);
            SendMessage(GetDlgItem(hDlg, staticpvtagid_list[i]), WM_SETFONT, (WPARAM)g_fonts[i], NULL);
        }
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            for (i=0; i<MAX_TAG_COUNT; i++)
            {
                if (g_fonts[i])
                {
                    DeleteObject(g_fonts[i]);
                    g_fonts[i] = NULL;
                }
            }
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDCANCEL:
            for (i=0; i<MAX_TAG_COUNT; i++)
            {
                if (g_fonts[i])
                {
                    DeleteObject(g_fonts[i]);
                    g_fonts[i] = NULL;
                }
            }
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
            break;
        case IDC_CHECK_TAG1:
        case IDC_CHECK_TAG2:
        case IDC_CHECK_TAG3:
        case IDC_CHECK_TAG4:
        case IDC_CHECK_TAG5:
        case IDC_CHECK_TAG6:
        case IDC_CHECK_TAG7:
        case IDC_CHECK_TAG8:
            for (i=0; i<MAX_TAG_COUNT; i++)
            {
                if (checktagid_list[i] == LOWORD(wParam))
                    break;
            }
            res = SendMessage(GetDlgItem(hDlg, checktagid_list[i]), BM_GETCHECK, 0, NULL);
            g_tags[i].enable = (res == BST_CHECKED);
            EnableWindow(GetDlgItem(hDlg, editkwtagid_list[i]), g_tags[i].enable);
            EnableWindow(GetDlgItem(hDlg, btnbgtagid_list[i]), g_tags[i].enable);
            EnableWindow(GetDlgItem(hDlg, btnfonttagid_list[i]), g_tags[i].enable);
            break;
        case IDC_BUTTON_TAGFONT1:
        case IDC_BUTTON_TAGFONT2:
        case IDC_BUTTON_TAGFONT3:
        case IDC_BUTTON_TAGFONT4:
        case IDC_BUTTON_TAGFONT5:
        case IDC_BUTTON_TAGFONT6:
        case IDC_BUTTON_TAGFONT7:
        case IDC_BUTTON_TAGFONT8:
            {
                CHOOSEFONT cf;            // common dialog box structure
                LOGFONT logFont;
                static DWORD rgbCurrent;   // current text color
                BOOL bUpdate = FALSE;

                for (i=0; i<MAX_TAG_COUNT; i++)
                {
                    if (btnfonttagid_list[i] == LOWORD(wParam))
                        break;
                }

                // Initialize CHOOSEFONT
                ZeroMemory(&cf, sizeof(cf));
                memcpy(&logFont, &(g_tags[i].font), sizeof(LOGFONT));
                cf.lStructSize = sizeof (cf);
                cf.hwndOwner = hDlg;
                cf.lpLogFont = &logFont;
                cf.rgbColors = g_tags[i].font_color;
                cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_EFFECTS;

                if (ChooseFont(&cf))
                {
                    if (g_tags[i].font_color != cf.rgbColors)
                    {
                        g_tags[i].font_color = cf.rgbColors;
                        bUpdate = TRUE;
                    }
                    g_tags[i].font.lfQuality = PROOF_QUALITY;

                    if (0 != memcmp(&logFont, &g_tags[i].font, sizeof(LOGFONT)))
                    {
                        memcpy(&g_tags[i].font, &logFont, sizeof(LOGFONT));
                        bUpdate = TRUE;

                        if (g_fonts[i])
                        {
                            DeleteObject(g_fonts[i]);
                        }
                        g_fonts[i] = CreateFontIndirect(&g_tags[i].font);
                        SendMessage(GetDlgItem(hDlg, staticpvtagid_list[i]), WM_SETFONT, (WPARAM)g_fonts[i], NULL);
                    }
                    if (bUpdate)
                    {
                        InvalidateRect(hDlg, NULL, TRUE);
                    }
                }
            }
            break;
        case IDC_BUTTON_TAGBG1:
        case IDC_BUTTON_TAGBG2:
        case IDC_BUTTON_TAGBG3:
        case IDC_BUTTON_TAGBG4:
        case IDC_BUTTON_TAGBG5:
        case IDC_BUTTON_TAGBG6:
        case IDC_BUTTON_TAGBG7:
        case IDC_BUTTON_TAGBG8:
            {
                CHOOSECOLOR cc;                 // common dialog box structure 
                static COLORREF acrCustClr[16]; // array of custom colors 

                for (i=0; i<MAX_TAG_COUNT; i++)
                {
                    if (btnbgtagid_list[i] == LOWORD(wParam))
                        break;
                }

                ZeroMemory(&cc, sizeof(cc));
                cc.lStructSize = sizeof(cc);
                cc.hwndOwner = hDlg;
                cc.lpCustColors = (LPDWORD) acrCustClr;
                cc.rgbResult = g_tags[i].bg_color;
                cc.Flags = CC_FULLOPEN | CC_RGBINIT;

                if (ChooseColor(&cc))
                {
                    if (g_tags[i].bg_color != cc.rgbResult)
                    {
                        g_tags[i].bg_color = cc.rgbResult;
                        InvalidateRect(hDlg, NULL, TRUE);
                    }
                }
            }
            break;
        case IDC_EDIT_TAGKW1:
        case IDC_EDIT_TAGKW2:
        case IDC_EDIT_TAGKW3:
        case IDC_EDIT_TAGKW4:
        case IDC_EDIT_TAGKW5:
        case IDC_EDIT_TAGKW6:
        case IDC_EDIT_TAGKW7:
        case IDC_EDIT_TAGKW8:
            {
                for (i=0; i<MAX_TAG_COUNT; i++)
                {
                    if (editkwtagid_list[i] == LOWORD(wParam))
                        break;
                }
                if (i<MAX_TAG_COUNT)
                {
                    GetWindowText(GetDlgItem(hDlg, editkwtagid_list[i]), g_tags[i].keyword, 63);
                    SetWindowText(GetDlgItem(hDlg, staticpvtagid_list[i]), g_tags[i].keyword);
                }
            }
            break;
        default:
            break;
        }
        break;
    case WM_CTLCOLORSTATIC:
        {
            for (i=0; i<MAX_TAG_COUNT; i++)
            {
                if (staticpvtagid_list[i] == GetDlgCtrlID((HWND)lParam))
                    break;
            }
            SetBkMode((HDC)wParam, TRANSPARENT);
            if (i < MAX_TAG_COUNT)
            {
                SetBkColor((HDC)wParam, g_tags[i].bg_color);
                SetBkMode((HDC)wParam, OPAQUE);
                SetTextColor((HDC)wParam, g_tags[i].font_color);
            }
            return (BOOL)CreateSolidBrush (GetSysColor(COLOR_MENU));
        }
        break;
    default:
        break;
    }
    return (INT_PTR)FALSE;
}

#endif
