#ifndef __TEXT_BOOK_H__
#define __TEXT_BOOK_H__

#include "Book.h"


class TextBook : public Book
{
public:
    TextBook();
    virtual ~TextBook();

public:
    virtual book_type_t GetBookType(void);
    virtual BOOL SaveBook(HWND hWnd);
    virtual BOOL UpdateChapters(int offset);

protected:
    virtual BOOL ParserBook(HWND hWnd);
    BOOL ReadBook(void);
    BOOL ParserChapters(void);
    BOOL ParserChaptersDefault(void);
    BOOL ParserChaptersKeyword(void);
    BOOL ParserChaptersRegex(void);
    BOOL IsChapter(wchar_t* text, int len);

protected:
    static wchar_t m_ValidChapter[];
};

#endif
