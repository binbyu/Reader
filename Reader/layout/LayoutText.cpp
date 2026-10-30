#include "LayoutText.h"
#include "Utils.h"
#include "DpiAwareness.h"
#include "IRenderer.h"
#include "IBook.h"
#include "Settings.h"

LayoutText::LayoutText()
    : LayoutBase()
    , m_stack(maxSearchLength())
{
}

LayoutText::~LayoutText()
{
    m_stack.reset();
}

int LayoutText::findPrevPageIndex(int end, int width, int height)
{
    m_config = toPhysical(Settings::instance().data().layout);
    PageInfo pageInfo;
    pageInfo.start = end;
    pageInfo.length = 0;
    pageInfo.x = m_config.borderL;
    pageInfo.y = m_config.borderT;
    pageInfo.cGap = m_config.charGap;

    if (!isValid() || width <= 0 || height <= 0) return pageInfo.start;

    { // for cover
        if (m_source->isCover(end - 1))
        {
            return end - 1;
        }
    }

    const wchar_t* text = m_source->text();
    const int total = m_source->length();
    if (!text || end <= 0 || end > total) return pageInfo.start;

    const int readerW = width - m_config.borderL - m_config.borderR;
    const int readerH = height - m_config.borderT - m_config.borderB;
    if (readerW <= 0 || readerH <= 0) return pageInfo.start;

    int index = end;
    int usedH = 0;
    int maxLineW = 0;

    int searchLen;
    int lineLen;
    int crlfLen;
    int prefixBlankLen;
    int suffixBlankLen;
    bool isBlankLine;

    int start = m_source->hasCover() ? 1 : 0;
    while (index > start)
    {
        searchLen = min(maxSearchLength(), index);
        // [curpos - len, curpos)
        if (!Utils::prevLine(text + index, searchLen, &lineLen, &crlfLen, &isBlankLine, &prefixBlankLen, &suffixBlankLen))
        {
            break;
        }
        index -= lineLen + crlfLen;
        if (m_config.noBlansLine && isBlankLine)
        {
            continue;
        }

        std::vector<LineInfo> lines;
        if (!paragraphToLines(index, lineLen, readerW, lines))
            break;

        if (lines.empty())
        {
            ASSERT(0);
            continue;
        }
        lines.back().length += crlfLen;
        if (crlfLen == 0) lines.back().gap = m_config.lineGap;

        for (auto it = lines.rbegin(); it != lines.rend(); ++it)
        {
            const LineInfo& line = *it;
            const int realH = line.h + (pageInfo.lines.empty() ? 0 : line.gap); // last line in page neednot gap

            if (usedH + realH > readerH)
                goto DONE;

            pageInfo.lines.insert(pageInfo.lines.begin(), line);
            usedH += realH;
            maxLineW = max(maxLineW, line.x + line.w);
        }

        if (index > 0 && ((m_config.splitChapter && m_source->isChapterStart(index)) || !m_source->isChapterContinuous(false, index)))
            break;
    }

DONE:
    if (!pageInfo.lines.empty())
    {
        const LineInfo& first = pageInfo.lines.front();
        pageInfo.start = first.start;
        const LineInfo& last = pageInfo.lines.back();
        pageInfo.length = last.start + last.length - pageInfo.start;
    }
    if (m_config.alignment == LayoutConfig::CENTER) pageInfo.x += (readerW - maxLineW) / 2;
    return pageInfo.start;
}

int LayoutText::findPrevLineIndex(int end, int n, int width)
{
    if (!isValid() || width <= 0 || n <= 0) return end;

    { // for cover
        if (m_source->isCover(end - 1))
        {
            return end - 1;
        }
    }

    const wchar_t* text = m_source->text();
    const int total = m_source->length();
    if (!text || end <= 0 || end > total) return end;

    m_config = toPhysical(Settings::instance().data().layout);
    const int readerW = width - m_config.borderL - m_config.borderR;
    if (readerW <= 0) return end;

    int index = end;
    int start = m_source->hasCover() ? 1 : 0;
    while (index > start && n > 0)
    {
        int searchLen = min(maxSearchLength(), index);
        // [curpos - len, curpos)
        int lineLen;
        int crlfLen;
        int prefixBlankLen;
        int suffixBlankLen;
        bool isBlankLine;
        if (!Utils::prevLine(text + index, searchLen, &lineLen, &crlfLen, &isBlankLine, &prefixBlankLen, &suffixBlankLen))
        {
            break;
        }
        index -= lineLen + crlfLen;
        if (m_config.noBlansLine && isBlankLine)
        {
            continue;
        }

        std::vector<LineInfo> lines;
        if (!paragraphToLines(index, lineLen, readerW, lines))
            break;

        if (lines.empty())
        {
            ASSERT(0);
            continue;
        }
        lines.back().length += crlfLen;
        if (crlfLen == 0) lines.back().gap = m_config.lineGap;

        int newLines = (int)lines.size();
        if (n > newLines)
        {
            n -= newLines;
        }
        else
        {
            index = lines[lines.size() - n].start;
            break;
        }

        if (index > 0 && ((m_config.splitChapter && m_source->isChapterStart(index)) || !m_source->isChapterContinuous(false, index)))
            break;
    }

    return index;
}

