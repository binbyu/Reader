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
    item_t* new_item(u128_t* item_md5, TCHAR* file_name);
    item_t* find_item(u128_t* item_md5, TCHAR* file_name);
    bool delete_item(int item_id);
    bool delete_all_item(void);
    header_t* default_header();
    bool add_mark(item_t *item, int value);
    bool del_mark(item_t *item, int index);

private:
    bool move_item(int from, int to);
    bool read();
    bool write();
    void update_addr(void);

private:
	TCHAR m_file_name[MAX_PATH];
    void* m_buffer;
    int   m_size;
};

#endif