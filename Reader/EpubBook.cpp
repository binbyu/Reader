#include "stdafx.h"
#include "EpubBook.h"
#include "Utils.h"
#include "unzip.h"
#include "iowin32.h"
#include "libxml/xmlreader.h"
#include "libxml/HTMLparser.h"
#include "libxml/xpath.h"
#include <process.h>
#include <shlwapi.h>


#ifdef _DEBUG
#pragma comment(lib, "zlibstatd.lib")
#pragma comment(lib, "libxml2d.lib")
#else
#pragma comment(lib, "zlibstat.lib")
#pragma comment(lib, "libxml2.lib")
#endif

EpubBook::EpubBook()
    : m_Cover(NULL)
{
    xmlInitParser();
}

EpubBook::~EpubBook()
{
    FreeFilelist();
    xmlCleanupParser();
}

book_type_t EpubBook::GetBookType()
{
    return book_epub;
}

Bitmap * EpubBook::GetCoverImage(void)
{
    return m_Cover;
}

bool EpubBook::ParserBook(void)
{
    bool ret = false;
    epub_t epub;
    mainfest_t::iterator itor;
    navmap_t::iterator it;

    // unzip epub file
    if (!UnzipBook())
        goto end;

    // parser epub file
    epub.ocf = "META-INF/container.xml";
    if (!ParserOcf(epub))
        goto end;

    if (!ParserOpf(epub))
        goto end;

    if (!ParserNcx(epub))
        goto end;

    // parser epub cover image
    ParserCover(epub);

    // Parser epub chapters & text
    if (!ParserChapters(epub))
        goto end;

    ret = true;

end:
    // free epub
    for (itor = epub.mainfest.begin(); itor != epub.mainfest.end(); itor++)
    {
        delete itor->second;
    }
    for (it = epub.navmap.begin(); it != epub.navmap.end(); it++)
    {
        delete it->second;
    }
    FreeFilelist();
    if (!ret)
        CloseBook();

    return ret;
}

void EpubBook::FreeFilelist(void)
{
    filelist_t::iterator itor;
    for (itor = m_flist.begin(); itor != m_flist.end(); itor++)
    {
        free(itor->second.data);
    }
    m_flist.clear();
}

bool EpubBook::UnzipBook(void)
{
    unzFile uf = NULL;
    zlib_filefunc64_def ffunc;
    uLong i;
    unz_global_info64 gi = {0};
    int err = UNZ_ERRNO;
    unz_file_info64 file_info;
    char filename_inzip[256] = {0};
    char *buf = NULL;
    file_data_t fdata;

    fill_win32_filefunc64W(&ffunc);
    uf = unzOpen2_64(m_fileName, &ffunc);
    if (!uf)
        goto end;

    err = unzGetGlobalInfo64(uf, &gi);
    if (err != UNZ_OK)
        goto end;

    FreeFilelist();

    for (i = 0; i < gi.number_entry; i++)
    {
        // current file
        {
            err = unzGetCurrentFileInfo64(uf, &file_info, filename_inzip, sizeof(filename_inzip), NULL, 0, NULL, 0);
            if (err != UNZ_OK)
                goto end;

            if (filename_inzip[file_info.size_filename - 1] == '\\' || filename_inzip[file_info.size_filename - 1] == '/')
            {
                // is directory
            }
            else
            {
                // check password
                err = unzOpenCurrentFilePassword(uf, NULL);
                if (err != UNZ_OK)
                    goto end;

                // create memory to save this file
                buf = (char *)malloc((size_t)file_info.uncompressed_size);
                if (!buf)
                    goto end;

                // read file to memory
                err = unzReadCurrentFile(uf, buf, (size_t)file_info.uncompressed_size);
                if (err != file_info.uncompressed_size)
                    goto end;
                err = UNZ_OK;

                // save to file map
                fdata.data = buf;
                fdata.size = (size_t)file_info.uncompressed_size;
                m_flist.insert(std::make_pair(filename_inzip, fdata));
                buf = NULL;
            }
            unzCloseCurrentFile(uf);
        }

        // next file
        if ((i + 1) < gi.number_entry)
        {
            err = unzGoToNextFile(uf);
            if (err != UNZ_OK)
                goto end;
        }
    }

end:
    if (uf)
    {
        unzCloseCurrentFile(uf);
        unzClose(uf);
    }
    if (buf)
        free(buf);

    if (err != UNZ_OK)
        FreeFilelist();
    return err == UNZ_OK;
}

