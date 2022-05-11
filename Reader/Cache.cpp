#include "framework.h"
#include "Cache.h"
#include "Keyset.h"
#include "Upgrade.h"
#include "jsondata.h"
#include "DPIAwareness.h"
#include <stdio.h>
#include <string.h>
#include <shlwapi.h>


extern VOID GetCacheVersion(TCHAR *);

Cache::Cache(const TCHAR* file)
    : m_jsonbak(NULL)
    , m_jsonlen(0)
{
    size_t i;
    GetModuleFileName(NULL, m_file_name, sizeof(TCHAR)*(MAX_PATH-1));
    for (i=_tcslen(m_file_name)-1; i>=0; i--)
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

BOOL Cache::init()
{
    void* json = NULL;
    int size = 0;
    header_t *header = NULL;

    if (!PathFileExists(m_file_name)) // not exist
    {
        if (!default_header())
            return FALSE;
    }
    else
    {
        if (read(&json, &size))
        {
            // backup file data
            if (m_jsonbak)
            {
                free(m_jsonbak);
                m_jsonbak = NULL;
            }
            m_jsonlen = size;
            m_jsonbak = malloc(m_jsonlen);
            memcpy(m_jsonbak, json, m_jsonlen);

            header = (header_t*)malloc(sizeof(header_t));
            // parser json string
            decode(json, size);
            memset(header, 0, sizeof(header_t));
            header->item_id = -1;
            default_header(header);
            { // fixed bug
                json = realloc(json, size+1);
                if (!json)
                {
                    free(header);
                    return FALSE;
                }
                ((char*)json)[size] = 0;
            }
            parser_json((const char *)json, header, &m_buffer, &m_size);
            free(json);
            free(header);
            return TRUE;
        }
        return FALSE;
    }

    return TRUE;
}

BOOL Cache::exit()
{
    BOOL result = TRUE;

    if (m_buffer)
    {
        result = save();

        // free
        free(m_buffer);
        m_buffer = NULL;
        m_size = 0;
    }

    if (m_jsonbak)
    {
        free(m_jsonbak);
        m_jsonbak = NULL;
        m_jsonlen = 0;
    }

    return result;
}

BOOL Cache::save()
{
    BOOL result = FALSE;
    char* json = NULL;
    int size = 0;

    json = create_json(get_header());
    if (json)
    {
        size = (int)strlen(json);
        encode(json, size);

        // check if data is changed
        if (size != m_jsonlen
            || 0 != memcmp(m_jsonbak, json, m_jsonlen))
        {
            result = write(json, size);

            // backup
            if (size != m_jsonlen)
            {
                m_jsonlen = size;
                m_jsonbak = realloc(m_jsonbak, m_jsonlen);
                if (!m_jsonbak)
                {
                    m_jsonlen = 0;
                }
            }
            if (m_jsonbak)
                memcpy(m_jsonbak, json, m_jsonlen);
        }
        else
        {
            result = TRUE; // no change, don't need write file
        }
    }
    create_json_free(json);
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
    if (item_id >= header->item_count)
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

item_t* Cache::new_item(TCHAR* file_name)
{
    header_t* header = NULL;
    item_t* item = find_item(file_name);
    
    int item_id = -1;
    void *oldAddr = NULL;

    // already exist
    if (item)
    {
        //return item;
        return NULL;
    }

    // new item
    oldAddr = m_buffer;
    m_buffer = realloc(m_buffer, m_size+sizeof(item_t));
    if (!m_buffer)
        return NULL;
    if (oldAddr != m_buffer)
        update_addr();
    m_size += sizeof(item_t);
    header = get_header();
    item_id = header->item_count++;

    // init item
    item = get_item(item_id);
    memset(item, 0, sizeof(item_t));
    item->id = item_id;
    memcpy(item->file_name, file_name, sizeof(TCHAR) * (_tcslen(file_name) + 1));

    // move to index 0
    move_item(item->id, 0);
    item = get_item(0);

    return item;
}

item_t* Cache::find_item(TCHAR* file_name)
{
    header_t* header = get_header();
    item_t* item = NULL;

    if (header->item_count <= 0)
        return NULL;

    for (int i=0; i<header->item_count; i++)
    {
        item = get_item(i);
        if (0 == _tcscmp(item->file_name, file_name))
        {
            return item;
        }
    }
    return NULL;
}

BOOL Cache::delete_item(int item_id)
{
    header_t* header = get_header();
    item_t* item = get_item(item_id);
    if (!item)
        return FALSE;

    for (int i=item_id+1; i<header->item_count; i++)
    {
        item_t* item_1 = get_item(i);
        item_t* item_2 = get_item(i-1);
        item_1->id--;
        memcpy(item_2, item_1, sizeof(item_t));
    }
    header->item_count--;
    m_size -= sizeof(item_t);
    return TRUE;
}

BOOL Cache::delete_all_item(void)
{
    header_t* header = get_header();

    header->item_count = 0;
    header->item_id = -1;
    m_size = sizeof(header_t);
    return TRUE;
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

    default_header(header);	
    return header;
}

void Cache::default_header(header_t* header)
{
    // default font
    static HFONT hFont = NULL;
    static LOGFONT lf;
    int i;
    const int PointSize = 14;
    int nHeight;
    if (!hFont)
    {
        nHeight = -MulDiv(PointSize, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
        hFont = CreateFont(nHeight, 0, 0, 0,
            FW_LIGHT, FALSE, FALSE, FALSE,
            ANSI_CHARSET, OUT_STROKE_PRECIS,
            CLIP_STROKE_PRECIS, PROOF_QUALITY,
            VARIABLE_PITCH | FF_SWISS, _T("Segoe UI Light"));
        GetObject(hFont, sizeof(lf), &lf);
    }
    memcpy(&header->font, &lf, sizeof(lf));
    header->font_color = 0x00;      // black
    memcpy(&header->font_title, &lf, sizeof(lf));
    header->font_color_title = 0x00;  // black
    header->use_same_font = 1;

    // default style
    header->style = WS_OVERLAPPEDWINDOW;
    header->exstyle = WS_EX_ACCEPTFILES | WS_EX_LAYERED;
    header->fs_style = WS_OVERLAPPEDWINDOW;
    header->fs_exstyle = WS_EX_ACCEPTFILES | WS_EX_LAYERED;

    // default placement
    header->placement.length = sizeof(WINDOWPLACEMENT);
    header->placement.flags = 0;
    header->placement.showCmd = SW_SHOWNORMAL;
    header->placement.ptMinPosition.x = -1;
    header->placement.ptMinPosition.y = -1;
    header->placement.ptMaxPosition.x = -1;
    header->placement.ptMaxPosition.y = -1;
    header->placement.rcNormalPosition.left = (GetCxScreenForDpi() - DEFAULT_APP_WIDTH) / 2;
    header->placement.rcNormalPosition.top = (GetCyScreenForDpi() - DEFAULT_APP_HEIGHT) / 2;
    header->placement.rcNormalPosition.right = header->placement.rcNormalPosition.left + DEFAULT_APP_WIDTH;
    header->placement.rcNormalPosition.bottom = header->placement.rcNormalPosition.top + DEFAULT_APP_HEIGHT;
#ifdef _MAC
    header->placement.rcDevice.left = 0;
    header->placement.rcDevice.top = 0;
    header->placement.rcDevice.right = GetCxScreenForDpi();
    header->placement.rcDevice.bottom = GetCyScreenForDpi();
#endif

    header->fs_placement.length = sizeof(WINDOWPLACEMENT);
    header->fs_placement.flags = 0;
    header->fs_placement.showCmd = SW_SHOWNORMAL;
    header->fs_placement.ptMinPosition.x = -1;
    header->fs_placement.ptMinPosition.y = -1;
    header->fs_placement.ptMaxPosition.x = -1;
    header->fs_placement.ptMaxPosition.y = -1;
    header->fs_placement.rcNormalPosition.left = (GetCxScreenForDpi() - DEFAULT_APP_WIDTH) / 2;
    header->fs_placement.rcNormalPosition.top = (GetCyScreenForDpi() - DEFAULT_APP_HEIGHT) / 2;
    header->fs_placement.rcNormalPosition.right = header->fs_placement.rcNormalPosition.left + DEFAULT_APP_WIDTH;
    header->fs_placement.rcNormalPosition.bottom = header->fs_placement.rcNormalPosition.top + DEFAULT_APP_HEIGHT;
#ifdef _MAC
    header->fs_placement.rcDevice.left = 0;
    header->fs_placement.rcDevice.top = 0;
    header->fs_placement.rcDevice.right = GetCxScreenForDpi();
    header->fs_placement.rcDevice.bottom = GetCyScreenForDpi();
#endif

    // default bk color
    header->bg_color = 0x00ffffff;  // White
    header->alpha = 0xff;

    header->char_gap = 0;
    header->line_gap = 5;
    header->paragraph_gap = 15;
    header->left_line_count = 0;
    header->internal_border = { 0 };
    GetCacheVersion(header->version);

    // default others
    header->wheel_speed = ws_single_line;
    header->page_mode = 1;
    header->autopage_mode = 0;
    header->uElapse = 3000;

    header->bg_image.enable = 0;
    header->disable_lrhide = 1;
    header->disable_eschide = 1;

    header->show_systray = 0;
    header->hide_taskbar = 0;
    header->word_wrap = 0;
    header->line_indent = 1;
    header->blank_lines = 1;
    header->chapter_page = 0;

    // default keyset
    header->global_key = 0;
    KS_GetDefaultKeyset(header->keyset);

    // default chapter rule
    header->chapter_rule.rule = 0;

    header->meun_font_follow = 0;

    for (i = 0; i<MAX_CUST_COLOR_COUNT; i++)
    {
        header->cust_colors[i] = 0x00FFFFFF;
    }
    // default tags
#if ENABLE_TAG
    for (i = 0; i < MAX_TAG_COUNT; i++)
    {
        if (header->tags[i].font.lfHeight == 0)
        {
            memcpy(&header->tags[i].font, &lf, sizeof(lf));
        }
        if (header->tags[i].bg_color == 0)
        {
            header->tags[i].bg_color = 0x00FFFFFF;
        }
        //header->tags[i].enable = 0;
    }
#endif
}

BOOL Cache::add_mark(item_t *item, int value)
{
    int i;

    if (!item)
        return FALSE;
    if (item->mark_size >= MAX_MARK_COUNT)
        return FALSE;
    for (i=0; i<item->mark_size; i++)
    {
        if (value == item->mark[i])
            return FALSE;
    }
    item->mark[item->mark_size] = value;
    item->mark_size++;
    return TRUE;
}

BOOL Cache::del_mark(item_t *item, int index)
{
    if (!item)
        return FALSE;
    if (item->mark_size <= 0)
        return FALSE;
    if (index >= item->mark_size)
        return FALSE;

    // delete
    if (item->mark_size - index - 1 > 0)
    {
        memcpy(item->mark+index, item->mark+index+1, (item->mark_size-index-1)*sizeof(int));
    }
    item->mark_size--;
    item->mark[item->mark_size] = 0;
    return TRUE;
}

BOOL Cache::move_item(int from, int to)
{
    // from and to is exist
    item_t temp = {0};
    item_t* from_item = get_item(from);
    item_t* to_item = get_item(to);
    if (!from_item || !to_item)
    {
        return FALSE;
    }

    if (from == to)
    {
        return TRUE;
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
    return TRUE;
}

BOOL Cache::read(void** data, int* size)
{
    HANDLE hFile = NULL;
    BOOL bErrorFlag = FALSE;
    DWORD dwFileSize = 0;
    DWORD dwBytesRead = 0;

    *data = NULL;
    *size = 0;
    hFile = CreateFile(m_file_name,                // file to open
        GENERIC_READ,          // open for reading
        FILE_SHARE_READ,       // share for reading
        NULL,                  // default security
        OPEN_EXISTING,         // existing file only
        FILE_ATTRIBUTE_HIDDEN, // hidden file
        NULL);                 // no attr. template

    if (hFile == INVALID_HANDLE_VALUE) 
    {
        return FALSE;
    }

    dwFileSize = GetFileSize(hFile, NULL);
    if (INVALID_FILE_SIZE == dwFileSize)
    {
        CloseHandle(hFile);
        return FALSE;
    }

    *size = dwFileSize;
    *data = (char *)malloc(dwFileSize);
    if (!(*data))
    {
        CloseHandle(hFile);
        return FALSE;
    }

    bErrorFlag = ReadFile(hFile,
        *data,
        dwFileSize,
        &dwBytesRead,
        NULL);

    if (FALSE == bErrorFlag)
    {
        CloseHandle(hFile);
        free(*data);
        *size = 0;
        return FALSE;
    }
    else
    {
        if ((int)dwBytesRead != *size)
        {
            CloseHandle(hFile);
            free(*data);
            *size = 0;
            return FALSE;
        }
    }

    CloseHandle(hFile);
    return TRUE;
}

BOOL Cache::write(void* data, int size)
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
        return FALSE;
    }

    bErrorFlag = WriteFile( 
        hFile,           // open file handle
        data,        // start of data to write
        size,          // number of bytes to write
        &dwBytesWritten, // number of bytes that were written
        NULL);           // no overlapped structure

    if (FALSE == bErrorFlag)
    {
        CloseHandle(hFile);
        return FALSE;
    }
    else
    {
        if ((int)dwBytesWritten != size)
        {
            CloseHandle(hFile);
            return FALSE;
        }
    }

    CloseHandle(hFile);
    return TRUE;
}

void Cache::update_addr(void)
{
    header_t* header = NULL;

    header = (header_t*)m_buffer;

    KS_UpdateKeyset(header->keyset);
}

void Cache::encode(void* data, int size)
{
//#ifndef _DEBUG
    char* p = (char*)data;
    int i;

    for (i = 0; i < size; i++)
    {
        p[i] ^= 0x3a;
    }
//#endif
}

void Cache::decode(void* data, int size)
{
    encode(data, size);
}
