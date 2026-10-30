#ifdef ENABLE_NETWORK
#include "BookOnline.h"
#include "Utils.h"
#include "conv.h"
#include "HtmlParser.h"
#include "Utils.h"
#include <regex>

#define BOOK_IDLEFETCH_TIMEOUT      60000   // 1 min
#define BOOK_IDLEFETCH_RETRY_COUNT  3

BookOnline::BookOnline(const std::wstring& fileName)
    : BookBase(fileName)
    , m_hWnd(nullptr)
    , m_file(nullptr)
    , m_bs(nullptr)
    , m_isCheckUpdate(false)
    , m_fetchFailCount(0)
{
}

BookOnline::~BookOnline()
{
    KillTimer(m_hWnd, IDT_BOOKIDLEFETCH);
    m_reqs.clear();
    if (m_file)
    {
        delete m_file;
        m_file = nullptr;
    }
    if (m_bs)
    {
        delete m_bs;
        m_bs = nullptr;
    }
}

BookType BookOnline::type() const
{
    return BookType::ONLINE;
}

void BookOnline::close()
{
    m_reqs.clear();
    BookBase::close();
    m_cv.notify_one();
}

bool BookOnline::isFetchSupport() const
{
    return true;
}

bool BookOnline::fetchChapter(int chapterIndex, AsyncCallback cb)
{
    if (chapterIndex < 0 || chapterIndex >= (int)m_chapters.size()) return false;
    return parseContent(chapterIndex, cb);
}

bool BookOnline::isRefreshSupport() const
{
    return true;
}

bool BookOnline::refreshChapter(AsyncCallback cb)
{
    return parseChapters(-1, cb);
}

bool BookOnline::refreshContent(int chapterIndex, AsyncCallback cb)
{
    if (chapterIndex < 0 || chapterIndex >= (int)m_chapters.size()) return false;
    return parseContent(chapterIndex, cb);
}

