/**
 * @file BookMobi.c
 * @brief MOBI book parser implementation.
 *
 * Design Concept:
 * Parses MOBI/AZW3 files by leveraging the 'libmobi' library to extract and
 * reconstruct raw MOBI data into standard EPUB-like OEBPS structures (e.g., OPF,
 * NCX, HTML, and assets), and then delegating the underlying parsing flow
 * to the BookEpub base class.
 *
 * Original Author: thomascatlee (https://github.com/thomascatlee)
 * Current  Author: binbyu       (https://github.com/binbyu)
 */

#include "BookMobi.h"
#include "conv.h"

BookMobi::BookMobi(const std::wstring& fileName)
    : BookEpub(fileName)
{
}

BookMobi::~BookMobi()
{
}

BookType BookMobi::type() const
{
    return BookType::MOBI;
}

bool BookMobi::loadBook(Files& files)
{
    freeBook(files);

    m_epub = new Mobi();
    if (!m_epub) return false;
    Mobi* m_mobi = mobi();
    if (!m_mobi) return false;

    // Open & load mobi file
    FILE* fp = _wfopen(m_fileName.c_str(), L"rb");
    if (!fp) return false;

    m_mobi->m = mobi_init();
    if (!m_mobi->m)
    {
        fclose(fp);
        return false;
    }

    MOBI_RET ret = mobi_load_file(m_mobi->m, fp);
    fclose(fp);
    if (MOBI_SUCCESS != ret)
    {
        return false;
    }

    /* Parse rawml text and other data held in MOBIData structure into MOBIRawml structure */
    m_mobi->rawml = mobi_init_rawml(m_mobi->m);
    if (!m_mobi->rawml)
    {
        return false;
    }

    ret = mobi_parse_rawml(m_mobi->rawml, m_mobi->m);
    if (MOBI_SUCCESS != ret)
    {
        return false;
    }

    /* Get file list */
    char partname[FILENAME_MAX];
    FileData fdata;

    if (m_mobi->rawml->markup != NULL)
    {
        /* Linked list of MOBIPart structures in rawml->markup holds main text files */
        MOBIPart *curr = m_mobi->rawml->markup;
        while (curr != NULL)
        {
            MOBIFileMeta file_meta = mobi_get_filemeta_by_type(curr->type);
            snprintf(partname, sizeof(partname), "part%05zu.%s", curr->uid, file_meta.extension);

            // save to file map
            fdata.data = (char*)curr->data;
            fdata.size = (int)curr->size;
            fdata.ownData = false; // need not free
            files.insert(std::make_pair(partname, fdata));

            curr = curr->next;
            if (m_abort) return false;
        }
    }
    if (m_mobi->rawml->resources != NULL)
    {
        /* Linked list of MOBIPart structures in rawml->resources holds binary files, also opf files */
        MOBIPart *curr = m_mobi->rawml->resources;
        /* jpg, gif, png, bmp, font, audio, video also opf, ncx */
        while (curr != NULL)
        {
            MOBIFileMeta file_meta = mobi_get_filemeta_by_type(curr->type);
            if (curr->size > 0) {
                int n = snprintf(partname, sizeof(partname), "resource%05zu.%s", curr->uid, file_meta.extension);
                if (n < 0)
                {
                    return false;
                }
                if ((size_t) n >= sizeof(partname))
                {
                    return false;
                }
 
                // save to file map
                fdata.data = (char*)curr->data;
                fdata.size = (int)curr->size;
                fdata.ownData = false; // need not free
                files.insert(std::make_pair(partname, fdata));

                if (file_meta.type == T_OPF) m_mobi->opf = partname;
                if (file_meta.type == T_NCX) m_mobi->ncx = partname;
            }
            curr = curr->next;
            if (m_abort) return false;
        }
    }
    return !files.empty();
}

bool BookMobi::parseOcf(Files& files)
{
    return true;
}

bool BookMobi::parseCover(Files& files)
{
    if (m_abort) return false;
    Mobi* m_mobi = mobi();
    if (!m_mobi) return false;
    freeCover();

    MOBIPdbRecord *record = NULL;
    MOBIExthHeader *exth = mobi_get_exthrecord_by_tag(m_mobi->m, EXTH_COVEROFFSET);
    if (exth)
    {
        uint32_t offset = mobi_decode_exthvalue((unsigned char*)exth->data, exth->size);
        size_t first_resource = mobi_get_first_resource_record(m_mobi->m);
        size_t uid = first_resource + offset;
        record = mobi_get_record_by_seqnumber(m_mobi->m, uid);
        if (!record || record->size <= 0) return true;

        m_cover = (Cover*)malloc(sizeof(Cover));
        m_cover->data = (char*)malloc(record->size);
        m_cover->size = (int)record->size;
        memcpy(m_cover->data, record->data, record->size);
    }
    return true;
}

Mobi* BookMobi::mobi() const
{
    return dynamic_cast<Mobi*>(m_epub);
}