/**
 * @file BookWeb.cpp
 * @brief Provides a LAN Web server to sync and read books with the Legado mobile app.
 *
 * Original Author: NanoKimi (https://github.com/NanoKimi)
 * Current  Author: binbyu   (https://github.com/binbyu)
 */

#ifdef ENABLE_NETWORK
#include "BookWeb.h"
#include "conv.h"
#include "JsonSerialize.h"
#include "Utils.h"
#include <algorithm>

#define HTTP_REQUEST_TIMEOUT        30000   // 30s
#define BOOK_SYNCPROGRESS_TIMEOUT   5000    // 5s
#define BOOK_SAVEFILE_TIMEOUT       3000    // 3s

BookWeb::BookWeb(const std::wstring& fileName)
    : BookOnline(fileName)
    , m_meta(nullptr)
    , m_fileUnsave(false)
    , m_progUnsave(false)
{
}

BookWeb::~BookWeb()
{
    KillTimer(m_hWnd, IDT_BOOKSYNCPROGRESS);
    KillTimer(m_hWnd, IDT_BOOKSAVEFILE);
    if (m_fileUnsave) saveFile();
    m_reqs.wait();
    if (m_progUnsave) setProgress(true); // force save progress to server when file close
}

BookType BookWeb::type() const
{
    return BookType::WEB;
}

bool BookWeb::setIndex(int index)
{
    if (BookBase::setIndex(index) && m_meta)
    {
        int curIndex = curChapterIndex();
        if (curIndex >= 0 && curIndex < (int)m_chapters.size())
        {
            Chapter& c = m_chapters[curIndex];
            int pos = this->index() - c.index;
            if (curIndex != m_meta->durChapterIndex || pos != m_meta->durChapterPos)
            {
                m_meta->durChapterTitle = c.title;
                m_meta->durChapterIndex = curIndex;
                m_meta->durChapterPos = pos;
                m_meta->durChapterTime = Utils::nowMs();
                m_progUnsave = true;

                // start timer for save file
                // use a timer to prevent excessive save when flipping pages continuously
                m_fileUnsave = true;
                KillTimer(m_hWnd, IDT_BOOKSAVEFILE);
                SetTimer(m_hWnd, IDT_BOOKSAVEFILE, BOOK_SAVEFILE_TIMEOUT, NULL);
            }
        }
        return true;
    }
    return false;
}

LRESULT BookWeb::onEvent(WPARAM wParam, LPARAM lParam)
{
    EventType type = (EventType)wParam;

    if (UPDATE_PROGRESS == type)
    {
        // fixed bug, Due to the delay of WM_PAINT processing.
        // if the WM_BOOKEVENT message is received before the Invalidate done refresh, the page operation will be lost
        UpdateWindow(m_hWnd);

        EventDataProgress* ed = (EventDataProgress*)lParam;
        if (!ed || !ed->item) return 0;
        if (!m_meta) return 0;
        if (m_meta->bookUrl != ed->item->bookUrl) return 0;

        bool newerProgress = !m_progUnsave && ed->item->durChapterTime > m_meta->durChapterTime;
        bool needUpdateChapter = m_meta->totalChapterNum != ed->item->totalChapterNum;
        bool needUpdateIndex = newerProgress && (m_meta->durChapterIndex != ed->item->durChapterIndex || m_meta->durChapterPos != ed->item->durChapterPos);

        if (!needUpdateChapter && !needUpdateIndex) return 0;

        if (newerProgress)
        {
            *m_meta = *ed->item;
        }
        else
        {
            m_meta->totalChapterNum = ed->item->totalChapterNum;
            m_meta->latestChapterTitle = ed->item->latestChapterTitle;
        }

        m_fileUnsave = true;
        KillTimer(m_hWnd, IDT_BOOKSAVEFILE);
        SetTimer(m_hWnd, IDT_BOOKSAVEFILE, BOOK_SAVEFILE_TIMEOUT, NULL);

        if (needUpdateIndex)
        {
            if (m_meta->durChapterIndex >= 0 && m_meta->durChapterIndex < (int)m_chapters.size())
            {
                Chapter& c = m_chapters[m_meta->durChapterIndex];
                if (c.index == -1)
                {
                    // For simplicity of logic, use an asynchronous method here to fetch the chapter content first.
                    // The progress will be updated the next time sync progress with the server.
                    parseContent(m_meta->durChapterIndex);
                }
                else
                {
                    setIndex(c.index + m_meta->durChapterPos);

                    Settings::instance().save();
                    PostMessageW(m_hWnd, WM_REDRAW, 0, NULL);
                }
            }
        }
        if (needUpdateChapter) refreshChapter(nullptr); // async to fetch chapters
        return 1;
    }
    else
    {
        return BookOnline::onEvent(wParam, lParam);
    }
}