LRESULT BookOnline::onEvent(WPARAM wParam, LPARAM lParam)
{
    EventType type = (EventType)wParam;
    int isUpdated = 0;
    int hasNew = 0;

    switch (type)
    {
    case UPDATE_CHAPTER:
    {
        EventDataChapter* ed = (EventDataChapter*)lParam;
        if (m_chapters.empty())
        {
            m_chapters = std::move(ed->chapters);
            isUpdated = 1;
        }
        else
        {
            if (m_chapters.size() <= ed->chapters.size())
            {
                for (size_t i=0; i<m_chapters.size(); i++)
                {
                    auto& c1 = m_chapters[i];
                    auto& c2 = ed->chapters[i];
                    if (c1.url != c2.url || c1.title != c2.title)
                    {
                        c1.url = c2.url; // c1.index != -1, still need to update url, support update the content by manually calling refreshContent().
                        c1.title = c2.title;
                        if (!isUpdated) isUpdated = c1.title != c2.title ? 1 : 0;
                    }
                }
                for (size_t i=m_chapters.size(); i<ed->chapters.size(); i++)
                {
                    m_chapters.push_back(ed->chapters[i]);
                    isUpdated = 1;
                    hasNew = 1;
                }
            }
        }

        m_updateTime = time(NULL);
        saveFile();

        if (isUpdated && isCurnReadingBook()) PostMessageW(m_hWnd, WM_UPDATECHAPTERS, 0, NULL);
        if (hasNew && m_bookInfo && !isCurnReadingBook())
        {
            m_bookInfo->hasNew = true;
            PostMessageW(m_hWnd, WM_UPDATEBOOKS, 0, NULL);
        }

        lgdebug(" -- UPDATE_CHAPTER. size=%d", (int)ed->chapters.size());
        break;
    }
    case UPDATE_CONTENT:
    {
        // fixed bug, Due to the delay of WM_PAINT processing.
        // if the WM_BOOKEVENT message is received before the Invalidate done refresh, the page operation will be lost
        UpdateWindow(m_hWnd);

        int needSave = 0;
        EventDataContent* ed = (EventDataContent*)lParam;
        if (ed->chapterIdx >= 0 && ed->chapterIdx < (int)m_chapters.size())
        {
            auto& c = m_chapters[ed->chapterIdx];

            if (c.index == -1) // insert new content
            {
                int newLength = m_length + ed->length;
                wchar_t* newText = (wchar_t*)realloc(m_text, (newLength + 1) * sizeof(wchar_t));
                if (!newText)
                {
                    lgerror("Failed to reallocate memory for online book text.");
                    break;
                }
                m_text = newText;
                m_length = newLength;
                m_text[m_length] = 0;

                int offset = -1;

                // move chapter index
                for (size_t i=ed->chapterIdx + 1; i<m_chapters.size(); i++)
                {
                    if (m_chapters[i].index != -1)
                    {
                        if (offset == -1) offset = m_chapters[i].index;
                        m_chapters[i].index += ed->length;
                    }
                }

                isUpdated = 1;

                if (offset == -1) // append
                {
                    c.index = m_length - ed->length;
                    c.textLen = ed->length;
                    memcpy(m_text + c.index, ed->text, ed->length * sizeof(wchar_t));
                }
                else // insert
                {
                    c.index = offset;
                    c.textLen = ed->length;
                    memmove(m_text + offset + ed->length, m_text + offset, (m_length - offset - ed->length) * sizeof(wchar_t));
                    memcpy(m_text + offset, ed->text, ed->length * sizeof(wchar_t));

                    // update book marks
                    if (m_bookInfo)
                    {
                        for (auto& i : m_bookInfo->marks)
                        {
                            if (i < offset) continue;
                            i += ed->length;
                            needSave = 1;
                        }
                    }
                }

                // update index
                if (m_bookInfo)
                {
                    if (m_bookInfo->index >= c.index)
                    {
                        m_bookInfo->index += ed->length;
                        needSave = 1;
                    }
                }
            }
            else // replace content
            {
                int oldTextLen = c.textLen;
                int delta = ed->length - oldTextLen;
                int suffixLen = m_length - (c.index + oldTextLen);

                if (delta == 0)
                {
                    if (0 != memcmp(m_text + c.index, ed->text, ed->length * sizeof(wchar_t)))
                    {
                        memcpy(m_text + c.index, ed->text, ed->length * sizeof(wchar_t));
                        isUpdated = 1;
                    }
                }
                else
                {
                    if (delta > 0)
                    {
                        int newLength = m_length + delta;
                        wchar_t* newText = (wchar_t*)realloc(m_text, (newLength + 1) * sizeof(wchar_t));
                        if (!newText)
                        {
                            lgerror("Failed to reallocate memory for online book text.");
                            break;
                        }
                        m_text = newText;
                        m_length = newLength;
                        m_text[m_length] = 0;

                        if (suffixLen > 0)
                        {
                            memmove(m_text + c.index + ed->length, m_text + c.index + oldTextLen, suffixLen * sizeof(wchar_t));
                        }
                        memcpy(m_text + c.index, ed->text, ed->length * sizeof(wchar_t));
                    }
                    else // delta < 0
                    {
                        if (suffixLen > 0)
                        {
                            memmove(m_text + c.index + ed->length, m_text + c.index + oldTextLen, suffixLen * sizeof(wchar_t));
                        }
                        memcpy(m_text + c.index, ed->text, ed->length * sizeof(wchar_t));

                        int newLength = m_length + delta;
                        wchar_t* newText = (wchar_t*)realloc(m_text, (newLength + 1) * sizeof(wchar_t));
                        if (!newText)
                        {
                            lgerror("Failed to reallocate memory for online book text.");
                            break;
                        }
                        m_text = newText;
                        m_length = newLength;
                        m_text[m_length] = 0;
                    }

                    // move chapter index
                    for (size_t i = ed->chapterIdx + 1; i < m_chapters.size(); i++)
                    {
                        if (m_chapters[i].index != -1)
                        {
                            m_chapters[i].index += delta;
                        }
                    }

                    c.textLen = ed->length;
                    isUpdated = 1;

                    // update book marks
                    if (m_bookInfo)
                    {
                        for (auto& i : m_bookInfo->marks)
                        {
                            if (i < c.index) continue;
                            if (i >= c.index + oldTextLen)
                            {
                                i += delta;
                                needSave = 1;
                            }
                            else if (i > c.index + ed->length)
                            {
                                i = c.index + ed->length;
                                needSave = 1;
                            }
                        }
                    }

                    // update index
                    if (m_bookInfo)
                    {
                        if (m_bookInfo->index >= c.index + oldTextLen)
                        {
                            m_bookInfo->index += delta;
                            needSave = 1;
                        }
                        else if (m_bookInfo->index > c.index + ed->length)
                        {
                            m_bookInfo->index = c.index + ed->length;
                            needSave = 1;
                        }
                    }
                }
            }

            if (needSave) Settings::instance().save();
            if (isUpdated) saveFile();

            lgdebug(" -- UPDATE_CONTENT. chapterIdx=%d, length=%d", ed->chapterIdx, ed->length);
        }
        break;
    }
    default:
        break;
    }
    return isUpdated;
}

