
// UpgraderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Upgrader.h"
#include "UpgraderDlg.h"
#include "afxdialogex.h"
#include "cJSON.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CUpgraderDlg dialog




CUpgraderDlg::CUpgraderDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CUpgraderDlg::IDD, pParent)
    , m_szVersion(_T(""))
    , m_bForce(FALSE)
    , m_szUrl(_T(""))
    , m_szDesc(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CUpgraderDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST, m_List);
    DDX_Text(pDX, IDC_EDIT_VERSION, m_szVersion);
    DDX_Check(pDX, IDC_CHECK_FORCE, m_bForce);
    DDX_Text(pDX, IDC_EDIT_URL, m_szUrl);
    DDX_Text(pDX, IDC_EDIT_DESC, m_szDesc);
}

BEGIN_MESSAGE_MAP(CUpgraderDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_WM_CLOSE()
    ON_BN_CLICKED(IDC_BUTTON_SAVE, &CUpgraderDlg::OnBnClickedButtonSave)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST, &CUpgraderDlg::OnNMDblclkList)
    ON_NOTIFY(NM_RCLICK, IDC_LIST, &CUpgraderDlg::OnNMRClickList)
    ON_COMMAND(ID_DELETE, &CUpgraderDlg::OnDelete)
    ON_COMMAND(ID_OPEN, &CUpgraderDlg::OnOpen)
END_MESSAGE_MAP()


// CUpgraderDlg message handlers

BOOL CUpgraderDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
    json_data_t::reverse_iterator itor;

    ReadJsonFile();
    for (itor=m_Json.rbegin(); itor!=m_Json.rend(); itor++)
    {
        m_List.InsertItem(0, itor->first.c_str());
    }

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CUpgraderDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CUpgraderDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CUpgraderDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CUpgraderDlg::OnClose()
{
    // TODO: Add your message handler code here and/or call default
    json_data_t::iterator itor;
    json_item_data_t *item;
    SaveJsonFile();
    
    for (itor = m_Json.begin(); itor!=m_Json.end(); itor++)
    {
        item = itor->second;
        delete item;
    }
    m_Json.clear();

    CDialogEx::OnClose();
}

void CUpgraderDlg::OnBnClickedButtonSave()
{
    // TODO: Add your control notification handler code here
    json_data_t::iterator itor;
    json_data_t::reverse_iterator ritor;
    json_item_data_t *item;

    UpdateData(TRUE);
    if (m_szVersion.IsEmpty())
    {
        MessageBox(_T("Please input version!"), _T("Error"), MB_OK|MB_ICONERROR);
        return;
    }
    if (m_szUrl.IsEmpty())
    {
        MessageBox(_T("Please input download url!"), _T("Error"), MB_OK|MB_ICONERROR);
        return;
    }
    if (m_szDesc.IsEmpty())
    {
        MessageBox(_T("Please input descripton!"), _T("Error"), MB_OK|MB_ICONERROR);
        return;
    }
    itor = m_Json.find(std::wstring(m_szVersion));
    if (itor != m_Json.end())
    {
        item = itor->second;
        item->url = m_szUrl;
        item->desc = m_szDesc;
        item->force = m_bForce;
    }
    else
    {
        item = new json_item_data_t;
        item->version = m_szVersion;
        item->url = m_szUrl;
        item->desc = m_szDesc;
        item->force = m_bForce;
        m_Json.insert(std::make_pair(item->version, item));

        m_List.DeleteAllItems();
        for (ritor=m_Json.rbegin(); ritor!=m_Json.rend(); ritor++)
        {
            m_List.InsertItem(0, ritor->first.c_str());
        }
    }
}

void CUpgraderDlg::OnNMDblclkList(NMHDR *pNMHDR, LRESULT *pResult)
{
    json_data_t::iterator itor;
    CString szVersion;
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // TODO: Add your control notification handler code here
    *pResult = 0;

    szVersion = m_List.GetItemText(pNMItemActivate->iItem, pNMItemActivate->iSubItem);
    itor = m_Json.find(std::wstring(szVersion));
    if (itor != m_Json.end())
    {
        m_szVersion = itor->second->version.c_str();
        m_szUrl = itor->second->url.c_str();
        m_szDesc = itor->second->desc.c_str();
        m_bForce = itor->second->force == 1 ? TRUE : FALSE;
    }
    else
    {
        m_szVersion.Empty();
        m_szUrl.Empty();
        m_szDesc.Empty();
        m_bForce = FALSE;
    }
    UpdateData(FALSE);
}


void CUpgraderDlg::OnNMRClickList(NMHDR *pNMHDR, LRESULT *pResult)
{
    POSITION pos;
    LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
    // TODO: Add your control notification handler code here
    *pResult = 0;

    pos = m_List.GetFirstSelectedItemPosition();
    if (pos == NULL)
    {
        return;
    }

    CMenu menu;
    CMenu *pSubMenu;
    menu.LoadMenu(IDR_MENU_RC);
    pSubMenu = menu.GetSubMenu(0);
    CPoint point;
    GetCursorPos(&point);
    pSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, point.x, point.y, this);
    pSubMenu->DestroyMenu();
    menu.DestroyMenu();
}

