#include "StdAfx.h"
#include "Cache.h"
#include <stdio.h>
#include <string.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

extern UINT GetAppVersion(void);

Cache::Cache(TCHAR* file)
{
    GetModuleFileName(NULL, m_file_name, MAX_PATH-1);
    for (int i=_tcslen(m_file_name)-1; i>=0; i--)
    {
        if (m_file_name[i] == _T('\\'))
        {
            memcpy(&m_file_name[i+1], file, (_tcslen(file)+1)*sizeof(TCHAR));
            break;
        }
    }
    m_buffer = NULL;
    m_size = 0;
}


Cache::~Cache(void)
{
    if (m_buffer)
    {
        free(m_buffer);
        m_buffer = NULL;
    }
    m_size = 0;
}

bool Cache::init()
{
    FILE* fp = NULL;

    if (!PathFileExists(m_file_name)) // not exist
    {
        if (!default_header())
            return false;
    }
    else
    {
        if (read())
        {
            if (get_header()->version != GetAppVersion())
            {
                // remove old cache data
                DeleteFile(m_file_name);
                free(m_buffer);
                m_buffer = NULL;
                if (!default_header())
                    return false;
            }
            return true;
        }
        return false;
    }

    return true;
}

bool Cache::exit()
{
    FILE* fp = NULL;
    bool result = true;

    if (m_buffer)
    {
        result = write();

        // free
        free(m_buffer);
        m_buffer = NULL;
        m_size = 0;
    }

    return result;
}

header_t* Cache::get_header()
{
    return (header_t*)m_buffer;
}

item_t* Cache::get_item(int item_id)
{
    header_t* header = get_header();
    int offset = sizeof(header_t) + item_id*sizeof(item_t);
    if (item_id >= header->size)
        return NULL;
    return (item_t*)((char*)m_buffer + offset);
}

item_t* Cache::open_item(int item_id)
{
    header_t* header = get_header();
    item_t* item = get_item(item_id);

    // move to index 0
    move_item(item->id, 0);
    item = get_item(0);

    if (header && item)
    {
        header->item_id = item_id;
    }

    return item;
}

item_t* Cache::new_item(u128_t* item_md5, TCHAR* file_name)
{
    header_t* header = NULL;
    item_t* item = find_item(item_md5, file_name);
    int item_id = -1;

    // already exist
    if (item)
    {
        //return item;
        return NULL;
    }

    // new item
    m_buffer = realloc(m_buffer, m_size+sizeof(item_t));
    if (!m_buffer)
        return NULL;
    m_size += sizeof(item_t);
    header = get_header();
    item_id = header->size++;

    // init item
    item = get_item(item_id);
    memset(item, 0, sizeof(item_t));
    item->id = item_id;
    memcpy(&item->md5, item_md5, sizeof(u128_t));
    memcpy(item->file_name, file_name, MAX_PATH);

    // move to index 0
    move_item(item->id, 0);
    item = get_item(0);

    return item;
}

item_t* Cache::find_item(u128_t* item_md5, TCHAR* file_name)
{
    header_t* header = get_header();
    item_t* item = NULL;

    if (header->size <= 0)
        return NULL;

    for (int i=0; i<header->size; i++)
    {
        item = get_item(i);
        if (0 == memcmp(&item->md5, item_md5, sizeof(u128_t)))
        {
            // update file name
            if (0 != memcmp(item->file_name, file_name, MAX_PATH))
            {
                memcpy(item->file_name, file_name, MAX_PATH);
            }
            return item;
        }
    }
    return NULL;
}

bool Cache::delete_item(int item_id)
{
    header_t* header = get_header();
    item_t* item = get_item(item_id);
    if (!item)
        return false;

    for (int i=item_id+1; i<header->size; i++)
    {
        item_t* item_1 = get_item(i);
        item_t* item_2 = get_item(i-1);
        item_1->id--;
        memcpy(item_2, item_1, sizeof(item_t));
    }
    header->size--;
    m_size -= sizeof(item_t);
    return true;
}

