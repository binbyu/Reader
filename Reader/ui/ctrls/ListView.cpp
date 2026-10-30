#include "ListView.h"
#include "resource.h"

ListView::ListView(HWND hWnd, IWnd* parent)
    : m_hWnd(hWnd)
    , m_parent(parent)
    , m_menuType(0)
    , m_checkEnabled(false)
    , m_updating(false)
    , m_pendingSelectionNotify(false)
{
    ListView_SetExtendedListViewStyle(m_hWnd, LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_SINGLEROW/* | LVS_EX_AUTOSIZECOLUMNS*/);
    SetWindowSubclass(m_hWnd, subclassProc, 0, (DWORD_PTR)this);
}

ListView::~ListView()
{
}

void ListView::setCheckBox(bool enabled)
{
    m_checkEnabled = enabled;
    if (m_checkEnabled)
    {
        ListView_SetExtendedListViewStyleEx(m_hWnd, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);
    }
}

void ListView::setContextMenuType(UINT type)
{
    m_menuType = type;
}

void ListView::setHeader(const std::vector<HeaderItem>& header)
{
    clearHeader();
    for (auto& i : header)
    {
        addHeader(i.ids, i.width);
    }
}

void ListView::addHeader(UINT ids, int width)
{
    std::wstring text = m_parent->loadString(ids);
    addHeader(text.c_str(), width);
}

void ListView::addHeader(const wchar_t* text, int width)
{
    int count = headerCount();
    LVCOLUMN lvc = { 0 };
    lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    lvc.pszText = (LPWSTR)text;
    lvc.cx = width;
    ListView_InsertColumn(m_hWnd, count, &lvc);
}

int ListView::headerCount() const
{
    return Header_GetItemCount(ListView_GetHeader(m_hWnd));
}

void ListView::clearHeader()
{
    int count = headerCount();
    for (int i = count - 1; i >= 0; i--)
    {
        ListView_DeleteColumn(m_hWnd, i);
    }
}

void ListView::addItem(int row, const wchar_t* text, void* userData)
{
    m_updating = true;
    LVITEM lvi = { 0 };
    lvi.mask = LVIF_TEXT | LVIF_PARAM;
    lvi.cchTextMax = MAX_URL_LENGTH;
    lvi.iItem = row;
    lvi.iSubItem = 0;
    lvi.pszText = (LPWSTR)text;
    lvi.lParam = (LPARAM)userData;
    ListView_InsertItem(m_hWnd, &lvi);
    m_updating = false;
}

void ListView::addItem(int row, const std::vector<std::wstring>& texts, void* userData)
{
    if (texts.empty())
    {
        addItem(row, L"", userData);
        return;
    }
    addItem(row, texts[0].c_str(), userData);
    for (int col = 1; col < (int)texts.size(); col++)
    {
        setItem(row, col, (LPWSTR)texts[col].c_str());
    }
}

void ListView::setItem(int row, int col, const wchar_t* text)
{
    ListView_SetItemText(m_hWnd, row, col, (LPWSTR)text);
}

void ListView::deleteItem(int row)
{
    m_updating = true;
    ListView_DeleteItem(m_hWnd, row);
    m_updating = false;
}

void ListView::setItemChecked(int row, bool checked)
{
    if (m_checkEnabled)
    {
        ListView_SetCheckState(m_hWnd, row, checked ? 1 : 0);
    }
}

void ListView::setItemSelected(int row, bool selected, bool sync)
{
    if (sync)
    {
        ListView_SetItemState(m_hWnd, row, selected ? (LVIS_SELECTED | LVIS_FOCUSED) : 0, LVIS_SELECTED | LVIS_FOCUSED);
    }
    else
    {
        LVITEMW lvi;
        lvi.stateMask = (LVIS_SELECTED | LVIS_FOCUSED);
        lvi.state = (selected ? (LVIS_SELECTED | LVIS_FOCUSED) : 0);
        ::PostMessageW((m_hWnd), LVM_SETITEMSTATE, (WPARAM)(row), (LPARAM)&lvi);
    }
}

void ListView::clearItems()
{
    m_updating = true;
    ListView_DeleteAllItems(m_hWnd);
    m_updating = false;
}

int ListView::itemCount() const
{
    return ListView_GetItemCount(m_hWnd);
}

int ListView::selectedItem() const
{
    return ListView_GetNextItem(m_hWnd, -1, LVNI_SELECTED);
}

