/**
 * @file BookWeb.h
 * @brief Provides a LAN Web server to sync and read books with the Legado mobile app.
 *
 *   GET  {server}/getBookshelf                            -> Bookshelf (includes durChapterIndex/durChapterPos reading progress)
 *   GET  {server}/getChapterList?url={bookUrl}            -> Chapter list
 *   GET  {server}/getBookContent?url={bookUrl}&index={n}  -> Chapter content (UTF-8 plain text)
 *   POST {server}/saveBookProgress                        -> Save reading progress (updates bookshelf progress in mobile Legado app)
 * 
 * Original Author: NanoKimi (https://github.com/NanoKimi)
 * Current  Author: binbyu   (https://github.com/binbyu)
 */

#pragma once
#ifdef ENABLE_NETWORK
#include "BookOnline.h"
#include "WebFile.h"

class BookWeb : public BookOnline
{
public:
    BookWeb(const std::wstring& fileName);
    virtual           ~BookWeb();
    virtual BookType  type() const;
    virtual bool      setIndex(int index);
    virtual LRESULT   onEvent(WPARAM wParam, LPARAM lParam);
    virtual LRESULT   onTimer(WPARAM wParam, LPARAM lParam);
    virtual void      checkUpdate(AsyncCallback cb);

protected:
    virtual BookErrno parseBook();
    virtual BookErrno saveFile();
    virtual BookErrno parseFile(const char* buffer, size_t size);
    virtual bool      parseChapters(int chapterIdx, AsyncCallback cb = nullptr);
    virtual bool      parseContent(int chapterIdx, AsyncCallback cb = nullptr);
    virtual bool      setProgress(bool force = false);
    virtual bool      getProgress(AsyncCallback cb = nullptr);
    void              syncProgress();
    std::string       buildUrl(ReqType type, int chapterIdx = -1);

protected:
    WebShelfItem* m_meta;
    bool          m_fileUnsave;
    bool          m_progUnsave;
};
#endif