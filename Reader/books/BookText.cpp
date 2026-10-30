#pragma once
#include "BookText.h"
#include "Utils.h"
#include "Logger.h"
#include <regex>

#define MAX_CHAPTER_TITLE_LENGTH    256

BookText::BookText(const std::wstring& fileName)
    : BookBase(fileName)
{
}

BookText::~BookText()
{
}

BookType BookText::type() const
{
    return BookType::TEXT;
}

bool BookText::isEditSupport() const
{
    return true;
}

bool BookText::editText(int start, int srcLen, wchar_t* const dstText, int dstLen)
{
    if (!formatText(dstText, &dstLen))
    {
        return false;
    }

    int delta = dstLen - srcLen;
    if (delta == 0)
    {
        if (0 == memcmp(m_text + start, dstText, dstLen * sizeof(wchar_t)))
        {
            return true;
        }
        memcpy(m_text + start, dstText, dstLen * sizeof(wchar_t));
    }
    else if (delta < 0)
    {
        memcpy(m_text + start, dstText, dstLen * sizeof(wchar_t));
        memmove(m_text + start + dstLen, m_text + start + srcLen, (m_length - (start + srcLen)) * sizeof(wchar_t));
        m_length += delta;
        m_text[m_length] = 0;
        moveChapterIndex(start, delta);
    }
    else
    {
        wchar_t* newText = (wchar_t*)malloc((m_length + delta + 1) * sizeof(wchar_t));
        if (start > 0)
        {
            memcpy(newText, m_text, start * sizeof(wchar_t));
        }
        memcpy(newText + start, dstText, dstLen * sizeof(wchar_t));
        memcpy(newText + start + dstLen, m_text + start + srcLen, (m_length - (start + srcLen)) * sizeof(wchar_t));
        free(m_text);
        m_text = newText;
        m_length += delta;
        m_text[m_length] = 0;
        moveChapterIndex(start, delta);
    }
    return saveFile() == BookErrno::SUCC;
}

BookErrno BookText::parseBook()
{
    char* data = nullptr;
    int size = 0;
    BookErrno err = readFile(data, size);
    if (err != BookErrno::SUCC)
    {
        return err;
    }

    // empty file
    if (size == 0)
    {
        m_text = (wchar_t*)malloc(sizeof(wchar_t));
        m_length = 0;
        m_text[0] = 0;
        return BookErrno::SUCC;
    }

    if (!decodeText(data, size, &m_text, &m_length))
    {
        free(data);
        lgwarn("Failed to decode text.");
        return BookErrno::FAIL;
    }

    if (!parserChapters())
    {
        free(data);
        free(m_text);
        m_text = nullptr;
        m_length = 0;
        lgwarn("Failed to parser chapters.");
        return BookErrno::FAIL;
    }
    return BookErrno::SUCC;
}

BookErrno BookText::readFile(char*& data, int& size)
{
    FILE* fp = nullptr;
    _wfopen_s(&fp, m_fileName.c_str(), L"rb");
    if (!fp)
    {
        lgerror("Failed to open book file. Error code: %lu", GetLastError());
        return BookErrno::OPEN_R_FAIL;
    }
    fseek(fp, 0, SEEK_END);
    size = (int)ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (size < 0)
    {
        fclose(fp);
        return BookErrno::FAIL;
    }
    if (size == 0)
    {
        fclose(fp);
        data = nullptr;
        lgerror("Open an empty book.");
        return BookErrno::EMPTY;
    }
    data = (char*)malloc(size);
    if (!data)
    {
        fclose(fp);
        lgerror("Failed to allocate memory.");
        return BookErrno::TOO_LARGE;
    }
    if (size != fread(data, 1, size, fp))
    {
        fclose(fp);
        free(data);
        data = nullptr;
        size = 0;
        lgerror("Failed to read book file.");
        return BookErrno::READ_FAIL;
    }
    fclose(fp);
    return BookErrno::SUCC;
}