header_t* Cache::default_header()
{
    header_t* header = NULL;

    if (!m_buffer)
    {
        m_size = sizeof(header_t);
        m_buffer = malloc(m_size);
        if (!m_buffer)
            return NULL;
        memset(m_buffer, 0, m_size);
        header = (header_t*)m_buffer;
        header->item_id = -1;
    }
    else
    {
        header = (header_t*)m_buffer;
    }
    
    // default font
    static HFONT hFont = NULL;
    static LOGFONT lf;
    if (!hFont)
    {
        hFont = CreateFont(20, 0, 0, 0,
            FW_THIN,false,false,false,
            ANSI_CHARSET,OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_SWISS,_T("Consolas"));
        GetObject(hFont, sizeof(lf), &lf);
    }
    memcpy(&header->font, &lf, sizeof(lf));
    header->font_color = 0x00;      // black

    // default rect
    int width = 300;
    int height = 500;
    header->rect.left = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    header->rect.top = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;
    header->rect.right = header->rect.left + width;
    header->rect.bottom = header->rect.top + height;

    // default bk color
    header->bk_color = 0x00ffffff;  // White

    header->line_gap = 5;
    header->internal_border = 0;
    header->version = GetAppVersion();

    // default hotkey
    header->hk_show_1 = 0;
    header->hk_show_2 = MOD_ALT;
    header->hk_show_3 = 'H';
    header->wheel_speed = 1;
    header->page_mode = 1;

    return header;
}

bool Cache::move_item(int from, int to)
{
    // from and to is exist
    item_t temp = {0};
    item_t* from_item = get_item(from);
    item_t* to_item = get_item(to);
    if (!from_item || !to_item)
    {
        return false;
    }

    if (from == to)
    {
        return true;
    }
    else if (from > to)
    {
        memcpy(&temp, from_item, sizeof(item_t));
        for (int i=from-1; i>=to; i--)
        {
            item_t* item_1 = get_item(i);
            item_t* item_2 = get_item(i+1);
            item_1->id++;
            memcpy(item_2, item_1, sizeof(item_t));
        }
        temp.id = to;
        memcpy(to_item, &temp, sizeof(item_t));
    }
    else
    {
        memcpy(&temp, from_item, sizeof(item_t));
        for (int i=from+1; i<to; i++)
        {
            item_t* item_1 = get_item(i);
            item_t* item_2 = get_item(i-1);
            item_1->id--;
            memcpy(item_2, item_1, sizeof(item_t));
        }
        temp.id = to;
        memcpy(to_item, &temp, sizeof(item_t));
    }
    return true;
}

bool Cache::read()
{
    HANDLE hFile = NULL;
    BOOL bErrorFlag = FALSE;
    DWORD dwFileSize = 0;
    DWORD dwBytesRead = 0;

    hFile = CreateFile(m_file_name,                // file to open
        GENERIC_READ,          // open for reading
        FILE_SHARE_READ,       // share for reading
        NULL,                  // default security
        OPEN_EXISTING,         // existing file only
        FILE_ATTRIBUTE_HIDDEN, // hidden file
        NULL);                 // no attr. template

    if (hFile == INVALID_HANDLE_VALUE) 
    {
        return false;
    }

    dwFileSize = GetFileSize(hFile, NULL);
    if (INVALID_FILE_SIZE == dwFileSize)
    {
        CloseHandle(hFile);
        return false;
    }

    m_buffer = malloc(dwFileSize);
    if (!m_buffer)
    {
        CloseHandle(hFile);
        return false;
    }
    m_size = dwFileSize;

    bErrorFlag = ReadFile(hFile,
        m_buffer,
        dwFileSize,
        &dwBytesRead,
        NULL);

    if (FALSE == bErrorFlag)
    {
        CloseHandle(hFile);
        return false;
    }
    else
    {
        if (dwBytesRead != m_size)
        {
            CloseHandle(hFile);
            return false;
        }
    }

    CloseHandle(hFile);
    return true;
}

bool Cache::write()
{
    HANDLE hFile = NULL;
    BOOL bErrorFlag = FALSE;
    DWORD dwBytesWritten = 0;

    hFile = CreateFile(m_file_name,                // name of the write
        GENERIC_WRITE,          // open for writing
        0,                      // do not share
        NULL,                   // default security
        CREATE_ALWAYS,          // create new file only
        FILE_ATTRIBUTE_HIDDEN,  // hidden file
        NULL);                  // no attr. template
    if (hFile == INVALID_HANDLE_VALUE) 
    {
        return false;
    }

    bErrorFlag = WriteFile( 
        hFile,           // open file handle
        m_buffer,        // start of data to write
        m_size,          // number of bytes to write
        &dwBytesWritten, // number of bytes that were written
        NULL);           // no overlapped structure

    if (FALSE == bErrorFlag)
    {
        CloseHandle(hFile);
        return false;
    }
    else
    {
        if (dwBytesWritten != m_size)
        {
            CloseHandle(hFile);
            return false;
        }
    }

    CloseHandle(hFile);
    return true;
}