LRESULT BookOnline::onTimer(WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
    case IDT_BOOKIDLEFETCH:
        if (m_fetchFailCount >= BOOK_IDLEFETCH_RETRY_COUNT) KillTimer(m_hWnd, IDT_BOOKIDLEFETCH);
        if (m_reqs.empty())
        {
            for (size_t i=0; i<m_chapters.size(); i++)
            {
                if (m_chapters[i].index == -1)
                {
                    fetchChapter((int)i);
                    break;
                }
            }
        }
        break;
    }
    return 0;
}

void BookOnline::setHWND(HWND hWnd)
{
    m_hWnd = hWnd;
}

void BookOnline::checkUpdate(AsyncCallback cb)
{
    BookErrno err = loadFile();
    if (BookErrno::SUCC != err)
    {
        if (cb) cb(err);
        return;
    }

    uint64_t curnTime = time(NULL);
    if (curnTime <= m_updateTime || curnTime - m_updateTime < 4 * 3600)
    {
        if (cb) cb(BookErrno::SUCC);
        return;
    }

    m_isCheckUpdate = true;
    refreshChapter(cb);
}

BookErrno BookOnline::parseBook()
{
    BookErrno err = loadFile();
    if (BookErrno::SUCC != err) return err;

    bool ready = false;

    auto cb = [&err, &ready, this] (BookErrno err_) {
        ready = true;
        err = err_;
        m_cv.notify_one();
    };

    if (m_chapters.empty()) parseChapters(0, cb);
    else if (m_length <= 0) parseContent(0, cb);
    else                    goto SUCC;

    {
        // wait for fetch complete
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this, &ready] { return m_abort || ready; });
        if (BookErrno::SUCC != err || m_abort || m_chapters.empty() || m_length == 0)
        {
            m_reqs.clear();
            if (err == BookErrno::SUCC) err = BookErrno::FAIL;
            return err;
        }
    }

SUCC:
    // start time for idle fetch
    KillTimer(m_hWnd, IDT_BOOKIDLEFETCH);
    SetTimer(m_hWnd, IDT_BOOKIDLEFETCH, BOOK_IDLEFETCH_TIMEOUT, NULL);
    return BookErrno::SUCC;
}

BookErrno BookOnline::loadFile()
{
    return readFile();
}

BookErrno BookOnline::saveFile()
{
    OLFile* oldFile = dynamic_cast<OLFile*>(m_file);
    if (!oldFile) return BookErrno::FAIL;

    OLFile* newFile = new OLFile(m_bookName, m_mainPage, oldFile->host(), m_chapters, oldFile->header()->notBooksrc, oldFile->header()->isFinished);
    if (!newFile) return BookErrno::TOO_LARGE;

    delete m_file;
    m_file = newFile;

    return writeFile();
}

BookErrno BookOnline::parseFile(const char* buffer, size_t size)
{
    OLFile* file = new OLFile(buffer, size, true);
    if (!file) return BookErrno::TOO_LARGE;

    if (!file->isValid())
    {
        delete file;
        lgerror("Book invalid file.");
        return BookErrno::PARSE_FAIL;
    }

    auto& sources = Settings::instance().data().sources;
    auto it = std::find_if(sources.begin(), sources.end(), [file](const BookSource& bs) { return bs.host == std::string(file->host()); });
    if (it != sources.end()) m_bs = new BookSource(*it);
    if (!m_bs && !file->header()->notBooksrc)
    {
        delete file;
        lgerror("Book source not found.");
        return BookErrno::BOOKSOURCE_NOT_FOUND;
    }

    m_bookName   = file->bookName();
    m_chapters   = file->chapters();
    m_mainPage   = file->mainPage();
    m_updateTime = file->header()->updateTime;
    m_file       = file;
    return BookErrno::SUCC;
}