BookErrno BookText::saveFile()
{
    if (!isEditSupport())
    {
        return BookErrno::FAIL;
    }
    FILE* fp = nullptr;
    _wfopen_s(&fp, m_fileName.c_str(), L"wb");
    if (!fp)
    {
        return BookErrno::OPEN_W_FAIL;
    }
    if (1 != fwrite("\xff\xfe", sizeof(wchar_t), 1, fp))
    {
        fclose(fp);
        return BookErrno::WRITE_FAIL;
    }
    if (m_length != fwrite(m_text, sizeof(wchar_t), m_length, fp))
    {
        fclose(fp);
        return BookErrno::WRITE_FAIL;
    }
    fclose(fp);
    return BookErrno::SUCC;
}

bool BookText::parserChapters()
{
    m_chapters.clear();
    if (m_chapterRule && m_chapterRule->rule == ChapterRule::KEYWORD)
    {
        return parserChaptersKeyword();
    }
    else if (m_chapterRule && m_chapterRule->rule == ChapterRule::REGEX)
    {
        return parserChaptersRegex();
    }
    else
    {
        return parserChaptersDefault();
    }
}

bool BookText::parserChaptersDefault()
{
    int lineLen = 0;
    int crlfLen = 0;
    int titleLen = 0;
    int i;
    int idx_1 = -1;
    int idx_2 = -1;
    int prefixBlankLen = 0;
    int suffixBlankLen = 0;
    bool bFound = false;
    wchar_t* text = m_text;
    Chapter chapter;

    while (1)
    {
        if (m_abort)
        {
            return false;
        }

        if (!Utils::nextLine(text, m_length - (int)(text - m_text), &lineLen, &crlfLen, NULL, &prefixBlankLen, &suffixBlankLen))
        {
            break;
        }

        bFound = false;
        idx_1 = -1;
        idx_2 = -1;
        for (i = prefixBlankLen; i < lineLen; i++)
        {
            if (idx_1 == -1
                && i + 1 < lineLen
                && (text[i] == L'楔' && text[i + 1] == L'子'
                    || text[i] == L'序' && text[i + 1] == L'章')
                && (i + 2 == lineLen
                    || (i + 2 < lineLen && isValidSeparator(text[i + 2]))))
            {
                idx_1 = i;
                bFound = true;
                break;
            }

            if (text[i] == L'第')
            {
                idx_1 = i;
                idx_2 = -1;
            }

            if ((idx_1 != -1 && idx_2 == -1)
                && isValidChapter(text[i])
                && (i + 1 == lineLen
                    || (i + 1 < lineLen && isValidSeparator(text[i + 1]))))
            {
                idx_2 = i;
            }

            if ((idx_1 != -1 && idx_2 != -1)
                && idx_1 + 1 < idx_2)
            {
                bFound = true;
                break;
            }
        }
        if (bFound
            && idx_1 != -1
            && (idx_2 == -1
                || (idx_2 > -1 && isValidNumber(text + idx_1 + 1, idx_2 - idx_1 - 1))))
        {
            titleLen = lineLen - suffixBlankLen - idx_1;
            if (titleLen > MAX_CHAPTER_TITLE_LENGTH) titleLen = MAX_CHAPTER_TITLE_LENGTH;

            chapter.index = (int)(text - m_text);
            chapter.title = std::wstring(text + idx_1, titleLen);
            chapter.titleLineLen = lineLen;
            m_chapters.emplace_back(chapter);
        }

        text += lineLen + crlfLen;
    }

    return true;
}

