#pragma once
#include "Dialog.h"
#include "Settings.h"

class DisplaySettingDlg : public Dialog
{
public:
    DisplaySettingDlg(HINSTANCE hInstance, HWND hParent);
    ~DisplaySettingDlg();

    bool isChanged() const;

protected:
    LRESULT onInitDialog();
    LRESULT onSave();
    LRESULT onUnifiedTextStyle();
    LRESULT onTextFont();
    LRESULT onTitleFont();
    LRESULT onBackgroundColor();
    LRESULT onBackgroundImageEnable();
    LRESULT onBrowserBackgroundImage();
    LRESULT onBackgroundImageFileChanged();
    LRESULT onBackgroundImageModeChanged();
    LRESULT onBackgroundAlphaChanged();
    LRESULT onTextAlphaChanged();
    LRESULT onUpdatePreview(HDC hdc, RECT rect);

protected:
    void updatePreview();
    std::wstring getBorder(const RECT& rect);
    bool getBorder(const wchar_t* str, RECT& rect, bool tips = false);
    void bindSliderEditPair(int sliderId, int editId, BYTE& value, int minValue, int maxValue);
    void setSliderValue(int sliderId, int value);
    void setEditValue(int editId, int value);
    bool handleSliderChange(int sliderId, int editId, BYTE& value);
    bool handleEditChange(int editId, int sliderId, BYTE& value, int minValue, int maxValue);
    void startPickColor();
    void stopPickColor();
    void updatePickColor(COLORREF color);
    bool openFileDialog(std::wstring& fileName);
    bool loadData();

protected:
    virtual LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam);
    static UINT_PTR CALLBACK chooseFontProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static LRESULT  CALLBACK chooseFontComboProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubClass, DWORD_PTR udata);
public:
    static bool openFontDialog(HWND hWnd, LOGFONTW& font, COLORREF& textColor);
    static bool openColorDialog(HWND hWnd, COLORREF& color);

protected:
    HCURSOR m_pickerCursor;
    bool m_isPickingColor;
    TextInfo m_styles;
    BYTE m_alpha;
    BYTE m_textAlpha;
    BackgroundInfo m_background;
    LayoutConfig m_layout;
    bool m_ignoreAlphaEditChange;
    bool m_changed;
};