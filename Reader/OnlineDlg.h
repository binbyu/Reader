#ifndef __ONLINE_DLG_H__
#define __ONLINE_DLG_H__
#ifdef ENABLE_NETWORK

#include "types.h"

typedef struct ol_book_param_t
{
    TCHAR book_name[256];
    char main_page[1024];
    char host[1024];
    u32 is_finished;
} ol_book_param_t;

void OpenOnlineDlg(void);

#endif
#endif // !__ONLINE_DLG_H__
