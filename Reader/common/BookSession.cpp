#include "BookSession.h"
#include "BookText.h"
#include "BookEpub.h"
#include "BookMobi.h"
#include "BookOnline.h"
#include "BookWeb.h"
#include "LayoutText.h"
#include "Settings.h"
#include "Utils.h"
#include "Logger.h"

BookSession::BookSession(HWND hWnd)
    : m_hWnd(hWnd)
    , m_layout(nullptr)
    , m_book(nullptr)
    , m_bookInfo(nullptr)
    , m_state(BookState::CLOSED)
    , m_errno(BookErrno::SUCC)
    , m_pageInfo({})
    , m_styles({0})
    , m_width(0)
    , m_height(0)
    , m_checkUpdateQueue()
{
    IRenderer::instance()->initialize();
    m_layout = new LayoutText();
    m_layout->setMeasurer(IRenderer::instance()->measurer());
    setTextStyles();
}

BookSession::~BookSession()
{
    uninit();
    IRenderer::instance()->cleanup();
}

void BookSession::init()
{
    std::wstring fileName;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto& books = Settings::instance().data().books;
        while (!books.empty())
        {
            auto& item = books.front();
            if (!Utils::isFileExist(item.fileName))
            {
                books.erase(books.begin());
                continue;
            }
            fileName = item.fileName;
            break;
        }
    }
    if (!fileName.empty())
    {
        open(fileName);
    }
    PostMessageW(m_hWnd, WM_UPDATEBOOKS, 0, NULL);
}

void BookSession::uninit()
{
    close();
    if (m_layout)
    {
        delete m_layout;
        m_layout = nullptr;
    }
}

void BookSession::open(int index)
{
    std::wstring fileName;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto& books = Settings::instance().data().books;
        if (index >= 0 && index < (int)books.size())
        {
            fileName = books[index].fileName;
        }
    }
    if (!fileName.empty())
    {
        open(fileName);
    }
}

void BookSession::open(const std::wstring& fileName, bool forced)
{
    if (!forced && m_book && m_book->fileName() == fileName) return;

    close();
    setState(BookState::LOADING);

    m_thread = std::thread([=] {

        if (!Utils::isFileExist(fileName))
        {
            remBookItem(fileName);
            setError(BookErrno::NOT_EXIST);
            setState(BookState::FAILED);
            return;
        }

        if (Utils::isFileEmpty(fileName))
        {
            remBookItem(fileName);
            setError(BookErrno::EMPTY);
            setState(BookState::FAILED);
            return;
        }

        std::wstring ext = Utils::fileExt(fileName);
        if (ext == L".txt")
        {
            m_book = new BookText(fileName);
        }
        else if (ext == L".epub")
        {
            m_book = new BookEpub(fileName);
        }
        else if (ext == L".mobi")
        {
            m_book = new BookMobi(fileName);
        }
    #ifdef ENABLE_NETWORK
        else if (ext == L".ol")
        {
            m_book = new BookOnline(fileName);
            dynamic_cast<BookOnline*>(m_book)->setHWND(m_hWnd);
        }
        else if (ext == L".web")
        {
            m_book = new BookWeb(fileName);
            dynamic_cast<BookWeb*>(m_book)->setHWND(m_hWnd);
        }
    #endif
        else
        {
            remBookItem(fileName);
            setError(BookErrno::UNKNOWN_FORMAT);
            setState(BookState::FAILED);
            return;
        }

        addBookItem(fileName);
        auto& books = Settings::instance().data().books;
        m_bookInfo = &books[0];
        m_book->setBookInfo(m_bookInfo);

        auto& data = Settings::instance().data();
        m_book->setChapterRule(&data.chapterRule);

        BookErrno err = m_book->open();
        if (m_book->isAbort())
        {
            return;
        }
        if (BookErrno::SUCC != err)
        {
            if (m_book->length() <= 0)
            {
                remBookItem(fileName);
                setError(err);
                setState(BookState::FAILED);
                return;
            }
            // for online/web book, fetch failed but still can opened
            setError(err);
            setState(BookState::UNREADY);
            m_layout->setSource(m_book);
            PostMessageW(m_hWnd, WM_UPDATECHAPTERS, 0, NULL);
            PostMessageW(m_hWnd, WM_UPDATEMARKS, 0, NULL);
            return;
        }
        setState(BookState::OPENED);
        m_layout->setSource(m_book);
        PostMessageW(m_hWnd, WM_UPDATECHAPTERS, 0, NULL);
        PostMessageW(m_hWnd, WM_UPDATEMARKS, 0, NULL);
    });
}

