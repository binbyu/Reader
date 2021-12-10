#ifndef __KEYSET_H__
#define __KEYSET_H__

#include "types.h"

typedef enum keytype_t
{
    KT_HOTKEY = 0,
    KT_SHORTCUTKEY
} keytype_t;

typedef enum keyid_t
{
    KI_HIDE = 0,
    KI_BORDER,
    KI_FULLSCREEN,
    KI_TOP,
    KI_OPEN,
    KI_ADDMARK,
    KI_AUTOPAGE,
    KI_SEARCH,
    KI_JUMP,
    KI_EDIT,
    KI_PAGEUP,
    KI_PAGEDOWN,
    KI_LINEUP,
    KI_LINEDOWN,
    KI_CHAPTERUP,
    KI_CHAPTERDOWN,
    KI_FONTZOOMIN,
    KI_FONTZOOMOUT,
    KI_MAXCOUNT
} keyid_t;


typedef LRESULT (* keyproc)(HWND, UINT, WPARAM, LPARAM);

typedef struct keydata_t 
{
    DWORD   key;       // keyid << 16 | keytype
    DWORD   key_id;    // just for keytype = KT_HOTKEY
    DWORD  *pvalue;
    int    *is_disable;
    DWORD   defval;
    DWORD   ctrl_id;
    DWORD   able_id;
    keyproc proc;
    UINT    desc;
} keydata_t;


void KS_Init(HWND hWnd, keyset_t *keyset);
void KS_UpdateKeyset(keyset_t *keyset);
void KS_GetDefaultKeyset(keyset_t *keyset);
void KS_OpenDlg(void);
INT_PTR CALLBACK KS_DlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL KS_KeyDownProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL KS_HotKeyProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL KS_RegisterAllHotKey(HWND hWnd);
BOOL KS_UnRegisterAllHotKey(HWND hWnd);


#endif
