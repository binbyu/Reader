#ifndef __CACHE_H__
#define __CACHE_H__

#include "types.h"

class Cache
{
public:
    Cache(const TCHAR* file);
    ~Cache(void);

    BOOL init();
    BOOL exit();
    BOOL save();
    header_t* get_header();
    item_t* get_item(int item_id);
    item_t* open_item(int item_id);
    item_t* new_item(TCHAR* file_name);
    item_t* find_item(TCHAR* file_name);
    BOOL delete_item(int item_id);
    BOOL delete_all_item(void);
    header_t* default_header();
    BOOL add_mark(item_t *item, int value);
    BOOL del_mark(item_t *item, int index);

private:
    void default_header(header_t* header);
    BOOL move_item(int from, int to);
    BOOL read(void **data, int *size);
    BOOL write(void *data, int size);
    void update_addr(void);
    void encode(void *data, int size);
    void decode(void* data, int size);

private:
    TCHAR m_file_name[MAX_PATH];
    void* m_buffer;
    int   m_size;
    void* m_jsonbak;
    int   m_jsonlen;
};

#endif