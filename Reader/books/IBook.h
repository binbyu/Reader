#pragma once
#include <string>
#include <vector>
#include <functional>
#include "types.h"

using AsyncCallback = std::function<void(BookErrno err)>;

class ISource
{
public:
    virtual                ~ISource() = default;
    virtual const wchar_t* text() const = 0;
    virtual int            length() const = 0;
    virtual bool           isTitleLine(int index) const = 0;
    virtual bool           isChapterStart(int index) const = 0;
    virtual bool           isChapterContinuous(bool isNext, int index) const = 0;
    virtual bool           isCover(int index) const = 0;
    virtual bool           hasCover() const = 0;
};

class IBook : public ISource
{
public:
    virtual                ~IBook() = default;
    virtual BookType       type() const = 0;
    virtual const wchar_t* fileName() const = 0;
    virtual const wchar_t* bookName() const = 0;
    virtual void           setChapterRule(const ChapterRule* rule) = 0;
    virtual void           setBookInfo(BookInfo* bookInfo) = 0;
    virtual BookErrno      open() = 0;
    virtual void           close() = 0;
    virtual bool           isAbort() const = 0;
    virtual int            index() const = 0;
    virtual bool           setIndex(int index) = 0;
    virtual int            curChapterIndex() const = 0;
    virtual int            chapterCount() const = 0;
    virtual const Chapter* chapter(int chapterIndex) const = 0;
    virtual const Cover*   cover() const = 0;
    virtual bool           isEditSupport() const = 0;
    virtual bool           editText(int start, int srcLen, wchar_t* const dstText, int dstLen) = 0;
    virtual bool           isFetchSupport() const = 0;
    virtual bool           fetchChapter(int chapterIndex, AsyncCallback cb = nullptr) = 0;
    virtual bool           isRefreshSupport() const = 0;
    virtual bool           refreshChapter(AsyncCallback cb) = 0;
    virtual bool           refreshContent(int chapterIndex, AsyncCallback cb) = 0;
    virtual LRESULT        onEvent(WPARAM wParam, LPARAM lParam) = 0;
    virtual LRESULT        onTimer(WPARAM wParam, LPARAM lParam) = 0;
};