void ListView::setItemUserData(int row, void* userData)
{
    LVITEM lvi = { 0 };
    lvi.mask = LVIF_PARAM;
    lvi.iItem = row;
    lvi.iSubItem = 0;
    lvi.lParam = (LPARAM)userData;
    ListView_SetItem(m_hWnd, &lvi);
}

void* ListView::itemUserData(int row)
{
    LVITEM lvi = { 0 };
    lvi.mask = LVIF_PARAM;
    lvi.iItem = row;
    lvi.iSubItem = 0;
    ListView_GetItem(m_hWnd, &lvi);
    return (void*)lvi.lParam;
}

LRESULT ListView::onListItemChanged(LPNMLISTVIEW pnmv)
{
    if (m_updating)
        return TRUE;

    if (pnmv->uChanged & LVIF_STATE)
    {
        // for checkbox checked
        UINT oldState = pnmv->uOldState & LVIS_STATEIMAGEMASK;
        UINT newState = pnmv->uNewState & LVIS_STATEIMAGEMASK;
        if (oldState != newState)
        {
            BOOL enable = (BOOL)ListView_GetCheckState(pnmv->hdr.hwndFrom, pnmv->iItem);
            SendMessage(m_parent->getHWND(), WM_LIST_CHECKED, pnmv->iItem, enable);
        }

        // for selected notification, defer until selection settles
        oldState = pnmv->uOldState & LVIS_SELECTED;
        newState = pnmv->uNewState & LVIS_SELECTED;
        if (oldState != newState && !m_pendingSelectionNotify)
        {
            m_pendingSelectionNotify = true;
            PostMessage(m_hWnd, WM_LIST_SELECTIONSETTLED, 0, 0);
        }
    }
    return (INT_PTR)TRUE;
}

