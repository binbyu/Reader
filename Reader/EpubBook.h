#ifndef __EPUB_BOOK_H__
#define __EPUB_BOOK_H__

#include "Book.h"
#include <string>
#include <map>
#include <vector>

typedef struct file_data_t
{
    void *data;
    int size;
} file_data_t;
typedef std::map<std::string, file_data_t> filelist_t;

typedef struct manifest_t
{
    std::string id;
    std::string href;
    std::string media_type;
} manifest_t;
typedef std::map<std::string, manifest_t *> manifests_t;

typedef std::vector<std::string> spines_t;

typedef struct navpoint_t
{
    std::string id;
    std::string src;
    std::string text;
    int order;
} navpoint_t;
typedef std::map<std::string, navpoint_t *> navpoints_t;

typedef struct epub_t
{
    std::string path;
    std::string ocf;
    std::string opf;
    std::string ncx;
    manifests_t manifests;
    spines_t spines;
    navpoints_t navpoints;
} epub_t;


class EpubBook : public Book
{
public:
    EpubBook();
    virtual ~EpubBook();

public:
    virtual book_type_t GetBookType(void);
    virtual BOOL SaveBook(HWND hWnd);
    virtual BOOL UpdateChapters(int offset);

protected:
    virtual BOOL ParserBook(HWND hWnd);
    virtual Gdiplus::Bitmap* GetCover(void);
    virtual int GetTextBeginIndex(void);
    void FreeFilelist(void);
    BOOL UnzipBook(void);
    BOOL ParserOcf(epub_t &epub);
    BOOL ParserOpf(epub_t &epub);
    BOOL ParserNcx(epub_t &epub);
    BOOL ParserOps(file_data_t *fdata, wchar_t **text, int *len, wchar_t **title, int *tlen, BOOL parsertitle);
    BOOL ParserChapters(epub_t &epub);
    BOOL ParserCover(epub_t &epub);

protected:
    Gdiplus::Bitmap *m_Cover;
    filelist_t m_flist;
};

#endif
