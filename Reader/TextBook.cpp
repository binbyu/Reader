#include "TextBook.h"
#include "types.h"
#include <regex>


wchar_t TextBook::m_ValidChapter[] =
{
    _T(' '), _T('\t'),
    _T('0'), _T('1'), _T('2'), _T('3'), _T('4'),
    _T('5'), _T('6'), _T('7'), _T('8'), _T('9'),
    _T('Áã'), _T('Ò»'), _T('¶þ'), _T('Èý'), _T('ËÄ'),
    _T('Îå'), _T('Áù'), _T('Æß'), _T('°Ë'), _T('¾Å'),
    _T('Ê®'), _T('°Ù'), _T('Ç§'), _T('Íò'), _T('ÒÚ'),
    _T('Ò¼'), _T('·¡'), _T('Èþ'), _T('ËÁ'),
    _T('Îé'), _T('Â½'), _T('Æâ'), _T('°Æ'), _T('¾Á'),
    _T('Ê°'), _T('°Û'), _T('Çª'), _T('Èf'), _T('ƒ|'),
    _T('Á½'),
    0x3000
};

TextBook::TextBook()
{
}

TextBook::~TextBook()
{
    ForceKill();
}

book_type_t TextBook::GetBookType(void)
{
    return book_text;
}

BOOL TextBook::SaveBook(HWND hWnd)
{
    FILE *fp = NULL;

    fp = _tfopen(m_fileName, _T("wb"));
    if (!fp)
        return FALSE;

    fwrite("\xff\xfe", 2, 1, fp);
    fwrite(m_Text, sizeof(wchar_t), m_Length, fp);
    fclose(fp);

    return TRUE;
}

BOOL TextBook::UpdateChapters(int offset)
{
    chapters_t::iterator itor;
    
    for (itor=m_Chapters.begin(); itor!=m_Chapters.end(); itor++)
    {
        if (itor->index > m_Index)
        {
            itor->index += offset;
            if (itor->index < 0)
                itor->index = 0;
        }
    }
    return TRUE;
}

BOOL TextBook::ParserBook(HWND hWnd)
{
    BOOL ret = FALSE;

    if (!ReadBook())
        goto end;

    if (!ParserChapters())
        goto end;

    ret = TRUE;

end:
    if (!ret)
        CloseBook();

    return ret;
}

BOOL TextBook::ReadBook(void)
{
    FILE *fp = NULL;
    char *buf = NULL;
    int len;
    int size;
    BOOL ret = FALSE;

    if (m_Data && m_Size > 0)
    {
        buf = m_Data;
        len = m_Size;
    }
    else if (m_fileName[0])
    {
        fp = _tfopen(m_fileName, _T("rb"));
        if (!fp)
            goto end;

        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        buf = (char *)malloc(len + 2);
        buf[len] = 0;
        buf[len+1] = 0;
        if (!buf)
            goto end;
        size = (int)fread(buf, 1, len, fp);
        if (size != len)
            goto end;
    }
    else
    {
        goto end;
    }

    if (!DecodeText(buf, len, &m_Text, &m_Length))
        goto end;

    if (m_bForceKill)
        goto end;

    ret = TRUE;

end:
    if (fp)
        fclose(fp);
    if (buf)
        free(buf);
    if (m_Data)
        m_Data = NULL;
    m_Size = 0;

    return ret;
}

BOOL TextBook::ParserChapters(void)
{
    if (m_Rule)
    {
        m_Chapters.clear();
        if (m_Rule->rule == 0)
        {
            return ParserChaptersDefault();
        }
        else if (m_Rule->rule == 1)
        {
            return ParserChaptersKeyword();
        }
        else if (m_Rule->rule == 2)
        {
            return ParserChaptersRegex();
        }
    }
    return FALSE;
}

