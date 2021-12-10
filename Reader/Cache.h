#ifndef __CACHE_H__
#define __CACHE_H__

#include "types.h"


class Cache
{
public:
    Cache(TCHAR* file);
    ~Cache(void);

    bool init();
    bool exit();
    bool save();
    header_t* get_header();
    item_t* get_item(int item_id);
    item_t* open_item(int item_id);
#if ENABLE_MD5
    item_t* new_item(u128_t* item_md5, TCHAR* file_name);
    item_t* find_item(u128_t* item_md5, TCHAR* file_name);
#else
    item_t* new_item(TCHAR* file_name);
    item_t* find_item(TCHAR* file_name);
#endif
    bool delete_item(int item_id);
    bool delete_all_item(void);
    header_t* default_header();
    bool add_mark(item_t *item, int value);
    bool del_mark(item_t *item, int index);

private:
    void default_header(header_t* header);
    bool move_item(int from, int to);
    bool read(void **data, int *size);
    bool write(void *data, int size);
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