void BookSession::reopen()
{
    if (m_book) open(m_book->fileName(), true);
}

void BookSession::close()
{
    if (m_book) m_book->close();

    if (m_thread.joinable()) m_thread.join();

    m_layout->setSource(nullptr);

    if (m_book)
    {
        delete m_book;
        m_book = nullptr;
    }
    m_bookInfo = nullptr;
    m_pageInfo.clear();

    PostMessageW(m_hWnd, WM_UPDATECHAPTERS, 0, NULL);
    PostMessageW(m_hWnd, WM_UPDATEMARKS, 0, NULL);
    setState(BookState::CLOSED);
}

void BookSession::remove(const std::wstring& fileName)
{
    if (m_book && m_book->fileName() == fileName) close();
    remBookItem(fileName);
    open(0);
}

void BookSession::remove(int index)
{
    std::wstring fileName;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto& books = Settings::instance().data().books;
        if (index >= 0 && index < (int)books.size())
        {
            fileName = books[index].fileName;
        }
    }
    if (!fileName.empty())
    {
        remove(fileName);
    }
}

void BookSession::clear()
{
    close();
    clrBookItem();
}

void BookSession::refreshChapter()
{
    if (m_book && m_book->isRefreshSupport())
    {
        setState(BookState::LOADING);
        m_book->refreshChapter([this](BookErrno err){
            if (BookErrno::SUCC != err)
            {
                setError(err);
                setState(BookState::UNREADY);
                return;
            }
            setState(BookState::OPENED);
        });
    }
}

void BookSession::refreshContent()
{
    if (m_book && m_book->isRefreshSupport())
    {
        setState(BookState::LOADING);
        m_book->refreshContent(m_book->curChapterIndex(), [this](BookErrno err){
            if (BookErrno::SUCC != err)
            {
                setError(err);
                setState(BookState::UNREADY);
                return;
            }
            setState(BookState::OPENED);
        });
    }
}

void BookSession::checkBookUpdate()
{
#ifdef ENABLE_NETWORK
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_checkUpdateQueue.empty()) return;

        auto& books = Settings::instance().data().books;
        m_checkUpdateQueue.reserve(books.size());

        for (auto& bi : books)
        {
            if (m_book && m_book->fileName() == bi.fileName)
            {
                if (m_book->isRefreshSupport())
                {
                    m_book->refreshChapter(nullptr);
                }
                continue;
            }

            if (Utils::fileExt(bi.fileName) == L".ol")
            {
                m_checkUpdateQueue.push_back(bi.fileName);
            }
        }

        if (m_checkUpdateQueue.empty()) return;
    }

    // to reduce memory usage, check book update one by one.
    checkNext();
#endif
}

BookState BookSession::state() const
{
    return m_state;
}

BookErrno BookSession::lastError() const
{
    return m_errno;
}

bool BookSession::isLoading() const
{
    return m_state == BookState::LOADING;
}

bool BookSession::isValid() const
{
    return m_state == BookState::OPENED || m_state == BookState::UNREADY;
}

bool BookSession::isEnd() const
{
    if (!m_book) return true;
    if (m_book->isFetchSupport())
    {
        int cnt = m_book->chapterCount();
        for (int i = 0; i < cnt; i++)
        {
            const auto* c = m_book->chapter(i);
            if (c->index == -1) return false;
        }
        return m_book->index() + pageLength() >= m_book->length();
    }
    else
    {
        return m_book->index() + pageLength() >= m_book->length();
    }
}

IBook* BookSession::book() const
{
    return m_book;
}

float BookSession::progress() const
{
    if (m_book) return ((m_book->index() + pageLength()) * 100.0f) / m_book->length();
    return 0.0f;
}

std::wstring BookSession::progressStr() const
{
    if (m_book)
    {
        int curn = m_book->index() + pageLength();
        int total = m_book->length();
        float prog = total > 0 ? (curn * 100.0f) / total : 0.0f;

        wchar_t buffer[256];
        swprintf_s(buffer, L"  %.2f%%  ( %d / %d )", prog, curn, total);
        return buffer;
    }
    return L"";
}