PageInfo LayoutText::layout(int start, int width, int height)
{
    m_config = toPhysical(Settings::instance().data().layout);
    PageInfo pageInfo;
    pageInfo.start = start;
    pageInfo.length = 0;
    pageInfo.x = m_config.borderL;
    pageInfo.y = m_config.borderT;
    pageInfo.cGap = m_config.charGap;

    if (!isValid() || width <= 0 || height <= 0) return pageInfo;

    { // for cover
        if (m_source->isCover(start))
        {
            pageInfo.length = 1;
            pageInfo.lines.push_back({start, 1, 0});
            return pageInfo;
        }
    }

    const wchar_t* text = m_source->text();
    const int total = m_source->length();
    if (!text || start < 0 || start >= total) return pageInfo;

    const int readerW = width - m_config.borderL - m_config.borderR;
    const int readerH = height - m_config.borderT - m_config.borderB;
    if (readerW <= 0 || readerH <= 0) return pageInfo;

    int index = start;
    int usedH = 0;
    int maxLineW = 0;

    int searchLen;
    int lineLen;
    int crlfLen;
    int prefixBlankLen;
    int suffixBlankLen;
    bool isBlankLine;

    while (index < total)
    {
        searchLen = min(maxSearchLength(), (total - index));
        // [curpos, curpos + len)
        if (!Utils::nextLine(text + index, searchLen, &lineLen, &crlfLen, &isBlankLine, &prefixBlankLen, &suffixBlankLen))
        {
            break;
        }
        if (m_config.noBlansLine && isBlankLine)
        {
            index += lineLen + crlfLen;
            continue;
        }

        std::vector<LineInfo> lines;
        if (!paragraphToLines(index, lineLen, readerW, lines))
            break;

        if (lines.empty())
        {
            ASSERT(0);
            index += lineLen + crlfLen;
            continue;
        }
        lines.back().length += crlfLen;
        if (crlfLen == 0) lines.back().gap = m_config.lineGap;

        for (const LineInfo& line : lines)
        {
            if (usedH + line.h > readerH)
                goto DONE;

            pageInfo.lines.push_back(line);
            usedH += line.h + line.gap;
            maxLineW = max(maxLineW, line.x + line.w);
        }

        index += lineLen + crlfLen;

        if ((m_config.splitChapter && m_source->isChapterStart(index)) || !m_source->isChapterContinuous(true, index))
            break;
    }

DONE:
    if (!pageInfo.lines.empty())
    {
        const LineInfo& last = pageInfo.lines.back();
        pageInfo.length = last.start + last.length - pageInfo.start;
    }
    if (m_config.alignment == LayoutConfig::CENTER) pageInfo.x += (readerW - maxLineW) / 2;
    return pageInfo;
}