bool BookOnline::parseChapters(int chapterIdx, AsyncCallback cb)
{
    if (!m_bs)
    {
        if (cb) cb(BookErrno::BOOKSOURCE_NOT_FOUND);
        return false;
    }

    if (m_chapterPage.empty())
    {
        if (m_bs->enableChapterPage) return parseMainPage(chapterIdx, cb);
        else m_chapterPage = m_mainPage;
    }

    // check if is already exist
    ReqData* rd = m_reqs.find(ReqType::CHAPTERS);
    if (rd)
    {
        if (cb) rd->cb = cb;
        rd->chapterIdx = chapterIdx;
        return true;
    }

    rd = new ReqData;
    rd->book = this;
    rd->type = ReqType::CHAPTERS;
    rd->cb = cb;
    rd->chapterIdx = chapterIdx;

    request_t r;
    memset(&r, 0, sizeof(request_t));
    r.method = GET;
    r.url = (char*)m_chapterPage.c_str();
    r.param1 = rd;
    r.completer = [](request_result_t *result) -> unsigned int
    {
        auto* rd = (ReqData*)result->param1;
        char* html = nullptr;
        int htmlLen = 0;
        ParserContext ctx = {0};
        std::vector<std::string> chapterTitles;
        std::vector<std::string> chapterUrls;
        std::vector<std::string> chapterNextUrls;
        std::vector<std::string> chapterNextKeywords;
        char combinedUrl[1024] = {0};
        EventDataChapter ed;
        wchar_t* title = nullptr;
        int titleLen = 0;
        BookErrno err = BookErrno::SUCC;
        AsyncCallback done = nullptr;
        BookOnline* this_ = nullptr;

        if (!rd) goto FAIL;
        this_ = dynamic_cast<BookOnline*>(rd->book);
        if (!this_) goto FAIL;
        if (result->errno_ == ECAN) goto FREE;
        if (result->errno_ != SUCC) { err = BookErrno::REQUEST_FAIL; goto FAIL; }
        if (result->status_code != 200) { err = BookErrno::HTTP_STATUS_ERROR; goto FAIL; }

        htmlLen = conv_to_utf8(result->body, result->bodylen, &html);
        if (!html) { err = BookErrno::TOO_LARGE; goto FAIL; }

        if (!HtmlParser::instance().parseHtmlBegin(html, htmlLen, ctx)) { err = BookErrno::XPATH_FAIL; goto FAIL; }
        HtmlParser::instance().parseHtmlNext(ctx, this_->m_bs->chapterTitleXpath, chapterTitles);
        HtmlParser::instance().parseHtmlNext(ctx, this_->m_bs->chapterUrlXpath, chapterUrls);
        if (this_->m_bs->enableChapterNext)
        {
            HtmlParser::instance().parseHtmlNext(ctx, this_->m_bs->chapterNextUrlXpath, chapterNextUrls);
            HtmlParser::instance().parseHtmlNext(ctx, this_->m_bs->chapterNextKeywordXpath, chapterNextKeywords);
        }
        HtmlParser::instance().parseHtmlEnd(ctx);

        if (chapterTitles.empty() || chapterTitles.size() != chapterUrls.size()) { err = BookErrno::XPATH_FAIL; goto FAIL; }
        for (auto& i : chapterUrls)
        {
            https_combine_url(result->req->url, i.c_str(), combinedUrl);
            i = combinedUrl;
        }
        rd->chapterTitles.insert(rd->chapterTitles.end(), chapterTitles.begin(), chapterTitles.end());
        rd->chapterUrls.insert(rd->chapterUrls.end(), chapterUrls.begin(), chapterUrls.end());

        if (this_->m_bs->enableChapterNext)
        {
            if (!chapterNextUrls.empty() && !chapterNextKeywords.empty() && chapterNextUrls.size() == chapterNextKeywords.size())
            {
                for (size_t i = 0; i < chapterNextUrls.size(); i++)
                {
                    if (std::string::npos != chapterNextKeywords[i].find(this_->m_bs->chapterNextKeyword))
                    {
                        if (Utils::isValidHref(chapterNextUrls[i]))
                        {
                            https_combine_url(result->req->url, chapterNextUrls[i].c_str(), combinedUrl);

                            // request next page
                            request_t r;
                            memset(&r, 0, sizeof(request_t));
                            r.method = GET;
                            r.url = combinedUrl;
                            r.param1 = rd;
                            r.completer = result->req->completer;
                            this_->m_reqs.remove_and_add(result->handle, r);
                            rd = nullptr; // not free
                            goto FREE;
                        }
                    }
                }
            }
        }

        // build chapters
        for (size_t i = 0; i < rd->chapterUrls.size(); i++)
        {
            const auto& utf8 = rd->chapterTitles[i];
            titleLen = conv_utf8_to_utf16(utf8.c_str(), (int)utf8.size(), &title);
            this_->formatTitle(title, &titleLen);
            if (title)
            {
                Chapter item;
                item.index = -1;
                item.title = title;
                item.titleLineLen = titleLen;
                item.url = rd->chapterUrls[i];
                item.textLen = 0;
                ed.chapters.push_back(item);
                conv_free(title);
            }
        }

        // update chapters
        if (!ed.chapters.empty())
        {
            SendMessageW(this_->m_hWnd, WM_BOOKEVENT, UPDATE_CHAPTER, (LPARAM)&ed);
        }

        // parse content
        if (rd->chapterIdx == -1) goto SUCC;
        this_->parseContent(rd->chapterIdx, rd->cb);
        goto FREE;

FAIL:
        if (html && htmlLen > 0) Utils::dumpHtmlParseErrorFile(html, htmlLen);
SUCC:
        if (rd) done = rd->cb;
FREE:
        if (this_) this_->m_reqs.remove(result->handle);
        if (rd) delete rd;
        if (html) conv_free(html);
        if (done) done(err);
        return 0;
    };

    m_reqs.add(r);
    lgdebug("Online book fetch chapters. chapterIdx=%d, cb=%s", chapterIdx, cb ? "valid" : "nullptr");
    return true;
}

