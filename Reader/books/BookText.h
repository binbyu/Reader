#pragma once
#include "BookBase.h"

class BookText : public BookBase
{
public:
    BookText(const std::wstring& fileName);
    virtual ~BookText();
    virtual BookType type() const;
    virtual bool isEditSupport() const;
    virtual bool editText(int start, int srcLen, wchar_t* const dstText, int dstLen);

protected:
    virtual BookErrno parseBook();
    virtual BookErrno readFile(char*& data, int& size);
    virtual BookErrno saveFile();

protected:
    bool parserChapters();
    bool parserChaptersDefault();
    bool parserChaptersKeyword();
    bool parserChaptersRegex();
    bool isValidNumber(wchar_t* text, int len);
    bool isValidChapter(wchar_t text);
    bool isValidSeparator(wchar_t text);
    bool moveChapterIndex(int index, int delta);
};