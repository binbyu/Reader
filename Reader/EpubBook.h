#ifndef __EPUB_BOOK_H__
#define __EPUB_BOOK_H__

#include "Book.h"
#include <string>
#include <map>
#include <vector>

typedef struct file_data_t
{
    void *data;
    size_t size;
} file_data_t;
typedef std::map<std::string, file_data_t> filelist_t;

typedef struct mainfest_item_t
{
    std::string id;
    std::string href;
    std::string media_type;
} mainfest_item_t;
typedef std::map<std::string, mainfest_item_t *> mainfest_t;

typedef std::vector<std::string> spine_t;

typedef struct navpoint_t
{
    std::string id;
    std::string src;
    std::string text;
    int order;
} navpoint_t;
typedef std::map<std::string, navpoint_t *> navmap_t;

typedef struct epub_t
{
    std::string path;
    std::string ocf;
    std::string opf;
    std::string ncx;
    mainfest_t mainfest;
    spine_t spine;
    navmap_t navmap;
} epub_t;


class EpubBook : public Book
{
public:
    EpubBook();
    virtual ~EpubBook();

public:
    virtual book_type_t GetBookType(void);
    virtual bool SaveBook(HWND hWnd);
    virtual bool UpdateChapters(int offset);
    Bitmap * GetCoverImage(void);

protected:
    virtual bool ParserBook(void);
    void FreeFilelist(void);
    bool UnzipBook(void);
    bool ParserOcf(epub_t &epub);
    bool ParserOpf(epub_t &epub);
    bool ParserNcx(epub_t &epub);
    bool ParserOps(file_data_t *fdata, wchar_t **text, int *len, wchar_t **title, int *tlen, bool parsertitle);
    bool ParserChapters(epub_t &epub);
    bool ParserCover(epub_t &epub);

protected:
    Bitmap *m_Cover;
    filelist_t m_flist;
};

#endif