bool BookOnline::parseContent(int chapterIdx, AsyncCallback cb)
{
    if (chapterIdx < 0 || chapterIdx >= (int)m_chapters.size())
    {
        if (cb) cb(BookErrno::FAIL);
        return false;
    }

    // check if is already exist
    ReqData* rd = m_reqs.find(ReqType::CONTENT, chapterIdx);
    if (rd)
    {
        if (cb) rd->cb = cb;
        rd->chapterIdx = chapterIdx;
        return true;
    }

    rd = new ReqData;
    rd->book = this;
    rd->type = ReqType::CONTENT;
    rd->cb = cb;
    rd->chapterIdx = chapterIdx;

    request_t r;
    memset(&r, 0, sizeof(request_t));
    r.method = GET;
    r.url = (char*)m_chapters[chapterIdx].url.c_str();
    r.param1 = rd;
    r.completer = [](request_result_t *result) -> unsigned int
    {
        auto* rd = (ReqData*)result->param1;
        char* html = nullptr;
        int htmlLen = 0;
        ParserContext ctx = {0};
        std::vector<std::string> contents;
        std::vector<std::string> contentNextUrls;
        std::vector<std::string> contentNextKeywords;
        char combinedUrl[1024] = {0};
        EventDataContent ed = { 0 };
        BookErrno err = BookErrno::SUCC;
        AsyncCallback done = nullptr;
        BookOnline* this_ = nullptr;

        if (!rd) goto FAIL;
        this_ = dynamic_cast<BookOnline*>(rd->book);
        if (!this_) goto FAIL;
        if (result->errno_ == ECAN) goto FREE;
        if (result->errno_ != SUCC) { err = BookErrno::REQUEST_FAIL; goto FAIL; }
        if (result->status_code != 200) { err = BookErrno::HTTP_STATUS_ERROR; goto FAIL; }

        htmlLen = conv_to_utf8(result->body, result->bodylen, &html);
        if (!html) { err = BookErrno::TOO_LARGE; goto FAIL; }

        HtmlParser::instance().tidyHtml(html, htmlLen);
        if (!HtmlParser::instance().parseHtmlBegin(html, htmlLen, ctx)) { err = BookErrno::XPATH_FAIL; goto FAIL; }
        if (this_->m_bs) HtmlParser::instance().parseHtmlNext(ctx, this_->m_bs->contentXpath, contents);
        else HtmlParser::instance().parseHtmlNext(ctx, DEFAULT_CONTENT_XPATH, contents);
        if (this_->m_bs && this_->m_bs->enableContentNext)
        {
            HtmlParser::instance().parseHtmlNext(ctx, this_->m_bs->contentNextUrlXpath, contentNextUrls);
            HtmlParser::instance().parseHtmlNext(ctx, this_->m_bs->contentNextKeywordXpath, contentNextKeywords);
        }
        HtmlParser::instance().parseHtmlEnd(ctx);

        if (contents.empty() || contents.front().empty()) { err = BookErrno::XPATH_FAIL; goto FAIL; }
        rd->contents.insert(rd->contents.end(), contents.begin(), contents.end());

        if (this_->m_bs && this_->m_bs->enableContentNext)
        {
            if (!contentNextUrls.empty() && !contentNextKeywords.empty() && contentNextUrls.size() == contentNextKeywords.size())
            {
                for (size_t i = 0; i < contentNextUrls.size(); i++)
                {
                    if (std::string::npos != contentNextKeywords[i].find(this_->m_bs->contentNextKeyword))
                    {
                        if (Utils::isValidHref(contentNextUrls[i]))
                        {
                            https_combine_url(result->req->url, contentNextUrls[i].c_str(), combinedUrl);

                            // request next page
                            request_t r;
                            memset(&r, 0, sizeof(request_t));
                            r.method = GET;
                            r.url = combinedUrl;
                            r.param1 = rd;
                            r.completer = result->req->completer;
                            this_->m_reqs.remove_and_add(result->handle, r);
                            rd = nullptr; // not free
                            goto FREE;
                        }
                    }
                }
            }
        }

        // build contents
        ed.chapterIdx = rd->chapterIdx;
        this_->buildContent(rd->contents, rd->chapterIdx, ed.text, ed.length);
        if (ed.text && ed.length > 0)
        {
            SendMessageW(this_->m_hWnd, WM_BOOKEVENT, UPDATE_CONTENT, (LPARAM)&ed);
        }
        free(ed.text);
        this_->m_fetchFailCount = 0;

        goto SUCC;

FAIL:
        if (this_) this_->m_fetchFailCount++;
        if (html && htmlLen > 0) Utils::dumpHtmlParseErrorFile(html, htmlLen);
SUCC:
        if (rd) done = rd->cb;
FREE:
        if (this_) this_->m_reqs.remove(result->handle);
        if (rd) delete rd;
        if (html) conv_free(html);
        if (done) done(err);
        return 0;
    };

    m_reqs.add(r);
    lgdebug("Online book fetch content. chapterIdx=%d, cb=%s", chapterIdx, cb ? "valid" : "nullptr");
    return true;
}

