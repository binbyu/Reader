#include "stdafx.h"
#include "TextBook.h"
#include "types.h"


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

bool TextBook::ParserBook(void)
{
    bool ret = false;

    if (!ReadBook())
        goto end;

    if (!ParserChapters())
        goto end;

    ret = true;

end:
    if (!ret)
        CloseBook();

    return ret;
}

bool TextBook::ReadBook(void)
{
    FILE *fp = NULL;
    char *buf = NULL;
    int len;
    int size;
    bool ret = false;

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

        buf = (char *)malloc(len + 1);
        buf[len] = 0;
        if (!buf)
            goto end;
        size = fread(buf, 1, len, fp);
        if (size != len)
            goto end;
    }
    else
    {
        goto end;
    }

    if (!DecodeText(buf, len, &m_Text, &m_TextLength))
        goto end;

    if (m_bForceKill)
        goto end;

    ret = true;

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

bool TextBook::ParserChapters(void)
{
    wchar_t *text = m_Text;
    wchar_t title[MAX_CHAPTER_LENGTH] = { 0 };
    int line_size;
    int title_len = 0;
    bool bFound = false;
    int idx_1 = -1, idx_2 = -1;
    chapter_item_t chapter;
    int menu_begin_id = IDM_CHAPTER_BEGIN;

    while (true)
    {
        if (m_bForceKill)
        {
            return false;
        }

        if (!GetLine(text, m_TextLength - (text - m_Text), &line_size))
        {
            break;
        }

        // check format
        bFound = false;
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
                    || line_size <= i + 1)
                    || text[i + 1] == _T('£º'))
            {
                if (text[i] == _T('¾í')
                    || text[i] == _T('ÕÂ')
                    || text[i] == _T('²¿')
                    || text[i] == _T('½Ú'))
                {
                    idx_2 = i;
                    bFound = true;
                    break;
                }
            }
            if (idx_1 == -1 && line_size > i + 2 && text[i] == _T('Ð¨') && text[i+1] == _T('×Ó')
                && ((text[i + 2] == _T(' ')
                || text[i + 2] == _T('\t'))
                || text[i + 2] == 0x3000 // Full Angle space
                || line_size <= i + 1))
            {
                idx_1 = i;
                idx_2 = line_size - 1;
                bFound = true;
                break;
            }
        }
        if (bFound && (text[idx_1] == _T('Ð¨') || (IsChapter(text + idx_1 + 1, idx_2 - idx_1 - 1))))
        {
            title_len = line_size - idx_1 < (MAX_CHAPTER_LENGTH - 1) ? line_size - idx_1 : MAX_CHAPTER_LENGTH - 1;
            memcpy(title, text + idx_1, title_len * sizeof(wchar_t));
            title[title_len] = 0;

            chapter.index = /*idx_1 +*/ (text - m_Text);
            chapter.title = title;
            m_Chapters.insert(std::make_pair(menu_begin_id++, chapter));
        }

        // set index
        text += line_size + 1; // add 0x0a
    }
    return true;
}

bool TextBook::GetLine(wchar_t* text, int len, int* line_size)
{
    if (!text || len <= 0)
        return false;

    for (int i = 0; i < len; i++)
    {
        if (text[i] == 0x0A)
        {
            *line_size = i;
            return true;
        }
    }
    *line_size = len;
    return true;
}

bool TextBook::IsChapter(wchar_t* text, int len)
{
    bool bFound = false;
    if (!text || len <= 0)
        return false;

    for (int i = 0; i < len; i++)
    {
        bFound = false;
        for (int j = 0; j < sizeof(m_ValidChapter)/sizeof(m_ValidChapter[0]); j++)
        {
            if (text[i] == m_ValidChapter[j])
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