LRESULT ListView::onListCustomDraw(LPNMLVCUSTOMDRAW customDraw)
{
    if (customDraw->nmcd.dwDrawStage == CDDS_PREPAINT)
    {
        SetWindowLongPtr(m_parent->getHWND(), DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
        return CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYSUBITEMDRAW;
    }
    if (customDraw->nmcd.dwDrawStage & CDDS_ITEMPREPAINT)
    {
        int item = (int)customDraw->nmcd.dwItemSpec;
        UINT state = ListView_GetItemState(customDraw->nmcd.hdr.hwndFrom, item, LVIS_SELECTED);
        if (state & LVIS_SELECTED)
        {
            customDraw->clrText = GetSysColor(COLOR_HIGHLIGHTTEXT);
            customDraw->clrTextBk = GetSysColor(COLOR_HIGHLIGHT);
            return CDRF_NEWFONT;
        }
    }
    return CDRF_DODEFAULT;
}

LRESULT ListView::onListContextMenu(const POINT& pt)
{
    if (m_menuType == 0)
        return TRUE;

    int cnt = itemCount();
    int row = selectedItem();
    bool inItem = row >= 0 && row < cnt;
    int cnt1 = 0;
    int cnt2 = 0;
    int cnt3 = 0;

    HMENU hMenu = CreatePopupMenu();

    if ((m_menuType & CONTEXT_MENU_OPEN) && inItem)
    {
        AppendMenu(hMenu, MF_STRING, IDM_OPEN, m_parent->loadString(IDS_OPEN).c_str());
        cnt1++;
    }
    if ((m_menuType & CONTEXT_MENU_PREVIEW) && inItem)
    {
        AppendMenu(hMenu, MF_STRING, IDM_PREVIEW, m_parent->loadString(IDS_PREVIEW).c_str());
        cnt1++;
    }

    if (m_menuType & CONTEXT_MENU_ADD)
    {
        AppendMenu(hMenu, MF_STRING, IDM_ADD, m_parent->loadString(IDS_ADD).c_str());
        cnt2++;
    }
    if ((m_menuType & CONTEXT_MENU_DELETE) && inItem)
    {
        AppendMenu(hMenu, MF_STRING, IDM_DELETE, m_parent->loadString(IDS_DELETE).c_str());
        cnt2++;
    }
    if ((m_menuType & CONTEXT_MENU_EDIT) && inItem)
    {
        AppendMenu(hMenu, MF_STRING, IDM_EDIT, m_parent->loadString(IDS_EDIT).c_str());
        cnt2++;
    }
    if (m_menuType & CONTEXT_MENU_CLEAR)
    {
        AppendMenu(hMenu, MF_STRING, IDM_CLEAR, m_parent->loadString(IDS_CLEAR).c_str());
        cnt2++;
    }

    if ((m_menuType & CONTEXT_MENU_MOVETOP) && inItem && row > 0)
    {
        AppendMenu(hMenu, MF_STRING, IDM_MOVETOP, m_parent->loadString(IDS_TOP).c_str());
        cnt3++;
    }
    if ((m_menuType & CONTEXT_MENU_MOVEUP) && inItem && row > 0)
    {
        AppendMenu(hMenu, MF_STRING, IDM_MOVEUP, m_parent->loadString(IDS_MOVEUP).c_str());
        cnt3++;
    }
    if ((m_menuType & CONTEXT_MENU_MOVEDOWN) && inItem && row < cnt - 1)
    {
        AppendMenu(hMenu, MF_STRING, IDM_MOVEDOWN, m_parent->loadString(IDS_MOVEDOWN).c_str());
        cnt3++;
    }
    if ((m_menuType & CONTEXT_MENU_MOVEBOTTOM) && inItem && row < cnt - 1)
    {
        AppendMenu(hMenu, MF_STRING, IDM_MOVEBOTTOM, m_parent->loadString(IDS_BOTTOM).c_str());
        cnt3++;
    }

    if (cnt1 > 0 && (cnt2 > 0 || cnt3 > 0))
    {
        InsertMenu(hMenu, cnt1, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
        cnt1++;
    }
    if (cnt2 > 0 && cnt3 > 0)
    {
        InsertMenu(hMenu, cnt1 + cnt2, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
    }

    if (GetMenuItemCount(hMenu) == 0)
    {
        DestroyMenu(hMenu);
        return TRUE;
    }

    int ret = TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL);
    DestroyMenu(hMenu);
    switch (ret)
    {
    case IDM_OPEN:
        onListItemOpen(row);
        break;
    case IDM_PREVIEW:
        onListItemPreview(row);
        break;
    case IDM_ADD:
        onListItemAdd();
        break;
    case IDM_DELETE:
        onListItemDelete(row);
        break;
    case IDM_EDIT:
        onListItemEdit(row);
        break;
    case IDM_CLEAR:
        onListItemClear();
        break;
    case IDM_MOVETOP:
        onListItemMoveTop(row);
        break;
    case IDM_MOVEUP:
        onListItemMoveUp(row);
        break;
    case IDM_MOVEDOWN:
        onListItemMoveDown(row);
        break;
    case IDM_MOVEBOTTOM:
        onListItemMoveBottom(row);
        break;
    default:
        break;
    }
    return (INT_PTR)TRUE;
}

LRESULT ListView::onDeferredSelectionNotify()
{
    m_pendingSelectionNotify = false;
    int index = selectedItem();
    SendMessage(m_parent->getHWND(), WM_LIST_SELECTED, index, (LPARAM)(index >= 0));
    return TRUE;
}

void ListView::onListItemOpen(int row)
{
    PostMessage(m_parent->getHWND(), WM_LIST_OPEN, row, NULL);
}

void ListView::onListItemPreview(int row)
{
    PostMessage(m_parent->getHWND(), WM_LIST_PREVIEW, row, NULL);
}

void ListView::onListItemAdd()
{
    PostMessage(m_parent->getHWND(), WM_LIST_ADD, 0, NULL);
}

void ListView::onListItemDelete(int row)
{
    if (IDYES == m_parent->messageBox(IDS_DELETE_CONFIRM, IDS_TIPS, MB_ICONQUESTION | MB_YESNO))
    {
        deleteItem(row);
        SendMessage(m_parent->getHWND(), WM_LIST_DELETE, row, NULL);
        if (row >= itemCount())
        {
            setItemSelected(row - 1, true);
        }
        else
        {
            setItemSelected(row, true);
        }
    }
}

void ListView::onListItemEdit(int row)
{
    PostMessage(m_parent->getHWND(), WM_LIST_EDIT, row, NULL);
}

void ListView::onListItemClear()
{
    if (IDYES == m_parent->messageBox(IDS_CLEAR_CONFIRM, IDS_TIPS, MB_ICONQUESTION | MB_YESNO))
    {
        clearItems();
        SendMessage(m_parent->getHWND(), WM_LIST_CLEAR, 0, NULL);
        setItemSelected(-1, true);
    }
}

void ListView::onListItemMoveTop(int row)
{
    m_updating = true;
    ItemData data = itemData(row);
    for (int i = row; i > 0; i--)
    {
        setItemData(i, itemData(i - 1));
    }
    setItemData(0, data);
    m_updating = false;
    SendMessage(m_parent->getHWND(), WM_LIST_MOVETOP, row, NULL);
    setItemSelected(0, true);
}

void ListView::onListItemMoveUp(int row)
{
    m_updating = true;
    swapItemData(row, row - 1);
    m_updating = false;
    SendMessage(m_parent->getHWND(), WM_LIST_MOVEUP, row, NULL);
    setItemSelected(row - 1, true);
}

void ListView::onListItemMoveDown(int row)
{
    m_updating = true;
    swapItemData(row, row + 1);
    m_updating = false;
    SendMessage(m_parent->getHWND(), WM_LIST_MOVEDOWN, row, NULL);
    setItemSelected(row + 1, true);
}

void ListView::onListItemMoveBottom(int row)
{
    int count = itemCount();
    m_updating = true;
    ItemData data = itemData(row);
    for (int i = row; i < count - 1; i++)
    {
        setItemData(i, itemData(i + 1));
    }
    setItemData(count - 1, data);
    m_updating = false;
    SendMessage(m_parent->getHWND(), WM_LIST_MOVEBOTTOM, row, NULL);
    setItemSelected(count - 1, true);
}

ListView::ItemData ListView::itemData(int row)
{
    ItemData data;
    int colCount = headerCount();
    data.texts.resize(colCount);
    for (int col = 0; col < colCount; col++)
    {
        wchar_t text[MAX_PATH] = { 0 };
        ListView_GetItemText(m_hWnd, row, col, text, MAX_PATH);
        data.texts[col] = text;
    }
    if (m_checkEnabled)
    {
        data.checked = ListView_GetCheckState(m_hWnd, row);
    }
    else
    {
        data.checked = -1;
    }
    return data;
}

void ListView::setItemData(int row, const ItemData& data)
{
    for (int col = 0; col < (int)data.texts.size(); col++)
    {
        ListView_SetItemText(m_hWnd, row, col, (LPWSTR)data.texts[col].c_str());
    }
    if (data.checked != -1)
    {
        ListView_SetCheckState(m_hWnd, row, data.checked);
    }
}

void ListView::swapItemData(int row1, int row2)
{
    ItemData data1 = itemData(row1);
    setItemData(row1, itemData(row2));
    setItemData(row2, data1);
}

LRESULT ListView::onNotify(WPARAM wParam, LPARAM lParam)
{
    LPNMHDR pnmh = (LPNMHDR)lParam;
    if (pnmh->hwndFrom == m_hWnd)
    {
        switch (pnmh->code)
        {
        case NM_CUSTOMDRAW:
            return onListCustomDraw((LPNMLVCUSTOMDRAW)lParam);
        case LVN_ITEMCHANGED:
            return onListItemChanged((LPNMLISTVIEW)lParam);
        case NM_DBLCLK:
            PostMessage(m_parent->getHWND(), WM_LIST_DBLCLK, 0, 0);
            break;
        }
    }
    return TRUE;
}

LRESULT CALLBACK ListView::subclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubClass, DWORD_PTR udata)
{
    ListView* pThis = (ListView*)udata;
    if (pThis)
    {
        switch (message)
        {
#if 0 // not work, use onNotify()
        case WM_NOTIFY:
            if (LOWORD(wParam) == pThis->m_ctrlId)
            {
                LPNMHDR pnmh = (LPNMHDR)lParam;
                switch (pnmh->code)
                {
                case NM_CUSTOMDRAW:
                    return pThis->OnListCustomDraw((LPNMLVCUSTOMDRAW)lParam);
                case LVN_ITEMCHANGED:
                    return pThis->onListItemChanged((LPNMLISTVIEW)lParam);
                }
            }
            break;
#endif

        case WM_LIST_SELECTIONSETTLED:
            return pThis->onDeferredSelectionNotify();

        case WM_CONTEXTMENU:
            if ((HWND)wParam == pThis->m_hWnd)
            {
                POINT pt = {LOWORD(lParam), HIWORD(lParam)};
                return pThis->onListContextMenu(pt);
            }
            break;
        }
    }
    return DefSubclassProc(hWnd, message, wParam, lParam);
}