bool LayoutText::paragraphToLines(int start, int length, int renderW, std::vector<LineInfo>& lines)
{
    lines.clear();
    if (start < 0 || length < 0 || renderW <= 0) return false;

    const wchar_t* text = m_source->text();
    const bool isHead = start == 0 ? true : Utils::isCrLf(text[start - 1]);
    const bool isTitle = m_source->isTitleLine(start);

    initCharStack(start, length);

    int indentW = 0;
    if (!isTitle && isHead && m_config.indent)
    {
        // skip blanks character
        while (!m_stack.empty() && Utils::isBlank(text[m_stack.top().index])) m_stack.pop();

        if (!m_stack.empty())
        {
            indentW = calcIndentWidth();
            // Indentation should at least allow for the insertion of the first non-blank character
            if (indentW + m_stack.top().w > renderW) indentW = 0;
        }
    }

    LineInfo li = {0};
    li.start = m_stack.empty() ? start : m_stack.top().index;
    li.x = indentW;
    li.w = 0;
    li.h = 0;
    li.gap = m_config.lineGap;

    int breakPos = 0;
    int breakW = 0;
    int breakH = 0;

    while (!m_stack.empty())
    {
        const CharInfo& ch = m_stack.top();
        const bool blank = Utils::isBlank(text[ch.index]);
        const bool hyphen = Utils::isHyphen(text[ch.index]);
        const int addW = ch.w + (li.w > 0 ? m_config.charGap : 0);

        if (li.x + li.w + addW <= renderW)
        {
            li.chars.push_back(ch);
            li.w += addW;
            li.h = max(li.h, ch.h);
            m_stack.pop();

            if (blank || hyphen)
            {
                breakPos = (int)li.chars.size();
                breakW = li.w;
                breakH = li.h;
            }
            continue;
        }

        if (li.w == 0) return false;

        if (m_config.wordWrap && breakPos > 0)
        {
            if (blank)
            {
                // add more blanks
                while (!m_stack.empty() && Utils::isBlank(text[m_stack.top().index])) m_stack.pop();
            }

            // re push to stack
            for (int j = (int)li.chars.size() - 1; j >= (int)breakPos; --j) m_stack.push(li.chars[j]);
            li.chars.erase(li.chars.begin() + breakPos, li.chars.end());

            if (!m_stack.empty()) li.length = m_stack.top().index - li.start;
            else li.length = start + length - li.start;

            li.w = breakW;
            li.h = breakH;
            lines.emplace_back(std::move(li));

            // reset
            if (!m_stack.empty())
            {
                li.start = m_stack.top().index;
            }
            li.length = 0;
            li.x = 0;
            li.w = 0;
            li.h = 0;
            li.gap = m_config.lineGap;
            breakPos = 0;
            breakW = 0;
            breakH = 0;
            continue;
        }

        if (!li.chars.empty())
        {
            li.length = ch.index - li.start;
            lines.emplace_back(std::move(li));
        }

        // reset
        li.start = ch.index;
        li.length = 0;
        li.x = 0;
        li.w = 0;
        li.h = 0;
        li.gap = m_config.lineGap;
        breakPos = 0;
        breakW = 0;
        breakH = 0;
    }

    // add left character
    if (!li.chars.empty())
    {
        li.length = start + length - li.start;
        lines.emplace_back(std::move(li));
    }

    // for empty paragraph
    if (lines.empty())
    {
        LineInfo li;
        li.start = start;
        li.length = length;
        li.x = 0;
        li.w = 0;
        li.h = emptyLineHeight();
        li.gap = m_config.lineGap;
        lines.emplace_back(std::move(li));
    }

    // The interval for the lines must cover the entire paragraph
    auto& front = lines.front();
    if (front.start > start)
    {
        front.length += front.start - start;
        front.start = start;
    }
    auto& back = lines.back();
    back.length = start + length - back.start;
    back.gap = m_config.paragraphGap;

    validateLines(text, start, length, renderW, m_config.charGap, lines);
    return true;
}

void LayoutText::initCharStack(int start, int length)
{
    m_stack.reset();
    if (!m_source) return;

    const wchar_t* text = m_source->text();
    int end = start + length;
    bool isTitle = m_source->isTitleLine(start);
    int tsIdx = -1;

    for (int i = end - 1; i >= start; --i)
    {
        if (isTitle)
        {
            tsIdx = m_styles.titleIdx;
        }
        else
        {
#if ENABLE_TAG
            int tagIdx = tagIndex(i);
            if (tagIdx >= 0 && tagIdx < (int)m_styles.tagIdx.size())
                tsIdx = m_styles.tagIdx[tagIdx];
            else
#endif
                tsIdx = m_styles.textIdx;
        }

        TextMetrics tm = m_measurer->measureText(text + i, 1, -1, tsIdx);
        m_stack.push({ i, tsIdx, tm.width, tm.height });
    }
}

int LayoutText::calcIndentWidth() const
{
    if (!m_measurer) return 0;
    const wchar_t text[] = { 0x3000, 0x3000, 0 };
    int width = m_measurer->measureText(text, 2, -1, m_styles.textIdx).width;
    return width + (m_config.charGap * 2);
}

int LayoutText::emptyLineHeight() const
{
    if (!m_measurer) return 0;
    TextMetrics tm = m_measurer->measureText(L"A", 1, -1, m_styles.textIdx);
    return tm.height;
}