bool BookOnline::parseMainPage(int chapterIdx, AsyncCallback cb)
{
    if (!m_bs || !m_bs->enableChapterPage)
    {
        if (cb) cb(BookErrno::BOOKSOURCE_NOT_FOUND);
        return false;
    }

    m_chapterPage.clear();

    // check if is already exist
    ReqData* rd = m_reqs.find(ReqType::CHAPTERS);
    if (rd)
    {
        if (cb) rd->cb = cb;
        rd->chapterIdx = chapterIdx;
        return true;
    }

    rd = new ReqData;
    rd->book = this;
    rd->type = ReqType::CHAPTERS;
    rd->cb = cb;
    rd->chapterIdx = chapterIdx;

    request_t r;
    memset(&r, 0, sizeof(request_t));
    r.method = GET;
    r.url = (char*)m_mainPage.c_str();
    r.param1 = rd;
    r.completer = [](request_result_t *result) -> unsigned int
    {
        auto* rd = (ReqData*)result->param1;
        char* html = nullptr;
        int htmlLen = 0;
        std::vector<std::string> value;
        char combinedUrl[1024] = {0};
        BookErrno err = BookErrno::SUCC;
        AsyncCallback done = nullptr;
        BookOnline* this_ = nullptr;

        if (!rd) goto FAIL;
        this_ = dynamic_cast<BookOnline*>(rd->book);
        if (!this_) goto FAIL;
        if (result->errno_ == ECAN) goto FREE;
        if (result->errno_ != SUCC) { err = BookErrno::REQUEST_FAIL; goto FAIL; }
        if (result->status_code != 200) { err = BookErrno::HTTP_STATUS_ERROR; goto FAIL; }

        htmlLen = conv_to_utf8(result->body, result->bodylen, &html);
        if (!html) { err = BookErrno::TOO_LARGE; goto FAIL; }

        HtmlParser::instance().parseHtml(html, htmlLen, this_->m_bs->chapterPageXpath, value);
        if (value.empty()) { err = BookErrno::XPATH_FAIL; goto FAIL; }

        if (!Utils::isValidHref(value.front())) { err = BookErrno::XPATH_FAIL; goto FAIL; }

        https_combine_url(result->req->url, value.front().c_str(), combinedUrl);
        this_->m_chapterPage = combinedUrl;
        this_->m_reqs.remove(result->handle);
        this_->parseChapters(rd->chapterIdx, rd->cb);
        goto FREE;

FAIL:
        if (html && htmlLen > 0) Utils::dumpHtmlParseErrorFile(html, htmlLen);
        if (rd) done = rd->cb;
FREE:
        if (this_) this_->m_reqs.remove(result->handle);
        if (rd) delete rd;
        if (html) conv_free(html);
        if (done) done(err);
        return 0;
    };

    m_reqs.add(r);
    lgdebug("Online book fetch mainpage. chapterIdx=%d, cb=%s", chapterIdx, cb ? "valid" : "nullptr");
    return true;
}

