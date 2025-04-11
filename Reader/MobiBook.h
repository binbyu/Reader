#ifndef __MOBI_BOOK_H__
#define __MOBI_BOOK_H__

#include "Book.h"
#include <string>
#include <map>
#include <vector>

/* include libmobi header */
#include <mobi.h>

typedef struct mobi_t
{
    std::string path;           //mobi文件解开后的虚拟路径，这里没用上，直接置""
    std::string opf;            //电子书标准opf，包含电子书的全部相关信息：     metadata、manifest、spine、guide、tour（可无）标签
    std::string ncx;            //电子书的目录结构，目录可以分为多级。
                                //目录以 navMap作为一级目录根节点，该层级下的navPoint子节点为一级节点；navPoint下也可以有子节点navPoint，作为二级目录，三级目录等等
                                //目前只支持一级
    manifests_t manifests;      //opf中的manifest标签内容，全部内容文件列表
    spines_t spines;            //opf中的spine标签内容，文档的线性阅读顺序
                                //spine定义的是要看的电子书每个文件的阅读顺序；而ncx是目录结构，一个文件可能有子目录
                                //因此，ncx包含的navPoint节点与spine的itemref节点相比，可能多或相等
    navpoints_t navpoints;      //ncx包含的navPoint节点
} mobi_t;


class MobiBook : public Book
{
public:
    MobiBook();
    virtual ~MobiBook();

public:
    virtual book_type_t GetBookType(void);
    virtual BOOL SaveBook(HWND hWnd);
    virtual BOOL UpdateChapters(int offset);

protected:
    virtual BOOL ParserBook(HWND hWnd);
    virtual Gdiplus::Bitmap* GetCover(void);
    virtual int GetTextBeginIndex(void);
    void FreeFilelist(void);
    BOOL UnzipBook(MOBIRawml *rawml, MOBIData *m, mobi_t &mobi);
    BOOL ParserOcf(mobi_t &mobi);
    BOOL ParserOpf(mobi_t &mobi);
    BOOL ParserNcx(mobi_t &mobi);
    BOOL ParserOps(file_data_t *fdata, wchar_t **text, int *len, wchar_t **title, int *tlen, BOOL parsertitle);
    BOOL ParserChapters(mobi_t &mobi);
    BOOL ParserCover(mobi_t &mobi, MOBIData *m);
    
protected:
    Gdiplus::Bitmap *m_Cover;
    filelist_t m_flist;
};

#endif
