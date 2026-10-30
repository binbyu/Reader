#pragma once
#include "IBook.h"

class BookBase : public IBook
{
public:
    BookBase(const std::wstring& fileName);
    virtual                ~BookBase();
    virtual int            length() const;
    virtual const wchar_t* text() const;
    virtual bool           isTitleLine(int index) const;
    virtual bool           isChapterStart(int index) const;
    virtual bool           isChapterContinuous(bool isNext, int index) const;
    virtual bool           isCover(int index) const;
    virtual bool           hasCover() const;

    virtual const wchar_t* fileName() const;
    virtual const wchar_t* bookName() const;
    virtual void           setChapterRule(const ChapterRule* rule);
    virtual void           setBookInfo(BookInfo* bookInfo);
    virtual BookErrno      open();
    virtual void           close();
    virtual bool           isAbort() const;
    virtual int            index() const;
    virtual bool           setIndex(int index);
    virtual int            curChapterIndex() const;
    virtual int            chapterCount() const;
    virtual const Chapter* chapter(int chapterIndex) const;
    virtual const Cover*   cover() const;
    virtual bool           isEditSupport() const;
    virtual bool           editText(int start, int srcLen, wchar_t* const dstText, int dstLen);
    virtual bool           isFetchSupport() const;
    virtual bool           fetchChapter(int chapterIndex, AsyncCallback cb = nullptr);
    virtual bool           isRefreshSupport() const;
    virtual bool           refreshChapter(AsyncCallback cb);
    virtual bool           refreshContent(int chapterIndex, AsyncCallback cb);
    virtual LRESULT        onEvent(WPARAM wParam, LPARAM lParam);
    virtual LRESULT        onTimer(WPARAM wParam, LPARAM lParam);

protected:
    virtual BookErrno      parseBook() = 0;
    bool                   decodeText(const char* src, int slen, wchar_t** dst, int* dlen);
    bool                   formatText(wchar_t* text, int* length);
    bool                   formatTitle(wchar_t* text, int* length);
    void                   freeCover();

protected:
    wchar_t*               m_text;
    int                    m_length;
    BookInfo*              m_bookInfo;
    std::wstring           m_fileName;
    std::wstring           m_bookName;
    const ChapterRule*     m_chapterRule;
    Chapters               m_chapters;
    Cover*                 m_cover;
    bool                   m_abort;
};