LRESULT BookWeb::onTimer(WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
    case IDT_BOOKSYNCPROGRESS:
        syncProgress();
        break;
    case IDT_BOOKSAVEFILE:
        KillTimer(m_hWnd, IDT_BOOKSAVEFILE);
        if (m_fileUnsave) saveFile();
        break;
    }
    return 0;
}

void BookWeb::checkUpdate(AsyncCallback cb)
{
    // unsupport
    return;
}

BookErrno BookWeb::parseBook()
{
    BookErrno err = loadFile();
    if (BookErrno::SUCC != err) return err;

    bool ready = false;
    int chapterIdx = m_meta->durChapterIndex;

    // step3: finish
    auto finish = [&err, &ready, &chapterIdx, this] (BookErrno err_) {
        ready = true;
        err = err_;
        if (err == BookErrno::SUCC)
        {
            // set index
            if (chapterIdx >= 0 && chapterIdx < (int)m_chapters.size())
            {
                Chapter& c = m_chapters[chapterIdx];
                if (c.index != -1) setIndex(c.index + m_meta->durChapterPos);
            }
        }
        m_cv.notify_one();
    };

    // step2: fetch chapters or content
    auto fetchChaptersOrContent = [this, finish](BookErrno err) {
        if (BookErrno::SUCC != err) { finish(err); return; }
        if (!m_meta)                { finish(BookErrno::PARSE_FAIL); return; }
        int chapterIdx = m_meta->durChapterIndex;
        if (chapterIdx < 0) chapterIdx = 0;

        if (m_meta->totalChapterNum != (int)m_chapters.size() || m_chapters.empty())
        {
            parseChapters(chapterIdx, finish);
            return;
        }

        if (chapterIdx >= (int)m_chapters.size()) chapterIdx = 0;
        if (chapterIdx < 0 || m_chapters.empty()) { finish(BookErrno::FAIL); return; }
        if (m_chapters[chapterIdx].index == -1)   parseContent(chapterIdx, finish);
        else                                      finish(BookErrno::SUCC);
    };

    // step1: sync progress from server
    if (m_meta->durChapterTime <= 0 || m_meta->durChapterTime + BOOK_SYNCPROGRESS_TIMEOUT < Utils::nowMs())
    {
        getProgress([fetchChaptersOrContent](BookErrno err) { fetchChaptersOrContent(err); });
    }
    else
    {
        fetchChaptersOrContent(err);
    }

    // wait for fetch complete
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this, &ready] { return m_abort || ready; });
    if (BookErrno::SUCC != err || m_abort || m_chapters.empty() || m_length == 0)
    {
        m_reqs.clear();
        if (err == BookErrno::SUCC) err = BookErrno::FAIL;
        return err;
    }

    // start timer for sync progress from server
    KillTimer(m_hWnd, IDT_BOOKSYNCPROGRESS);
    SetTimer(m_hWnd, IDT_BOOKSYNCPROGRESS, BOOK_SYNCPROGRESS_TIMEOUT, NULL);
    return BookErrno::SUCC;
}

BookErrno BookWeb::saveFile()
{
    WebFile* oldFile = dynamic_cast<WebFile*>(m_file);
    if (!oldFile || !oldFile->bookMeta()) return BookErrno::FAIL;

    WebFile* newFile = new WebFile(m_bookName, *oldFile->bookMeta(), m_chapters);
    if (!newFile) return BookErrno::TOO_LARGE;

    delete m_file;
    m_file = newFile;
    m_meta = newFile->bookMeta();
    m_fileUnsave = false;

    return writeFile();
}