void BookSession::seek(int index)
{
    if (m_bookInfo && m_book && isValid())
    {
        if (m_book->setIndex(index))
        {
            preFetchChapter(m_book->curChapterIndex());
            Settings::instance().save();
            PostMessageW(m_hWnd, WM_REDRAW, 0, NULL);
        }
    }
}

void BookSession::seek(float prog)
{
    if (m_book) seek((int)(prog / 100.0f * m_book->length()));
}

void BookSession::seek(bool forward, const wchar_t* keyword)
{
    if (m_book && keyword && *keyword)
    {
        size_t len = wcslen(keyword);
        size_t cmpsize = len * sizeof(wchar_t);

        if (forward)
        {
            for (int i = m_book->index() + 1; i < m_book->length() - (int)len + 1; i++)
            {
                if (0 == memcmp(keyword, m_book->text() + i, cmpsize))
                {
                    return seek(i);
                }
            }
        }
        else
        {
            for (int i = m_book->index() - 1; i >= 0; i--)
            {
                if (0 == memcmp(keyword, m_book->text() + i, cmpsize))
                {
                    return seek(i);
                }
            }
        }
    }
}

void BookSession::seekChapter(int index)
{
    if (fetchChapter(SEEK_CHAPTER, index)) return;

    if (m_book)
    {
        auto* c = m_book->chapter(index);
        if (c)
        {
            seek(c->index);
        }
    }
}

void BookSession::nextLine(int n)
{
    if (fetchChapter(NEXT_LINE, -1, n)) return;

    if (n > 0)
    {
        if (n < (int)m_pageInfo.lines.size())
        {
            seek(m_pageInfo.lines[n].start);
        }
        else
        {
            nextPage();
        }
    }
}

void BookSession::prevLine(int n)
{
    if (fetchChapter(PREV_LINE, -1, n)) return;

    if (m_layout && m_book) seek(m_layout->findPrevLineIndex(m_book->index(), n, m_width));
}

void BookSession::nextPage()
{
    if (fetchChapter(NEXT_PAGE)) return;

    if (m_book) seek(m_book->index() + pageLength());
}

void BookSession::prevPage()
{
    if (fetchChapter(PREV_PAGE)) return;

    if (m_layout && m_book) seek(m_layout->findPrevPageIndex(m_book->index(), m_width, m_height));
}

void BookSession::nextChapter()
{
    if (m_book)
    {
        int idx = m_book->curChapterIndex();
        int cnt = m_book->chapterCount();
        if (idx + 1 < cnt)
        {
            seekChapter(idx + 1);
        }
    }
}

void BookSession::prevChapter()
{
    if (m_book)
    {
        int idx = m_book->curChapterIndex();
        auto* c = m_book->chapter(idx);
        if (!c) return;
        if (c->index < m_book->index())
        {
            seek(c->index);
            return;
        }
        if (idx > 0)
        {
            seekChapter(idx - 1);
            return;
        }
    }
}

const Marks& BookSession::marks()
{
    if (m_bookInfo)
    {
        return m_bookInfo->marks;
    }
    static Marks empty;
    return empty;
}

void BookSession::addMark(int index)
{
    if (m_bookInfo)
    {
        auto& marks = m_bookInfo->marks;
        auto it = std::lower_bound(marks.begin(), marks.end(), m_bookInfo->index);
        if (it == marks.end())
        {
            marks.insert(it, m_bookInfo->index);
            Settings::instance().save();
            PostMessageW(m_hWnd, WM_UPDATEMARKS, 0, NULL);
        }
    }
}

void BookSession::remMark(int index)
{
    if (m_bookInfo)
    {
        auto& marks = m_bookInfo->marks;
        auto it = std::find(marks.begin(), marks.end(), (int)index);
        if (it != marks.end())
        {
            marks.erase(it);
            Settings::instance().save();
            PostMessageW(m_hWnd, WM_UPDATEMARKS, 0, NULL);
        }
    }
}

void BookSession::clrMark()
{
    if (m_bookInfo)
    {
        if (!m_bookInfo->marks.empty())
        {
            m_bookInfo->marks.clear();
            Settings::instance().save();
            PostMessageW(m_hWnd, WM_UPDATEMARKS, 0, NULL);
        }
    }
}

int BookSession::pageLength() const
{
    return m_pageInfo.length;
}

const PageInfo& BookSession::pageInfo() const
{
    return m_pageInfo;
}