int LayoutText::maxSearchLength() const
{
    return 4096;
}

#if ENABLE_TAG
int LayoutText::tagIndex(int index)
{
    if (!m_source) return -1;
    const auto& tags = Setting::instance().data().tags;
    for (int i=0; i<(int)tags.size(); i++)
    {
        const auto& tag = tags[i];
        if (tag.enable && !tag.keyword.empty())
        {
            int length = (int)tag.keyword.size();
            int start = index - length + 1;
            if (start < 0) start = 0;
            int end = min(index + 1, m_source->length() - length + 1);
            if (end > m_source->length()) end = m_source->length();

            for (int j=start; j<end; j++)
            {
                if (0 == wcsncmp(tag.keyword.c_str(), m_source->text()+j, length))
                {
                    return i;
                }
            }
        }
    }
    return -1;
}
#endif

void LayoutText::validateLines(const wchar_t* text, int start, int length, int renderW, int cGap, const std::vector<LineInfo>& lines) const
{
#if ENABLE_TESTCASE
    ASSERT(text != nullptr);
    ASSERT(start >= 0);
    ASSERT(length >= 0);
    ASSERT(renderW > 0);
    ASSERT(cGap >= 0);
    ASSERT(!lines.empty());

    if (!text || start < 0 || length < 0 || renderW <= 0 || cGap < 0 || lines.empty()) return;

    const int end = start + length;

    if (length == 0)
    {
        ASSERT(lines.size() == 1);
        if (lines.size() != 1) return;

        const auto& line = lines.front();

        ASSERT(line.start == start);
        ASSERT(line.length == 0);
        ASSERT(line.x == 0);
        ASSERT(line.w == 0);
        ASSERT(line.h == emptyLineHeight());
        ASSERT(line.chars.empty());
        return;
    }

    int expectedStart = start;

    for (const auto& line : lines)
    {
        const int lineEnd = line.start + line.length;

        ASSERT(line.length > 0);
        ASSERT(line.start == expectedStart);
        ASSERT(lineEnd <= end);
        ASSERT(line.x >= 0);
        ASSERT(line.w >= 0);
        ASSERT(line.x + line.w <= renderW);
        ASSERT(line.h >= 0);

        if (line.length <= 0 || line.start != expectedStart ||
            lineEnd > end || line.x < 0 || line.w < 0 ||
            line.x + line.w > renderW || line.h < 0)
            return;

        int lastIndex = line.start - 1;
        int contentW = 0;
        int contentH = 0;

        for (size_t i = 0; i < line.chars.size(); ++i)
        {
            const CharInfo& ch = line.chars[i];

            ASSERT(ch.index >= line.start);
            ASSERT(ch.index < lineEnd);
            ASSERT(ch.index > lastIndex);
#if ENABLE_TAG
            ASSERT(ch.tsIdx >= 0 && ch.tsIdx < Setting::instance().data().tags.size() + 2);
#else
            ASSERT(ch.tsIdx >= 0 && ch.tsIdx < 2);
#endif
            // U+0300 ~ U+036F   Combining Diacritical Marks
            // U+200B  ZERO WIDTH SPACE
            // U+200C  ZERO WIDTH NON-JOINER
            // U+200D  ZERO WIDTH JOINER
            // U+200E  LEFT-TO-RIGHT MARK
            // U+200F  RIGHT-TO-LEFT MARK
            // U+2060  WORD JOINER
            // U+2061  FUNCTION APPLICATION
            // U+2062  INVISIBLE TIMES
            // U+2063  INVISIBLE SEPARATOR
            // U+2064  INVISIBLE PLUS
            //if (text[ch.index] < 0x0300 || text[ch.index] > 0x036F) ASSERT(ch.w > 0);
            ASSERT(ch.h > 0);

            for (int j = lastIndex + 1; j < ch.index; ++j)
            {
                ASSERT(Utils::isBlank(text[j]));
            }

            if (i > 0) contentW += cGap;
            contentW += ch.w;
            contentH = max(contentH, ch.h);
            lastIndex = ch.index;
        }

        for (int i = lastIndex + 1; i < lineEnd; ++i)
            ASSERT(Utils::isBlank(text[i]));

        ASSERT(contentW == line.w);
        ASSERT(contentW <= renderW - line.x);

        if (contentW != line.w || contentW > renderW - line.x)
            return;

        if (!line.chars.empty())
            ASSERT(contentH == line.h);

        expectedStart = lineEnd;
    }

    ASSERT(expectedStart == end);
#endif
}