bool BookOnline::buildContent(const std::vector<std::string>& contents, int chapterIdx, wchar_t*& text, int& length)
{
    if (chapterIdx < 0 || chapterIdx >= (int)m_chapters.size()) return false;
    if (contents.empty()) return false;

    struct Buffer
    {
        wchar_t *text;
        int len;
    };
    Buffer buffer;
    std::vector<Buffer> buffers;

    for (const auto& i : contents)
    {
        buffer.text = nullptr;
        buffer.len = conv_utf8_to_utf16(i.c_str(), (int)i.size(), &buffer.text);
        if (buffer.text)
        {
            filterContent(buffer.text, buffer.len);
            if (buffer.len > 0)
            {
                buffers.push_back(buffer);
                length += buffer.len + 1/* \n */;
                buffer.text = nullptr;
            }
            if (buffer.text) conv_free(buffer.text);
        }
    }
    if (length <= 0) return false;

    const auto& chapter = m_chapters[chapterIdx];
    length += (int)chapter.title.size() + 1/* \n */;

    int offset = 0;
    text = (wchar_t*)malloc(sizeof(wchar_t) * (length + 1));
    text[length] = 0;
    memcpy(text + offset, chapter.title.c_str(), sizeof(wchar_t) * chapter.title.size());
    offset += (int)chapter.title.size();

    for (auto& i : buffers)
    {
        if (offset > 0) text[offset++] = L'\n';
        memcpy(text + offset, i.text, sizeof(wchar_t) * i.len);
        offset += i.len;
        conv_free(i.text);
    }
    if (buffers.size() > 0) text[offset++] = L'\n'; // add '\n' for chapter text end
    ASSERT(offset == length);
    return true;
}

bool BookOnline::filterContent(wchar_t* text, int& length)
{
    if (m_bs)
    {
        switch (m_bs->contentFilterType)
        {
        case 0: // disable
            break;
        case 1: // keyword
            {
                wchar_t* buffer = (wchar_t*)malloc(sizeof(wchar_t) * (length + 1));
                memset(buffer, 0, sizeof(wchar_t) * (length + 1));
                int bufferLen = 0;
                int kwlen = (int)m_bs->contentFilterKeyword.size();
                if (kwlen <= 0) break;
                for (int i=0; i < length; i++)
                {
                    if (length - i >= kwlen && 0 == wcsncmp(text + i, m_bs->contentFilterKeyword.c_str(), kwlen))
                    {
                        i += kwlen-1; // for loop will i++, so kwlen-1
                        continue;
                    }
                    buffer[bufferLen++] = text[i];
                }
                if (bufferLen != length)
                {
                    memcpy(text, buffer, sizeof(wchar_t) * (bufferLen + 1));
                    length = bufferLen;
                }
                free(buffer);
            }
            break;
        case 2: // regex
            {
                try
                {
                    std::wregex re(m_bs->contentFilterKeyword, std::regex::optimize);
                    std::wstring result = std::regex_replace(std::wstring(text, length), re, L"");

                    if ((int)result.size() != length)
                    {
                        length = (int)result.size();
                        memcpy(text, result.c_str(), sizeof(wchar_t) * length);
                        text[length] = 0;
                    }
                }
                catch (...)
                {
                }
            }
            break;
        }
    }

    if (length > 0)
    {
        formatText(text, &length);
    }
    return true;
}