void BookSession::drawPage(HDC hdc, const RECT& rect)
{
    if (!m_book || isLoading()) return;

    m_width = rect.right - rect.left;
    m_height = rect.bottom - rect.top;

    m_pageInfo = m_layout->layout(m_book->index(), m_width, m_height);
    if (!m_pageInfo.empty() && m_book->isCover(m_pageInfo.start))
    {
        auto* cover = m_book->cover();
        if (cover)
        {
            auto image = IRenderer::instance()->loadImageFromStream((BYTE*)cover->data, cover->size);
            if (image)
            {
                IRenderer::instance()->drawImage(image, rect, true);
                delete image;
                return;
            }
        }
    }

    int x;
    int y = m_pageInfo.y;
    RECT rc;
    for (const auto& line : m_pageInfo.lines)
    {
        x = line.x + m_pageInfo.x;
        const size_t count = line.chars.size();
        for (size_t i = 0; i < count; ++i)
        {
            const auto& c = line.chars[i];
            rc.left = x;
            rc.top = y + line.h - c.h;
            rc.right = rc.left + c.w;
            // If the next character has the same style and is contiguous, extend the background rectangle to include cGap
            if (i + 1 < count && line.chars[i + 1].tsIdx == c.tsIdx && line.chars[i + 1].index == c.index + 1)
            {
                rc.right += m_pageInfo.cGap;
            }
            rc.bottom = rc.top + c.h;
            IRenderer::instance()->drawText(m_book->text() + c.index, 1, rc, c.tsIdx);
            x += c.w + m_pageInfo.cGap;
        }
        y += line.h + line.gap;
    }
}

void BookSession::updateStyles()
{
    setTextStyles();
    PostMessageW(m_hWnd, WM_REDRAW, 0, NULL);
}

const TextStyles& BookSession::styles() const
{
    return m_styles;
}

LRESULT BookSession::onEvent(WPARAM wParam, LPARAM lParam)
{
    if (m_book) return m_book->onEvent(wParam, lParam);
    return -1;
}

LRESULT BookSession::onTimer(WPARAM wParam, LPARAM lParam)
{
    if (m_book) return m_book->onTimer(wParam, lParam);
    return -1;
}

void BookSession::addBookItem(const std::wstring& fileName)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& books = Settings::instance().data().books;
    auto it = std::find_if(books.begin(), books.end(), [&](const BookInfo& bi) { return bi.fileName == fileName; });
    if (it != books.end())
    {
        it->hasNew = false;
        std::rotate(books.begin(), it, it + 1);
    }
    else
    {
        BookInfo bi;
        bi.fileName = fileName;
        bi.index = 0;
        bi.hasNew = false;
        books.insert(books.begin(), bi);
    }
    Settings::instance().save();
    PostMessageW(m_hWnd, WM_UPDATEBOOKS, 0, NULL);
}

void BookSession::remBookItem(const std::wstring& fileName)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& books = Settings::instance().data().books;
    auto it = std::find_if(books.begin(), books.end(), [fileName](const BookInfo& bi) { return bi.fileName == fileName; });
    if (it != books.end())
    {
        books.erase(it);
        Settings::instance().save();
        PostMessageW(m_hWnd, WM_UPDATEBOOKS, 0, NULL);
    }
}

void BookSession::clrBookItem()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& books = Settings::instance().data().books;
    if (books.empty()) return;
    books.clear();
    Settings::instance().save();
    PostMessageW(m_hWnd, WM_UPDATEBOOKS, 0, NULL);
}

void BookSession::setState(BookState state)
{
    if (m_state != state)
    {
        lgdebug("Book state changed %d -> %d", (int)m_state, (int)state);
        m_state = state;
        PostMessageW(m_hWnd, WM_BOOKSTATECHANGED, 0, (LPARAM)m_state);
    }
}

void BookSession::setError(BookErrno err)
{
    m_errno = err;
}