BOOL TextBook::ParserChaptersDefault(void)
{
    wchar_t *text = m_Text;
    wchar_t title[MAX_CHAPTER_LENGTH] = { 0 };
    int line_size;
    int title_len = 0;
    BOOL bFound = FALSE;
    int idx_1 = -1, idx_2 = -1;
    chapter_item_t chapter;

    while (TRUE)
    {
        if (m_bForceKill)
        {
            return FALSE;
        }

        if (!GetLine(text, m_Length - (int)(text - m_Text), &line_size, NULL, NULL, NULL, NULL))
        {
            break;
        }

        // check format
        bFound = FALSE;
        idx_1 = -1;
        idx_2 = -1;
        for (int i = 0; i < line_size; i++)
        {
            if (text[i] == _T('µÚ'))
            {
                idx_1 = i;
            }
            if (idx_1 > -1
                && ((line_size > i + 1 && text[i + 1] == _T(' ')
                    || text[i + 1] == _T('\t'))
                    || text[i + 1] == 0x3000 // Full Angle space
                    || text[i + 1] == 0xA0 // Full Angle space
                    || line_size <= i + 1)
                    || text[i + 1] == _T('£º')
                    || text[i + 1] == _T(':'))
            {
                if (text[i] == _T('¾í')
                    || text[i] == _T('ÕÂ')
                    || text[i] == _T('²¿')
                    || text[i] == _T('½Ú'))
                {
                    idx_2 = i;
                    bFound = TRUE;
                    break;
                }
            }
            if (idx_1 == -1 && line_size > i + 2 && text[i] == _T('Ð¨') && text[i + 1] == _T('×Ó')
                && ((text[i + 2] == _T(' ')
                || text[i + 2] == _T('\t'))
                || text[i + 2] == 0x3000 // Full Angle space
                || line_size <= i + 1))
            {
                idx_1 = i;
                idx_2 = line_size - 1;
                bFound = TRUE;
                break;
            }
            if (idx_1 == -1 && line_size > i + 2 && text[i] == _T('Ðò') && text[i + 1] == _T('ÕÂ')
                && ((text[i + 2] == _T(' ')
                    || text[i + 2] == _T('\t'))
                    || text[i + 2] == 0x3000 // Full Angle space
                    || line_size <= i + 1))
            {
                idx_1 = i;
                idx_2 = line_size - 1;
                bFound = TRUE;
                break;
            }
        }
        if (bFound && (text[idx_1] == _T('Ð¨') || text[idx_1] == _T('Ðò') || (IsChapter(text + idx_1 + 1, idx_2 - idx_1 - 1))))
        {
            title_len = line_size - idx_1 < (MAX_CHAPTER_LENGTH - 1) ? line_size - idx_1 : MAX_CHAPTER_LENGTH - 1;
            memcpy(title, text + idx_1, title_len * sizeof(wchar_t));
            title[title_len] = 0;

            chapter.index = /*idx_1 +*/ (int)(text - m_Text);
            chapter.title = title;
            chapter.title_len = title_len;
            m_Chapters.push_back(chapter);
        }

        // set index
        text += line_size + 1; // add 0x0a
    }

    return TRUE;
}

BOOL TextBook::ParserChaptersKeyword(void)
{
    wchar_t *text = m_Text;
    wchar_t title[MAX_CHAPTER_LENGTH] = { 0 };
    int line_size;
    int title_len = 0;
    BOOL bFound = FALSE;
    int idx_1 = -1;
    int cmplen;
    chapter_item_t chapter;

    while (TRUE)
    {
        if (m_bForceKill)
        {
            return FALSE;
        }

        if (!GetLine(text, m_Length - (int)(text - m_Text), &line_size, NULL, NULL, NULL, NULL))
        {
            break;
        }

        // check format
        cmplen = (int)wcslen(m_Rule->keyword);
        if (cmplen <= line_size)
        {
            bFound = FALSE;
            idx_1 = -1;
            for (int i = 0; i < line_size; i++)
            {
                if (wcsncmp(text+i, m_Rule->keyword, cmplen) == 0)
                {
                    idx_1 = i;
                    bFound = TRUE;
                    break;
                }
            }
            if (bFound)
            {
                title_len = line_size - idx_1 < (MAX_CHAPTER_LENGTH - 1) ? line_size - idx_1 : MAX_CHAPTER_LENGTH - 1;
                memcpy(title, text + idx_1, title_len * sizeof(wchar_t));
                title[title_len] = 0;

                chapter.index = /*idx_1 +*/ (int)(text - m_Text);
                chapter.title = title;
                chapter.title_len = title_len;
                m_Chapters.push_back(chapter);
            }
        }

        // set index
        text += line_size + 1; // add 0x0a
    }
    return TRUE;
}

BOOL TextBook::ParserChaptersRegex(void)
{
    wchar_t title[MAX_CHAPTER_LENGTH] = { 0 };
    int title_len = 0;
    chapter_item_t chapter;
    int offset = 0;
    std::wcmatch cm;
    std::wregex *e = NULL;
    TCHAR *text = m_Text;

    try
    {
        e = new std::wregex(m_Rule->regex);    
    }
    catch (...)
    {
        if (e)
        {
            delete e;
        }
        return FALSE;
    }

    while (std::regex_search(text, cm, *e, std::regex_constants::format_first_only))
    {
        if (m_bForceKill)
        {
            break;
        }

        title_len = (int)cm.length() < (MAX_CHAPTER_LENGTH - 1) ? (int)cm.length() : MAX_CHAPTER_LENGTH - 1;
        memcpy(title, cm.str().c_str(), title_len * sizeof(wchar_t));
        title[title_len] = 0;

        chapter.index = offset + (int)cm.position();
        chapter.title = title;
        chapter.title_len = title_len;
        m_Chapters.push_back(chapter);


        text += cm.position() + cm.length();
        offset += (int)cm.position() + (int)cm.length();
    }
    if (e)
    {
        delete e;
    }

    return TRUE;
}

BOOL TextBook::IsChapter(wchar_t* text, int len)
{
    BOOL bFound = FALSE;
    if (!text || len <= 0)
        return FALSE;

    for (int i = 0; i < len; i++)
    {
        bFound = FALSE;
        for (int j = 0; j < sizeof(m_ValidChapter)/sizeof(m_ValidChapter[0]); j++)
        {
            if (text[i] == m_ValidChapter[j])
            {
                bFound = TRUE;
                break;
            }
        }
        if (!bFound)
        {
            return FALSE;
        }
    }
    return TRUE;
}