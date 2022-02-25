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
    virtual bool SaveBook(HWND hWnd);
    virtual bool UpdateChapters(int offset);

protected:
    virtual bool ParserBook(HWND hWnd);
    bool ReadBook(void);
    bool ParserChapters(void);
    bool ParserChaptersDefault(void);
    bool ParserChaptersKeyword(void);
    bool ParserChaptersRegex(void);
    bool GetLine(wchar_t* text, int len, int* line_size);
    bool IsChapter(wchar_t* text, int len);

protected:
    static wchar_t m_ValidChapter[];
};

#endif