BookErrno BookWeb::parseFile(const char* buffer, size_t size)
{
    WebFile* file = new WebFile(buffer, size, true);
    if (!file) return BookErrno::TOO_LARGE;

    if (!file->isValid())
    {
        delete file;
        lgerror("Book invalid file.");
        return BookErrno::PARSE_FAIL;
    }

    m_bookName = file->bookName();
    m_chapters = file->chapters();
    m_meta     = file->bookMeta();
    if (!m_meta)
    {
        delete file;
        lgerror("Book meta not found.");
        return BookErrno::PARSE_FAIL;
    }
    m_file     = file;
    return BookErrno::SUCC;
}

bool BookWeb::parseChapters(int chapterIdx, AsyncCallback cb)
{
    // check if is already exist
    ReqData *rd = m_reqs.find(ReqType::CHAPTERS);
    if (rd)
    {
        if (cb) rd->cb = cb;
        rd->chapterIdx = chapterIdx;
        return true;
    }

    // build url
    std::string url = buildUrl(ReqType::CHAPTERS);
    if (url.empty())
    {
        if (cb) cb(BookErrno::PARSE_FAIL);
        return false;
    }

    // do request
    rd = new ReqData;
    rd->book = this;
    rd->type = ReqType::CHAPTERS;
    rd->cb = cb;
    rd->chapterIdx = chapterIdx;

    request_t r;
    memset(&r, 0, sizeof(request_t));
    r.method = GET;
    r.url = (char *)url.c_str();
    r.param1 = rd;
    r.param2 = NULL;
    r.noproxy = 1;
    r.timeout = HTTP_REQUEST_TIMEOUT; // Legado's web server is unstable, so add timeout for http request.
    r.completer = [](request_result_t *result) -> unsigned int
    {
        auto *rd = (ReqData *)result->param1;
        char *html = nullptr;
        int htmlLen = 0;
        std::vector<uint8_t> bytes;
        WebChapterList list;
        EventDataChapter ed;
        wchar_t* title = nullptr;
        int titleLen = 0;
        BookErrno err = BookErrno::SUCC;
        AsyncCallback done = nullptr;
        BookWeb *this_ = nullptr;

        if (!rd) goto FAIL;
        this_ = dynamic_cast<BookWeb*>(rd->book);
        if (!this_) goto FAIL;
        if (result->errno_ == ECAN) goto FREE;
        if (result->errno_ != SUCC) { err = BookErrno::REQUEST_FAIL; goto FAIL; }
        if (result->status_code != 200) { err = BookErrno::HTTP_STATUS_ERROR; goto FAIL; }

        htmlLen = conv_to_utf8(result->body, result->bodylen, &html);
        if (!html) { err = BookErrno::TOO_LARGE; goto FAIL; }

        bytes.assign(html, html + htmlLen);
        if (!JsonSerialize::deserialize(bytes, list)) { err = BookErrno::JSON_FAIL; goto FAIL; }
        if (list.data.empty() || !list.isSuccess) { err = BookErrno::JSON_NODATA; goto FAIL; }
        std::sort(list.data.begin(), list.data.end(), [](const WebChapterItem& a, const WebChapterItem& b){ return a.index < b.index; });

        // build chapters
        for (auto& i : list.data)
        {
            const auto& utf8 = i.title;
            titleLen = conv_utf8_to_utf16(utf8.c_str(), (int)utf8.size(), &title);
            this_->formatTitle(title, &titleLen);
            if (title)
            {
                Chapter item;
                item.index = -1;
                item.title = title;
                item.titleLineLen = titleLen;
                item.url = i.url;
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
        if (html && htmlLen > 0) Utils::dumpJsonParseErrorFile(html, htmlLen);
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
    lgdebug("Web book fetch chapters. chapterIdx=%d, cb=%s", chapterIdx, cb ? "valid" : "nullptr");
    return true;
}

bool BookWeb::parseContent(int chapterIdx, AsyncCallback cb)
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

    // build url
    std::string url = buildUrl(ReqType::CONTENT, chapterIdx);
    if (url.empty())
    {
        if (cb) cb(BookErrno::PARSE_FAIL);
        return false;
    }

    rd = new ReqData;
    rd->book = this;
    rd->type = ReqType::CONTENT;
    rd->cb = cb;
    rd->chapterIdx = chapterIdx;

    request_t r;
    memset(&r, 0, sizeof(request_t));
    r.method = GET;
    r.url = (char*)url.c_str();
    r.param1 = rd;
    r.param2 = NULL;
    r.noproxy = 1;
    r.timeout = HTTP_REQUEST_TIMEOUT; // Legado's web server is unstable, so add timeout for http request.
    r.completer = [](request_result_t *result) -> unsigned int
    {
        auto* rd = (ReqData*)result->param1;
        char* html = nullptr;
        int htmlLen = 0;
        std::vector<uint8_t> bytes;
        WebContent data;
        EventDataContent ed = { 0 };
        BookErrno err = BookErrno::SUCC;
        AsyncCallback done = nullptr;
        BookWeb* this_ = nullptr;

        if (!rd) goto FAIL;
        this_ = dynamic_cast<BookWeb*>(rd->book);
        if (!this_) goto FAIL;
        if (result->errno_ == ECAN) goto FREE;
        if (result->errno_ != SUCC) { err = BookErrno::REQUEST_FAIL; goto FAIL; }
        if (result->status_code != 200) { err = BookErrno::HTTP_STATUS_ERROR; goto FAIL; }

        htmlLen = conv_to_utf8(result->body, result->bodylen, &html);
        if (!html) { err = BookErrno::TOO_LARGE; goto FAIL; }

        bytes.assign(html, html + htmlLen);
        if (!JsonSerialize::deserialize(bytes, data)) { err = BookErrno::JSON_FAIL; goto FAIL; }
        if (data.data.empty() || !data.isSuccess) { err = BookErrno::JSON_NODATA; goto FAIL; }

        // build contents
        ed.chapterIdx = rd->chapterIdx;
        this_->buildContent({ data.data }, rd->chapterIdx, ed.text, ed.length);
        if (ed.text && ed.length > 0)
        {
            SendMessageW(this_->m_hWnd, WM_BOOKEVENT, UPDATE_CONTENT, (LPARAM)&ed);
        }
        free(ed.text);

        goto SUCC;

FAIL:
        if (html && htmlLen > 0) Utils::dumpJsonParseErrorFile(html, htmlLen);
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
    lgdebug("Web book fetch content. chapterIdx=%d, cb=%s", chapterIdx, cb ? "valid" : "nullptr");
    return true;
}

bool BookWeb::setProgress(bool force)
{
    if (!m_meta) return false;

    ReqData* rd = nullptr;

    if (!force)
    {
        // check if is already exist
        rd = m_reqs.find(ReqType::SETPROGS);
        if (rd) return true;
    }

    // save book progress
    WebSaveBookProgress data;
    data.name            = m_meta->name;
    data.author          = m_meta->author;
    data.durChapterTitle = m_meta->durChapterTitle;
    data.durChapterIndex = m_meta->durChapterIndex;
    data.durChapterPos   = m_meta->durChapterPos;
    data.durChapterTime  = m_meta->durChapterTime;

    std::vector<uint8_t> json;
    JsonSerialize::enserialize(data, json);

    // build url
    std::string url = buildUrl(ReqType::SETPROGS);
    if (url.empty()) return false;

    if (!force)
    {
        rd = new ReqData;
        rd->book = this;
        rd->type = ReqType::SETPROGS;
        rd->cb = nullptr;
        rd->chapterIdx = -1;
    }

    request_t r;
    memset(&r, 0, sizeof(request_t));
    r.method = POST;
    r.url = (char*)url.c_str();
    r.content = (char*)json.data();
    r.content_length = (int)json.size();
    r.param1 = rd;
    r.param2 = NULL;
    r.noproxy = 1;
    r.timeout = HTTP_REQUEST_TIMEOUT; // Legado's web server is unstable, so add timeout for http request.
    r.extraheader = (char*)"Content-Type: application/json; charset=utf-8\r\n";
    r.completer = [](request_result_t *result) -> unsigned int
    {
        auto* rd = (ReqData*)result->param1;
        if (rd)
        {
            BookWeb* this_ = dynamic_cast<BookWeb*>(rd->book);
            if (this_) this_->m_reqs.remove(result->handle);
            delete rd;
        }
        return 0;
    };
    if (!force) m_reqs.add(r);
    else https_request(&r);
    return true;
}

bool BookWeb::getProgress(AsyncCallback cb)
{
    // check if is already exist
    ReqData *rd = m_reqs.find(ReqType::GETPROGS);
    if (rd)
    {
        if (cb) rd->cb = cb;
        return true;
    }

    // build url
    std::string url = buildUrl(ReqType::GETPROGS);
    if (url.empty())
    {
        if (cb) cb(BookErrno::PARSE_FAIL);
        return false;
    }

    // do request
    rd = new ReqData;
    rd->book = this;
    rd->type = ReqType::GETPROGS;
    rd->cb = cb;
    rd->chapterIdx = -1;

    request_t r;
    memset(&r, 0, sizeof(request_t));
    r.method = GET;
    r.url = (char *)url.c_str();
    r.param1 = rd;
    r.param2 = NULL;
    r.noproxy = 1;
    r.timeout = HTTP_REQUEST_TIMEOUT; // Legado's web server is unstable, so add timeout for http request.
    r.completer = [](request_result_t *result) -> unsigned int
    {
        auto *rd = (ReqData *)result->param1;
        char *html = nullptr;
        int htmlLen = 0;
        std::vector<uint8_t> bytes;
        WebShelfList list;
        WebShelfItem* item = nullptr;
        EventDataProgress ed;
        BookErrno err = BookErrno::SUCC;
        AsyncCallback done = nullptr;
        BookWeb *this_ = nullptr;

        if (!rd) goto FAIL;
        this_ = dynamic_cast<BookWeb*>(rd->book);
        if (!this_) goto FAIL;
        if (result->errno_ == ECAN) goto FREE;
        if (result->errno_ != SUCC) { err = BookErrno::REQUEST_FAIL; goto FAIL; }
        if (result->status_code != 200) { err = BookErrno::HTTP_STATUS_ERROR; goto FAIL; }

        htmlLen = conv_to_utf8(result->body, result->bodylen, &html);
        if (!html) { err = BookErrno::TOO_LARGE; goto FAIL; }

        bytes.assign(html, html + htmlLen);
        if (!JsonSerialize::deserialize(bytes, list)) { err = BookErrno::JSON_FAIL; goto FAIL; }
        if (list.data.empty() || !list.isSuccess) { err = BookErrno::JSON_NODATA; goto FAIL; }

        // find book progress
        for (auto& i : list.data)
        {
            if (i.bookUrl == this_->m_meta->bookUrl)
            {
                item = &i;
                break;
            }
        }

        // update progress
        ed.item = item;
        if (ed.item)
        {
            SendMessageW(this_->m_hWnd, WM_BOOKEVENT, UPDATE_PROGRESS, (LPARAM)&ed);
        }
        goto SUCC;

    FAIL:
        if (html && htmlLen > 0) Utils::dumpJsonParseErrorFile(html, htmlLen);
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
    lgdebug("Web book fetch progress. cb=%s", cb ? "valid" : "nullptr");
    return true;
}

void BookWeb::syncProgress()
{
    // if a progress is requesting, skip the current round.
    if (m_reqs.find(ReqType::GETPROGS)) return;
    if (m_reqs.find(ReqType::SETPROGS)) return;

    // if there is local progress pending upload, upload it
    if (m_progUnsave)
    {
        m_progUnsave = false;
        setProgress();
        return;
    }

    // sync progress from server
    getProgress();
}

std::string BookWeb::buildUrl(ReqType type, int chapterIdx)
{
    std::string serverUrl = Settings::instance().data().webShelf.serverUrl;
    if (serverUrl.empty())
    {
        lgerror("WebShelf serverUrl is empty.");
        return "";
    }

    char normalized[1024];
    https_normalize_url(serverUrl.c_str(), normalized);
    if (!normalized[0])
    {
        lgerror("WebShelf serverUrl is empty.");
        return "";
    }

    if (!m_meta)
    {
        lgerror("WebShelf book meta is empty.");
        return "";
    }

    char bookUrl[MAX_URL_LENGTH] = { 0 };
    https_url_encode(m_meta->bookUrl.c_str(), bookUrl, MAX_URL_LENGTH);

    switch (type)
    {
    case ReqType::CHAPTERS:
        return std::string(normalized) + "getChapterList?url=" + bookUrl;
    case ReqType::CONTENT:
        if (chapterIdx >= 0 && chapterIdx < (int)m_chapters.size())
        {
            return std::string(normalized) + "getBookContent?url=" + bookUrl + "&index=" + std::to_string(chapterIdx);
        }
        return "";
    case ReqType::SETPROGS:
        return std::string(normalized) + "saveBookProgress";
    case ReqType::GETPROGS:
        return std::string(normalized) + "getBookshelf";
    default:
        break;
    }
    return "";
}
#endif