bool EpubBook::ParserOcf(epub_t &epub)
{
    filelist_t::iterator itor;
    xmlDocPtr doc = NULL;
    const xmlChar *xpath = NULL;
    xmlXPathContextPtr xpathctx = NULL;
    xmlXPathObjectPtr xpathobj = NULL;
    xmlNodeSetPtr nodeset;
    xmlChar *keyword;
    int i;
    bool ret = false;

    itor = m_flist.find(epub.ocf);
    if (itor == m_flist.end())
        goto end;

    doc = xmlReadMemory((const char *)itor->second.data, itor->second.size, NULL, NULL, XML_PARSE_NOBLANKS);
    if (!doc)
        goto end;

    xpathctx = xmlXPathNewContext(doc);
    if (!xpathctx)
        goto end;

    xpath = BAD_CAST("//*[local-name()='rootfile']/@full-path");
    xpathobj = xmlXPathEvalExpression(xpath, xpathctx);
    if (!xpathobj)
        goto end;

    if (xmlXPathNodeSetIsEmpty(xpathobj->nodesetval))
        goto end;

    nodeset = xpathobj->nodesetval;
    for (i = 0; i < nodeset->nodeNr; i++)
    {
        keyword = xmlNodeGetContent(nodeset->nodeTab[i]);

        if (keyword && keyword[0])
        {
            epub.opf = (const char *)keyword;
            if (epub.opf.find('/'))
            {
                epub.path = epub.opf.substr(0, epub.opf.rfind('/') + 1);
            }
            ret = true;
            xmlFree(keyword);
            break;
        }
        if (keyword)
            xmlFree(keyword);
    }

end:
    if (xpathobj)
        xmlXPathFreeObject(xpathobj);
    if (xpathctx)
        xmlXPathFreeContext(xpathctx);
    if (doc)
        xmlFreeDoc(doc);

    return ret;
}