void CUpgraderDlg::ReadJsonFile()
{
    FILE *fp;
    char *buf;
    int len;
    cJSON *root, *list, *item, *version, *url, *desc, *force;
    int i, size;
    json_item_data_t *it;

    fp = _tfopen(JSON_FILE, _T("rb"));
    if (!fp)
        return;

    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buf = (char *)malloc(len + 1);
    buf[len] = 0;
    fread(buf, 1, len, fp);
    fclose(fp);

    root = cJSON_Parse(buf);
    list = cJSON_GetObjectItem(root, "list");
    size = cJSON_GetArraySize(list);
    for (i = 0; i < size; i++)
    {
        item = cJSON_GetArrayItem(list, i);
        version = cJSON_GetObjectItem(item, "version");
        url = cJSON_GetObjectItem(item, "downloadurl");
        desc = cJSON_GetObjectItem(item, "description");
        force = cJSON_GetObjectItem(item, "force");

        it = new json_item_data_t;
        Utf8ToUnicode(version->valuestring, it->version);
        Utf8ToUnicode(url->valuestring, it->url);
        Utf8ToUnicode(desc->valuestring, it->desc);
        it->force = force->valueint;
        m_Json.insert(std::make_pair(it->version, it));
    }
    cJSON_Delete(root);
    free(buf);
}

void CUpgraderDlg::SaveJsonFile()
{
    cJSON *root, *list, *item;
    json_data_t::iterator itor;
    std::string temp;
    char *buf;
    FILE *fp;

    root = cJSON_CreateObject();
    list = cJSON_CreateArray();
    for (itor = m_Json.begin(); itor != m_Json.end(); itor++)
    {
        item = cJSON_CreateObject();
        UnicodeToUtf8(itor->second->version.c_str(), temp);
        cJSON_AddStringToObject(item, "version", temp.c_str());
        UnicodeToUtf8(itor->second->url.c_str(), temp);
        cJSON_AddStringToObject(item, "downloadurl", temp.c_str());
        UnicodeToUtf8(itor->second->desc.c_str(), temp);
        cJSON_AddStringToObject(item, "description", temp.c_str());
        cJSON_AddBoolToObject(item, "force", itor->second->force);
        cJSON_AddItemToArray(list, item);
    }
    cJSON_AddItemToObject(root, "list", list);

    buf = cJSON_PrintUnformatted(root);

    fp = _tfopen(JSON_FILE, _T("wb"));
    fwrite(buf, 1, strlen(buf), fp);
    fclose(fp);

    cJSON_Delete(root);
}

void CUpgraderDlg::Utf8ToUnicode(const char *utf8, std::wstring &uni)
{
    wchar_t* result;
    int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    result = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
    memset(result, 0, (len + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, (LPWSTR)result, len);
    uni = result;
    free(result);
}

void CUpgraderDlg::UnicodeToUtf8(const wchar_t *uni, std::string &utf8)
{
    char* result;
    int len = WideCharToMultiByte(CP_UTF8, 0, uni, -1, NULL, 0, NULL, NULL);
    result = (char*)malloc((len + 1) * sizeof(char));
    memset(result, 0, (len + 1) * sizeof(char));
    WideCharToMultiByte(CP_UTF8, 0, uni, -1, result, len, NULL, NULL);
    utf8 = result;
    free(result);
}


void CUpgraderDlg::OnDelete()
{
    // TODO: Add your command handler code here
    json_data_t::iterator itor;
    CString szVersion;
    POSITION pos;
    int nItem;

    pos = m_List.GetFirstSelectedItemPosition();
    if (pos == NULL)
    {
        return;
    }
    nItem = m_List.GetNextSelectedItem(pos);

    szVersion = m_List.GetItemText(nItem, 0);
    itor = m_Json.find(std::wstring(szVersion));
    if (itor != m_Json.end())
    {
        delete itor->second;
        m_Json.erase(itor);
    }

    m_List.DeleteItem(nItem);
}


void CUpgraderDlg::OnOpen()
{
    // TODO: Add your command handler code here
    json_data_t::iterator itor;
    CString szVersion;
    POSITION pos;
    int nItem;

    pos = m_List.GetFirstSelectedItemPosition();
    if (pos == NULL)
    {
        return;
    }
    nItem = m_List.GetNextSelectedItem(pos);

    szVersion = m_List.GetItemText(nItem, 0);
    itor = m_Json.find(std::wstring(szVersion));
    if (itor != m_Json.end())
    {
        m_szVersion = itor->second->version.c_str();
        m_szUrl = itor->second->url.c_str();
        m_szDesc = itor->second->desc.c_str();
        m_bForce = itor->second->force == 1 ? TRUE : FALSE;
    }
    else
    {
        m_szVersion.Empty();
        m_szUrl.Empty();
        m_szDesc.Empty();
        m_bForce = FALSE;
    }
    UpdateData(FALSE);
}
