
// UpgraderDlg.h : header file
//

#pragma once
#include "afxcmn.h"
#include <string>
#include <map>

#define JSON_FILE           _T("version.json")

typedef struct json_item_data_t 
{
    std::wstring version;
    std::wstring url;
    std::wstring desc;
    int force;
} json_item_data_t;

typedef std::map<std::wstring, json_item_data_t*> json_data_t;


// CUpgraderDlg dialog
class CUpgraderDlg : public CDialogEx
{
// Construction
public:
	CUpgraderDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_UPGRADER_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
    afx_msg void OnClose();
    afx_msg void OnBnClickedButtonSave();
    afx_msg void OnNMDblclkList(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMRClickList(NMHDR *pNMHDR, LRESULT *pResult);
	DECLARE_MESSAGE_MAP()

private:
    void ReadJsonFile();
    void SaveJsonFile();

    void Utf8ToUnicode(const char *utf8, std::wstring &uni);
    void UnicodeToUtf8(const wchar_t *uni, std::string &utf8);

private:
    CListCtrl m_List;
    CString m_szVersion;
    BOOL m_bForce;
    CString m_szUrl;
    CString m_szDesc;
    json_data_t m_Json;
public:
    afx_msg void OnDelete();
    afx_msg void OnOpen();
};
