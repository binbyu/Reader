#pragma once
#include "IWnd.h"
#include <vector>

#define CONTEXT_MENU_MOVEUP             (0x0001)
#define CONTEXT_MENU_MOVEDOWN           (0x0002)
#define CONTEXT_MENU_MOVETOP            (0x0004)
#define CONTEXT_MENU_MOVEBOTTOM         (0x0008)
#define CONTEXT_MENU_ADD                (0x0010)
#define CONTEXT_MENU_DELETE             (0x0020)
#define CONTEXT_MENU_EDIT               (0x0040)
#define CONTEXT_MENU_CLEAR              (0x0080)
#define CONTEXT_MENU_OPEN               (0x0100)
#define CONTEXT_MENU_PREVIEW            (0x0200)

#define CONTEXT_MENU_MOVE               (CONTEXT_MENU_MOVEUP | CONTEXT_MENU_MOVEDOWN | CONTEXT_MENU_MOVETOP | CONTEXT_MENU_MOVEBOTTOM)

class ListView
{
public:
    struct HeaderItem
    {
        UINT ids;
        int width;
    };
private:
    struct ItemData
    {
        std::vector<std::wstring> texts;
        int checked;    // -1: not use, 0: unchecked, 1: checked
    };
public:
    ListView(HWND hWnd, IWnd* parent);
    ~ListView();

    void setCheckBox(bool enabled);
    void setContextMenuType(UINT type);
    void setHeader(const std::vector<HeaderItem>& header);
    void addHeader(UINT ids, int width);
    void addHeader(const wchar_t* text, int width);
    int  headerCount() const;
    void clearHeader();

    void addItem(int row, const wchar_t* text, void* userData = NULL);
    void addItem(int row, const std::vector<std::wstring>& texts, void* userData = NULL);
    void setItem(int row, int col, const wchar_t* text);
    void deleteItem(int row);
    void setItemChecked(int row, bool checked);
    void setItemSelected(int row, bool selected, bool sync = true);
    void clearItems();
    int  itemCount() const;
    int  selectedItem() const;
    void setItemUserData(int row, void* userData);
    void* itemUserData(int row);

protected:
    LRESULT onListItemChanged(LPNMLISTVIEW pnmv);
    LRESULT onListCustomDraw(LPNMLVCUSTOMDRAW customDraw);
    LRESULT onListContextMenu(const POINT& pt);
    LRESULT onDeferredSelectionNotify();

protected:
    void onListItemOpen(int row);
    void onListItemPreview(int row);
    void onListItemAdd();
    void onListItemDelete(int row);
    void onListItemEdit(int row);
    void onListItemClear();
    void onListItemMoveTop(int row);
    void onListItemMoveUp(int row);
    void onListItemMoveDown(int row);
    void onListItemMoveBottom(int row);

protected:
    ItemData itemData(int row);
    void setItemData(int row, const ItemData& data);
    void swapItemData(int row1, int row2);

public:
    LRESULT onNotify(WPARAM wParam, LPARAM lParam);
protected:
    static LRESULT CALLBACK subclassProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubClass, DWORD_PTR udata);

protected:
    HWND m_hWnd;
    IWnd* m_parent;
    UINT m_menuType;
    bool m_checkEnabled;
    bool m_updating;
    bool m_pendingSelectionNotify;
};