bool BookText::parserChaptersKeyword()
{
    int lineLen = 0;
    int crlfLen = 0;
    int titleLen = 0;
    bool bFound = false;
    int idx_1 = -1;
    int i, cnt;
    int prefixBlankLen = 0;
    int suffixBlankLen = 0;
    wchar_t* text = m_text;
    Chapter chapter;

    while (1)
    {
        if (m_abort)
        {
            return false;
        }

        if (!Utils::nextLine(text, m_length - (int)(text - m_text), &lineLen, &crlfLen, NULL, &prefixBlankLen, &suffixBlankLen))
        {
            break;
        }

        if (lineLen >= (int)m_chapterRule->keyword.size())
        {
            bFound = false;
            idx_1 = -1;
            cnt = lineLen - (int)m_chapterRule->keyword.size();
            for (i = 0; i <= cnt; i++)
            {
                if (wcsncmp(text + i, m_chapterRule->keyword.c_str(), m_chapterRule->keyword.size()) == 0)
                {
                    idx_1 = i;
                    bFound = true;
                    break;
                }
            }
            if (bFound)
            {
                titleLen = lineLen - suffixBlankLen - prefixBlankLen;
                if (titleLen > MAX_CHAPTER_TITLE_LENGTH) titleLen = MAX_CHAPTER_TITLE_LENGTH;

                chapter.index = (int)(text - m_text);
                chapter.title = std::wstring(text + prefixBlankLen, titleLen);
                chapter.titleLineLen = lineLen;
                m_chapters.emplace_back(chapter);
            }
        }

        text += lineLen + crlfLen;
    }
    return true;
}

bool BookText::parserChaptersRegex()
{
    try
    {
        std::wregex regex(m_chapterRule->regex, std::regex::optimize);

        const wchar_t* begin = m_text;
        const wchar_t* end = m_text + m_length;

        auto words_begin = std::wcregex_iterator(begin, end, regex);
        auto words_end = std::wcregex_iterator();

        Chapter chapter;
        for (std::wcregex_iterator i = words_begin; i != words_end; ++i)
        {
            if (m_abort) return false;

            const std::wcmatch& match = *i;

            int titleLen = (int)match.length();
            if (titleLen == 0) continue;

            if (titleLen > MAX_CHAPTER_TITLE_LENGTH) titleLen = MAX_CHAPTER_TITLE_LENGTH;

            chapter.index = (int)match.position();
            chapter.title = match.str();
            chapter.titleLineLen = titleLen;
            m_chapters.emplace_back(chapter);
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}

bool BookText::isValidNumber(wchar_t* text, int len)
{
    static const wchar_t VALID_NUMBER[] =
    {
        L'0', L'1', L'2', L'3', L'4',
        L'5', L'6', L'7', L'8', L'9',
        L'零', L'一', L'二', L'三', L'四',
        L'五', L'六', L'七', L'八', L'九',
        L'十', L'百', L'千', L'万', L'亿',
        L'壹', L'贰', L'叁', L'肆',
        L'伍', L'陆', L'柒', L'捌', L'玖',
        L'拾', L'佰', L'仟', L'萬', L'億',
        L'两'
    };

    bool bFound = false;
    if (!text || len <= 0)
        return false;

    for (int i = 0; i < len; i++)
    {
        bFound = false;
        for (int j = 0; j < sizeof(VALID_NUMBER) / sizeof(VALID_NUMBER[0]); j++)
        {
            if (text[i] == VALID_NUMBER[j])
            {
                bFound = true;
                break;
            }
        }
        if (!bFound)
        {
            return false;
        }
    }
    return true;
}

bool BookText::isValidChapter(wchar_t text)
{
    static const wchar_t VALID_CHAPTER[] =
    {
        L'卷', L'章', L'部', L'节', L'集'
    };

    for (int i = 0; i < sizeof(VALID_CHAPTER) / sizeof(VALID_CHAPTER[0]); i++)
    {
        if (text == VALID_CHAPTER[i])
        {
            return true;
        }
    }
    return false;
}

bool BookText::isValidSeparator(wchar_t text)
{
    static const wchar_t VALID_SEPARATOR[] =
    {
        L'：', L':', L'-', L'、'
    };

    if (Utils::isBlank(text))
    {
        return true;
    }

    for (int i = 0; i < sizeof(VALID_SEPARATOR) / sizeof(VALID_SEPARATOR[0]); i++)
    {
        if (text == VALID_SEPARATOR[i])
        {
            return true;
        }
    }
    return false;
}

bool BookText::moveChapterIndex(int index, int delta)
{
    for (auto& i : m_chapters)
    {
        if (i.index > index)
        {
            i.index += delta;
            if (i.index < 0) i.index = 0;
            else if (i.index >= m_length) i.index = m_length - 1;
        }
    }
    return true;
}