bool BookOnline::isCurnReadingBook()
{
    return !m_isCheckUpdate;
}

BookErrno BookOnline::readFile()
{
    FILE* fp = _wfopen(m_fileName.c_str(), L"rb");
    if (!fp)
    {
        lgerror("Failed to open book file. Error code: %lu", GetLastError());
        return BookErrno::OPEN_W_FAIL;
    }
    fseek(fp, 0, SEEK_END);
    int size = (int)ftell(fp);
    fseek(fp, 0, SEEK_SET);

    int baseSize = sizeof(HeaderBase);
    if (size < baseSize)
    {
        fclose(fp);
        lgerror("Invalid book file. size=%d, baseSize=%d", size, baseSize);
        return BookErrno::UNKNOWN_FORMAT;
    }

    HeaderBase hdr = {0};
    if (baseSize != fread(&hdr, 1, baseSize, fp))
    {
        fclose(fp);
        lgerror("Failed to read book file header base. Error code: %lu", GetLastError());
        return BookErrno::READ_FAIL;
    }

    if (size < (int)hdr.headerSize || (int)hdr.headerSize < baseSize)
    {
        fclose(fp);
        lgerror("Invalid book file. size=%d, headerSize=%d, baseSize", size, hdr.headerSize, baseSize);
        return BookErrno::UNKNOWN_FORMAT;
    }

    if ((size - hdr.headerSize) % 2 != 0) // text is encode with UTF16-LE
    {
        fclose(fp);
        lgerror("Invalid book file. size=%d, headerSize=%d", size, hdr.headerSize);
        return BookErrno::UNKNOWN_FORMAT;
    }

    char* buffer = (char*)malloc(hdr.headerSize);
    if (!buffer)
    {
        fclose(fp);
        lgerror("Failed to allocate memory for book file. size=%d", hdr.headerSize);
        return BookErrno::TOO_LARGE;
    }

    memcpy(buffer, &hdr, baseSize);
    int leftSize = hdr.headerSize - baseSize;
    if (leftSize > 0)
    {
        if (leftSize != fread(buffer + baseSize, 1, leftSize, fp))
        {
            fclose(fp);
            free(buffer);
            lgerror("Failed to read book file header. Error code: %lu", GetLastError());
            return BookErrno::READ_FAIL;
        }
    }

    auto res = parseFile(buffer, hdr.headerSize);
    if (res != BookErrno::SUCC)
    {
        fclose(fp);
        free(buffer);
        return res;
    }

    // read text
    m_length = (size - hdr.headerSize) / sizeof(wchar_t);
    m_text = (wchar_t*)malloc((m_length + 1) * sizeof(wchar_t));
    if (!m_text)
    {
        fclose(fp);
        delete m_file; m_file = nullptr;
        if (m_bs) { delete m_bs; m_bs = nullptr; }
        return BookErrno::TOO_LARGE;
    }
    m_text[m_length] = 0;
    if (m_length > 0)
    {
        if (m_length != fread(m_text, sizeof(wchar_t), m_length, fp))
        {
            fclose(fp);
            free(m_text); m_text = nullptr;
            delete m_file; m_file = nullptr;
            if (m_bs) { delete m_bs; m_bs = nullptr; }
            lgerror("Failed to read book file content. Error code: %lu", GetLastError());
            return BookErrno::READ_FAIL;
        }
    }
    fclose(fp);
    return BookErrno::SUCC;
}

BookErrno BookOnline::writeFile()
{
    if (!m_file) return BookErrno::FAIL;

    FILE* fp = _wfopen(m_fileName.c_str(), L"wb");
    if (!fp)
    {
        lgerror("Open file failed. Error code: %lu", GetLastError());
        return BookErrno::OPEN_W_FAIL;
    }
    if (m_file->size() != fwrite(m_file->buffer(), 1, m_file->size(), fp))
    {
        fclose(fp);
        lgerror("Write file header failed. Error code: %lu", GetLastError());
        return BookErrno::WRITE_FAIL;
    }
    if (m_length != fwrite(m_text, sizeof(wchar_t), m_length, fp))
    {
        fclose(fp);
        lgerror("Write file text failed. Error code: %lu", GetLastError());
        return BookErrno::WRITE_FAIL;
    }
    fclose(fp);
    return BookErrno::SUCC;
}
#endif