void BookSession::setTextStyles()
{
    auto& data = Settings::instance().data();
    size_t size = 2
#if ENABLE_TAG
        + data.tags.size()
#endif
        ;

    m_styles.resize(size);

    BYTE textAlpha = data.window.textAlpha;
    int idx = 0;
    m_styles[idx++] = TextStyle(data.styles.textFont, textAlpha << 24 | data.styles.textColor);
    m_styles[idx++] = TextStyle(data.styles.titleFont, textAlpha << 24 | data.styles.titleColor);
#if ENABLE_TAG
    for (auto& tag : data.tags)
    {
        m_styles[idx++] = TextStyle(tag.font, textAlpha << 24 | tag.textColor, 0xff000000 | tag.backgroundColor);
    }
#endif

    if (m_layout)
    {
        TextStyleConfig styles = { 0, data.styles.unified ? 0 : 1 };
#if ENABLE_TAG
        idx = 2;
        for (auto& tag : data.tags)
        {
            styles.tagIdx.push_back(idx++);
        }
#endif
        m_layout->setTextStyle(styles);
    }
}

bool BookSession::isChapterReady(int chapterIdx)
{
    if (isValid() && m_book && m_book->isFetchSupport())
    {
        if (chapterIdx >= 0 && chapterIdx < m_book->chapterCount())
        {
            const auto* c = m_book->chapter(chapterIdx);
            return c->index != -1;
        }
    }
    return true;
}

bool BookSession::fetchChapter(FetchType type, int chapterIdx, int n)
{
    if (!isValid() || !m_book || !m_book->isFetchSupport()) return false;

    int total = m_book->chapterCount();
    if (total <= 0) return false;

    int curnIdx = m_book->curChapterIndex();
    const Chapter* curnChapter = m_book->chapter(curnIdx);
    if (!curnChapter) return false;

    int targetIdx = -1;
    bool switchToLoading = false;

    switch (type)
    {
    case SEEK_CHAPTER:
        if (chapterIdx < 0 || chapterIdx >= total) return false;
        targetIdx = chapterIdx;
        if (!isChapterReady(chapterIdx)) switchToLoading = true;
        break;
    case PREV_PAGE:
    case PREV_LINE:
        targetIdx = curnIdx - 1;
        if (targetIdx >= 0)
        {
            if (m_book->index() <= curnChapter->index && !isChapterReady(targetIdx)) switchToLoading = true;
        }
        break;
    case NEXT_PAGE:
    case NEXT_LINE:
        targetIdx = curnIdx + 1;
        if (targetIdx < total)
        {
            int chapterEnd = (curnChapter->textLen > 0) ? (curnChapter->index + curnChapter->textLen) : m_book->length();
            if (m_book->index() + pageLength() >= chapterEnd && !isChapterReady(targetIdx)) switchToLoading = true;
        }
        break;
    default:
        return false;
    }

    if (switchToLoading)
    {
        setState(BookState::LOADING);
        m_book->fetchChapter(targetIdx, [this, type, targetIdx, n](BookErrno err) {
            if (BookErrno::SUCC != err)
            {
                setError(err);
                setState(BookState::UNREADY);
                return;
            }

            setState(BookState::OPENED);

            // restore paging
            switch (type)
            {
            case SEEK_CHAPTER: seekChapter(targetIdx); break;
            case PREV_PAGE:    prevPage();  break;
            case PREV_LINE:    prevLine(n); break;
            case NEXT_PAGE:    nextPage();  break;
            case NEXT_LINE:    nextLine(n); break;
            default: break;
            }
        });
    }

    // pre fetch
    preFetchChapter(targetIdx);
    return switchToLoading;
}

void BookSession::preFetchChapter(int curn)
{
    if (!isValid() || !m_book || !m_book->isFetchSupport()) return;

    int total = m_book->chapterCount();
    int prev = curn - 1;
    int next = curn + 1;

    if (next < total && !isChapterReady(next)) m_book->fetchChapter(next);
    if (prev >= 0 && !isChapterReady(prev)) m_book->fetchChapter(prev);
}

void BookSession::checkNext()
{
#ifdef ENABLE_NETWORK
    std::wstring fileName;
    BookInfo* bookInfo = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_checkUpdateQueue.empty()) return;

        fileName = m_checkUpdateQueue.front();
        m_checkUpdateQueue.erase(m_checkUpdateQueue.begin());

        auto& books = Settings::instance().data().books;
        auto it = std::find_if(books.begin(), books.end(), [&](const BookInfo& bi) { return bi.fileName == fileName; });
        if (it != books.end()) bookInfo = &(*it);
    }
    if (!bookInfo || fileName.empty()) return;

    auto* book = new BookOnline(fileName);
    book->setHWND(m_hWnd);
    book->setBookInfo(bookInfo);
    book->checkUpdate([this, book](BookErrno err) {
        delete book;
        checkNext();
    });
#endif
}