bool EpubBook::ParserOpf(epub_t &epub)
{
    filelist_t::iterator itor;
    xmlDocPtr doc = NULL;
    const xmlChar *xpath = NULL;
    xmlXPathContextPtr xpathctx = NULL;
    xmlXPathObjectPtr xpathobj = NULL;
    xmlNodeSetPtr nodeset;
    xmlChar *keyword;
    mainfest_item_t *item = NULL;
    mainfest_t::iterator it;
    int i,j;
    bool ret = false;

    itor = m_flist.find(epub.opf);
    if (itor == m_flist.end())
        goto end;

    doc = xmlReadMemory((const char *)itor->second.data, itor->second.size, NULL, NULL, XML_PARSE_NOBLANKS);
    if (!doc)
        goto end;

    xpathctx = xmlXPathNewContext(doc);
    if (!xpathctx)
        goto end;

    // parser mainfest
    {
        xpath = BAD_CAST("//*[local-name()='item']/@*[name()='id' or name()='href' or name()='media-type']");
        xpathobj = xmlXPathEvalExpression(xpath, xpathctx);
        if (!xpathobj)
            goto end;

        if (xmlXPathNodeSetIsEmpty(xpathobj->nodesetval))
            goto end;

        nodeset = xpathobj->nodesetval;
        for (i = 0; i < nodeset->nodeNr; /*i++*/)
        {
            item = new mainfest_item_t;
            if (!item)
                goto end;
            for (j = 0; j < 3 && i < nodeset->nodeNr; j++, i++)
            {
                keyword = xmlNodeGetContent(nodeset->nodeTab[i]);
                if (keyword && keyword[0])
                {
                    if (xmlStrcmp(nodeset->nodeTab[i]->name, (const xmlChar *)"id") == 0)
                    {
                        item->id = (const char *)keyword;
                    }
                    else if (xmlStrcmp(nodeset->nodeTab[i]->name, (const xmlChar *)"href") == 0)
                    {
                        item->href = (const char *)keyword;
                    }
                    else if (xmlStrcmp(nodeset->nodeTab[i]->name, (const xmlChar *)"media-type") == 0)
                    {
                        item->media_type = (const char *)keyword;
                    }
                }
                if (keyword)
                    xmlFree(keyword);
            }
            if (!item->id.empty() && !item->href.empty() && !item->media_type.empty())
            {
                epub.mainfest.insert(std::make_pair(item->id, item));
            }
            else
            {
                free(item);
                goto end;
            }
        }

        if (epub.mainfest.empty())
            goto end;

        if (xpathobj)
            xmlXPathFreeObject(xpathobj);
    }
    

    // parser spine
    {
        xpath = BAD_CAST("//*[local-name()='itemref']/@idref");
        xpathobj = xmlXPathEvalExpression(xpath, xpathctx);
        if (!xpathobj)
            goto end;

        if (xmlXPathNodeSetIsEmpty(xpathobj->nodesetval))
            goto end;

        nodeset = xpathobj->nodesetval;
        for (i = 0; i < nodeset->nodeNr; i++)
        {
            keyword = xmlNodeGetContent(nodeset->nodeTab[i]);
            if (keyword && keyword[0])
            {
                if (epub.mainfest.find((const char *)keyword) == epub.mainfest.end())
                {
                    xmlFree(keyword);
                    goto end;
                }
                epub.spine.push_back((const char *)keyword);
            }
            if (keyword)
                xmlFree(keyword);
        }

        if (epub.spine.empty())
            goto end;

        if (xpathobj)
            xmlXPathFreeObject(xpathobj);
    }

    ret = true;

    // parser ncx, ncx is not a required file
    {
        xpath = BAD_CAST("//*[local-name()='spine']/@toc");
        xpathobj = xmlXPathEvalExpression(xpath, xpathctx);
        if (!xpathobj)
            goto end;

        if (xmlXPathNodeSetIsEmpty(xpathobj->nodesetval))
            goto end;

        nodeset = xpathobj->nodesetval;
        for (i = 0; i < nodeset->nodeNr; i++)
        {
            keyword = xmlNodeGetContent(nodeset->nodeTab[i]);
            if (keyword && keyword[0])
            {
                it = epub.mainfest.find((const char *)keyword);
                if (it == epub.mainfest.end())
                {
                    xmlFree(keyword);
                    epub.ncx = "";
                    break;
                }
                epub.ncx = it->second->href;
                break;
            }
            if (keyword)
                xmlFree(keyword);
        }
    }

end:
    if (xpathobj)
        xmlXPathFreeObject(xpathobj);
    if (xpathctx)
        xmlXPathFreeContext(xpathctx);
    if (doc)
        xmlFreeDoc(doc);

    return ret;
}

