#ifndef __EPUB_BOOK_H__
#define __EPUB_BOOK_H__

#include "Book.h"
#include <string>
#include <map>
#include <vector>

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