bool EpubBook::ParserNcx(epub_t &epub)
{
    filelist_t::iterator itor;
    xmlDocPtr doc = NULL;
    xmlNodePtr node;
    xmlChar *order, *id, *text, *src;
    const xmlChar *xpath = NULL;
    xmlXPathContextPtr xpathctx = NULL;
    xmlXPathObjectPtr xpathobj = NULL;
    xmlNodeSetPtr nodeset;
    navpoint_t *navp = NULL;
    int i;
    bool ret = false;

    // not exist ncx
    if (epub.ncx.empty())
        return true;

    itor = m_flist.find(epub.path+epub.ncx);
    if (itor == m_flist.end())
        goto end;

    doc = xmlReadMemory((const char *)itor->second.data, itor->second.size, NULL, NULL, XML_PARSE_NOBLANKS);
    if (!doc)
        goto end;

    xpathctx = xmlXPathNewContext(doc);
    if (!xpathctx)
        goto end;

    xpath = BAD_CAST("//*[local-name()='navPoint']");
    xpathobj = xmlXPathEvalExpression(xpath, xpathctx);
    if (!xpathobj)
        goto end;

    if (xmlXPathNodeSetIsEmpty(xpathobj->nodesetval))
        goto end;

    nodeset = xpathobj->nodesetval;
    for (i = 0; i < nodeset->nodeNr; i++)
    {
        node = nodeset->nodeTab[i];
        id = xmlGetProp(node, BAD_CAST"id");
        order = xmlGetProp(node, BAD_CAST"playOrder");
        text = xmlNodeGetContent(node);
        node = node->children;
        while (node)
        {
            if (xmlStrcasecmp(node->name, BAD_CAST"content") == 0)
            {
                src = xmlGetProp(node, BAD_CAST"src");
                break;
            }
            node = node->next;
        }

        if (!id || !order || !text || !src
            || !id[0] || !order[0] || !text[0] || !src[0])
        {
            if (id)
                xmlFree(id);
            if (order)
                xmlFree(order);
            if (text)
                xmlFree(text);
            if (src)
                xmlFree(src);
            goto end;
        }

        navp = new navpoint_t;
        navp->id = (const char *)id;
        navp->order = atoi((const char *)order);
        navp->text = (const char *)text;
        navp->src = (const char *)src;
        epub.navmap.insert(std::make_pair(navp->id, navp));

        xmlFree(id);
        xmlFree(order);
        xmlFree(text);
        xmlFree(src);
    }

    if (!epub.navmap.empty())
    {
        ret = true;
    }

end:
    if (xpathobj)
        xmlXPathFreeObject(xpathobj);
    if (xpathctx)
        xmlXPathFreeContext(xpathctx);
    if (doc)
        xmlFreeDoc(doc);
    return ret;
}

bool EpubBook::ParserOps(file_data_t *fdata, wchar_t **text, int *len, wchar_t **title, int *tlen, bool parsertitle)
{
    filelist_t::iterator itor;
    xmlDocPtr doc = NULL;
    xmlChar *value;
    const xmlChar *xpath = NULL;
    xmlXPathContextPtr xpathctx = NULL;
    xmlXPathObjectPtr xpathobj = NULL;
    xmlNodeSetPtr nodeset;
    int i;
    bool ret = false;
    
    doc = xmlReadMemory((const char *)fdata->data, fdata->size, NULL, NULL, 0/*XML_PARSE_NOBLANKS*/);
    if (!doc)
        goto end;

    xpathctx = xmlXPathNewContext(doc);
    if (!xpathctx)
        goto end;

    if (parsertitle)
    {
        // parser title
        xpath = BAD_CAST("//*[local-name()='title']");
        xpathobj = xmlXPathEvalExpression(xpath, xpathctx);
        if (!xpathobj)
            goto body;

        if (xmlXPathNodeSetIsEmpty(xpathobj->nodesetval))
            goto body;

        nodeset = xpathobj->nodesetval;
        for (i = 0; i < nodeset->nodeNr; i++)
        {
            value = xmlNodeGetContent(nodeset->nodeTab[i]);

            if (!DecodeText((const char *)value, strlen((const char *)value), title, tlen))
            {
                xmlFree(value);
                break;
            }
            xmlFree(value);
            break;
        }
    }

body:
    if (xpathobj)
        xmlXPathFreeObject(xpathobj);
    {
        // parser body
        xpath = BAD_CAST("//*[local-name()='body']");
        xpathobj = xmlXPathEvalExpression(xpath, xpathctx);
        if (!xpathobj)
            goto end;

        if (xmlXPathNodeSetIsEmpty(xpathobj->nodesetval))
            goto end;

        nodeset = xpathobj->nodesetval;
        for (i = 0; i < nodeset->nodeNr; i++)
        {
            value = xmlNodeGetContent(nodeset->nodeTab[i]);

            if (!DecodeText((const char *)value, strlen((const char *)value), text, len))
                ret = false;
            else
                ret = true;

            xmlFree(value);
            break;
        }
    }

end:
    if (xpathobj)
        xmlXPathFreeObject(xpathobj);
    if (xpathctx)
        xmlXPathFreeContext(xpathctx);
    if (doc)
        xmlFreeDoc(doc);
    return ret;
}

bool EpubBook::ParserChapters(epub_t &epub)
{
    typedef struct buffer_t
    {
        wchar_t *text;
        int len;
    } buffer_t;

    spine_t::iterator itspine;
    filelist_t::iterator itflist;
    mainfest_t::iterator itmfest;
    navmap_t::iterator itnav;
    file_data_t *fdata;
    chapter_item_t chapter;
    std::string filename;
    wchar_t *text = NULL;
    wchar_t *title = NULL;
    int len = 0, tlen = 0;
    int index = 0, i=0;
    buffer_t *buffer = NULL;

    m_TextLength = 1; // add one wchar_t '0x0a' new line for cover
    buffer = (buffer_t *)malloc(epub.spine.size() * sizeof(buffer_t));
    for (itspine = epub.spine.begin(); itspine != epub.spine.end(); itspine++)
    {
        itmfest = epub.mainfest.find(*itspine);
        if (itmfest != epub.mainfest.end())
        {
            filename = epub.path + itmfest->second->href;
            itflist = m_flist.find(filename);
            itnav = epub.navmap.find(itmfest->second->id);
            if (itflist != m_flist.end() /*&& itnav != epub.navmap.end()*/)
            {
                fdata = &(itflist->second);
                if (ParserOps(fdata, &text, &len, &title, &tlen, itnav == epub.navmap.end()))
                {
                    if (len > 0)
                    {
                        buffer[index].text = text;
                        buffer[index].len = len;
                        chapter.index = m_TextLength == 1 ? 0 : m_TextLength;
                        m_TextLength += len;
                        if (itnav != epub.navmap.end())
                        {
                            DecodeText(itnav->second->text.c_str(), itnav->second->text.size(), &title, &tlen);
                            chapter.title = title;
                        }
                        else
                        {
                            chapter.title = title;
                        }
                        if (title)
                            free(title);
                        m_Chapters.insert(std::make_pair(IDM_CHAPTER_BEGIN + index++, chapter));
                    }
                }
            }
        }
    }

    if (index > 0)
    {
        len = 1; // add one wchar_t '0x0a' new line for cover
        m_TextLength += len;
        m_Text = (wchar_t *)malloc(sizeof(wchar_t) * (m_TextLength + 1));
        m_Text[m_TextLength] = 0;
        m_Text[0] = 0x0A;
        for (i = 0; i < index; i++)
        {
            memcpy(m_Text + len, buffer[i].text, buffer[i].len * sizeof(wchar_t));
            len += buffer[i].len;
            free(buffer[i].text);
        }
    }
    free(buffer);
    return true;
}

bool EpubBook::ParserCover(epub_t &epub)
{
    filelist_t::iterator itflist;
    mainfest_t::iterator itmfest;
    file_data_t *fdata;
    IStream *pStream = NULL;

    if (m_Cover)
    {
        delete m_Cover;
        m_Cover = NULL;
    }

    itmfest = epub.mainfest.find("cover-image");
    if (itmfest != epub.mainfest.end())
    {
        itflist = m_flist.find(epub.path + itmfest->second->href);
        if (itflist != m_flist.end())
        {
            fdata = &(itflist->second);
            pStream = SHCreateMemStream((const BYTE *)fdata->data, fdata->size);
            m_Cover = new Bitmap(pStream);
            if (m_Cover)
            {
                if (Gdiplus::Ok != m_Cover->GetLastStatus())
                {
                    delete m_Cover;
                    m_Cover = NULL;
                }
            }
            pStream->Release();
        }
    }

    return m_Cover != NULL;
}