#include "MobiBook.h"
#include "Utils.h"
#include "types.h"
#include <regex>


#include <shlwapi.h>
#include <comdef.h>

#include "libxml/xmlreader.h"
#include "libxml/HTMLtree.h"
#include "libxml/HTMLparser.h"
#include "libxml/xpath.h"


/**
 @brief Messages for libmobi return codes
 For reference see enum MOBI_RET in mobi.h
 */
const char *libmobi_messages[] = {
    "Success",
    "Generic error",
    "Wrong function parameter",
    "Corrupted data",
    "File not found",
    "Document is encrypted",
    "Unsupported document format",
    "Memory allocation error",
    "Initialization error",
    "Buffer error",
    "XML error",
    "Invalid DRM pid",
    "DRM key not found",
    "DRM support not included",
    "Write failed",
    "DRM expired"
};

#define LIBMOBI_MSG_COUNT ARRAYSIZE(libmobi_messages)

/**
 @brief Return message for given libmobi return code
 @param[in] ret Libmobi return code
 @return Message string
 */
const char * libmobi_msg(const MOBI_RET ret) {
    size_t index = ret;
    if (index < LIBMOBI_MSG_COUNT) {
        return libmobi_messages[index];
    }
    return "Unknown error";
}

MobiBook::MobiBook()
    : m_Cover(NULL)
{
    /* Initialize main MOBIData structure */
    /* Must be deallocated with mobi_free() when not needed */
}

MobiBook::~MobiBook()
{
    ForceKill();
    FreeFilelist();
    if (m_Cover)
    {
        delete m_Cover;
    }
}

book_type_t MobiBook::GetBookType()
{
    return book_mobi;
}

BOOL MobiBook::SaveBook(HWND hWnd)
{
    return FALSE;
}

BOOL MobiBook::UpdateChapters(int offset)
{
    return FALSE;
}

BOOL MobiBook::ParserBook(HWND hWnd)
{
    BOOL ret = FALSE;

    MOBIData *m = mobi_init();
    MOBIRawml *rawml;
    mobi_t mobi;    
    
    manifests_t::iterator itor;
    navpoints_t::iterator it;

    if (m == NULL) {
        printf("Memory allocation failed\n");
        return FALSE;
    }    

    MOBI_RET mobi_ret = mobi_load_filename(m, _bstr_t(m_fileName));
    if (mobi_ret != MOBI_SUCCESS) {
        mobi_free(m);
        printf("Error loading file (%s)\n", libmobi_msg(mobi_ret));
        return FALSE;
    }

    rawml = mobi_init_rawml(m);
    if (rawml == NULL) {
        printf("Memory allocation failed\n");
        return FALSE;
    }    

    /* Parse rawml text and other data held in MOBIData structure into MOBIRawml structure */
    mobi_ret = mobi_parse_rawml(rawml, m);
    if (mobi_ret != MOBI_SUCCESS) {
        printf("Parsing rawml failed (%s)\n", libmobi_msg(mobi_ret));
        return FALSE;
    }



    // unzip mobi file
    if (!UnzipBook(rawml, m, mobi))
        goto end;

    mobi.path = "";
    // parser mobi file
    if (!ParserOpf(mobi))
        goto end;

    if (!ParserNcx(mobi))
        goto end;

    // parser mobi cover image
    ParserCover(mobi, m);

    // Parser mobi chapters & text


    if (!ParserChapters(mobi))
        goto end;


    ret = TRUE;

end:
    // free mobi
    for (itor = mobi.manifests.begin(); itor != mobi.manifests.end(); itor++)
    {
        delete itor->second;
    }
    mobi.manifests.clear();
    for (it = mobi.navpoints.begin(); it != mobi.navpoints.end(); it++)
    {
        delete it->second;
    }
    mobi.navpoints.clear();
    mobi.spines.clear();
    FreeFilelist();

    /* Free MOBIRawml structure */
    if (rawml != NULL)
    {
        mobi_free_rawml(rawml);
    }

    /* Free MOBIData structure */
    mobi_free(m);

    
    if (!ret)
    {
        if (m_Cover)
        {
            delete m_Cover;
            m_Cover = NULL;
        }
        CloseBook();
    }

    return ret;
}

Gdiplus::Bitmap* MobiBook::GetCover(void)
{
    return m_Cover;
}

int MobiBook::GetTextBeginIndex(void)
{
    return m_Cover ? 1 : 0;
}

void MobiBook::FreeFilelist(void)
{
    m_flist.clear();
}

BOOL MobiBook::UnzipBook(MOBIRawml *rawml, MOBIData *m, mobi_t &mobi)
{
    char partname[FILENAME_MAX];

    file_data_t fdata;

    FreeFilelist();

    
    if (rawml->markup != NULL) {
        /* Linked list of MOBIPart structures in rawml->markup holds main text files */
        MOBIPart *curr = rawml->markup;
        while (curr != NULL) {
            MOBIFileMeta file_meta = mobi_get_filemeta_by_type(curr->type);
            snprintf(partname, sizeof(partname), "part%05zu.%s", curr->uid, file_meta.extension);
            printf("%s\n", partname);
            
            // save to file map
            fdata.data = curr->data;
            fdata.size = curr->size;
            m_flist.insert(std::make_pair(partname, fdata));            
            
            curr = curr->next;
        }
    }
    if (rawml->flow != NULL) {
        /* Linked list of MOBIPart structures in rawml->flow holds supplementary text files */
        MOBIPart *curr = rawml->flow;
        /* skip raw html file */
        curr = curr->next;
        while (curr != NULL) {
            MOBIFileMeta file_meta = mobi_get_filemeta_by_type(curr->type);
            snprintf(partname, sizeof(partname), "flow%05zu.%s", curr->uid, file_meta.extension);
            printf("%s\n", partname);
            
            // save to file map
            fdata.data = curr->data;
            fdata.size = curr->size;
            m_flist.insert(std::make_pair(partname, fdata));                   
            
            curr = curr->next;
        }
    }
    if (rawml->resources != NULL) {
        /* Linked list of MOBIPart structures in rawml->resources holds binary files, also opf files */
        MOBIPart *curr = rawml->resources;
        /* jpg, gif, png, bmp, font, audio, video also opf, ncx */
        while (curr != NULL) {
            MOBIFileMeta file_meta = mobi_get_filemeta_by_type(curr->type);
            if (curr->size > 0) {
                int n = snprintf(partname, sizeof(partname), "resource%05zu.%s", curr->uid, file_meta.extension);
                if (n < 0) {
                    printf("Creating file name failed\n");
                    return FALSE;
                }
                if ((size_t) n >= sizeof(partname)) {
                    printf("File name too long: %s\n", partname);
                    return FALSE;
                }
                
                printf("%s\n", partname);
 
                // save to file map
                fdata.data = curr->data;
                fdata.size = curr->size;
                m_flist.insert(std::make_pair(partname, fdata));

                if (file_meta.type == T_OPF)
                {
                    mobi.opf = partname;
                }

                if (file_meta.type == T_NCX)
                {
                    mobi.ncx = partname;
                }

            }
            curr = curr->next;
        }
    }
    return TRUE;
}

BOOL MobiBook::ParserOpf(mobi_t &mobi)
{
    filelist_t::iterator itor;
    xmlDocPtr doc = NULL;
    const xmlChar *xpath = NULL;
    xmlXPathContextPtr xpathctx = NULL;
    xmlXPathObjectPtr xpathobj = NULL;
    xmlNodeSetPtr nodeset;
    xmlChar *keyword;
    manifest_t *item = NULL;
    manifests_t::iterator it;
    int i,j;
    BOOL ret = FALSE;
    char buff[1024];

    itor = m_flist.find(mobi.opf);
    if (itor == m_flist.end())
        goto end;

    doc = xmlReadMemory((const char *)itor->second.data, itor->second.size, NULL, NULL, XML_PARSE_RECOVER | XML_PARSE_NOBLANKS);
    if (!doc)
        goto end;

    if (m_bForceKill)
        goto end;

    xpathctx = xmlXPathNewContext(doc);
    if (!xpathctx)
        goto end;

    if (m_bForceKill)
        goto end;

    // parser manifest
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
            item = new manifest_t;
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
                        url_decode((const char *)keyword, buff);
                        item->href = (const char *)buff;
                    }
                    else if (xmlStrcmp(nodeset->nodeTab[i]->name, (const xmlChar *)"media-type") == 0)
                    {
                        item->media_type = (const char *)keyword;
                    }
                }
                if (keyword)
                    xmlFree(keyword);
                if (m_bForceKill)
                {
                    delete item;
                    goto end;
                }
            }
            if (!item->id.empty() && !item->href.empty() && !item->media_type.empty())
            {
                mobi.manifests.insert(std::make_pair(item->id, item));
            }
            else
            {
                delete item;
                goto end;
            }
        }

        if (mobi.manifests.empty())
            goto end;

        if (xpathobj)
            xmlXPathFreeObject(xpathobj);
    }
    
    if (m_bForceKill)
        goto end;

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
                if (mobi.manifests.find((const char *)keyword) == mobi.manifests.end())
                {
                    xmlFree(keyword);
                    goto end;
                }
                mobi.spines.push_back((const char *)keyword);
            }
            if (keyword)
                xmlFree(keyword);
            if (m_bForceKill)
                goto end;
        }

        if (mobi.spines.empty())
            goto end;

        if (xpathobj)
            xmlXPathFreeObject(xpathobj);
    }

    ret = TRUE;

    if (m_bForceKill)
    {
        ret = FALSE;
        goto end;
    }

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
                it = mobi.manifests.find((const char *)keyword);
                if (it == mobi.manifests.end())
                {
                    xmlFree(keyword);
                    mobi.ncx = "";
                    break;
                }
                mobi.ncx = it->second->href;
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

BOOL MobiBook::ParserNcx(mobi_t &mobi)
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
    BOOL ret = FALSE;
    int flag = 0;
    char buff[1024];
    int nav_cnt = 0;

    // not exist ncx
    if (mobi.ncx.empty())
        return TRUE;

    itor = m_flist.find(mobi.path+mobi.ncx);
    if (itor == m_flist.end())
        goto end;

    doc = xmlReadMemory((const char *)itor->second.data, itor->second.size, NULL, NULL, XML_PARSE_RECOVER | XML_PARSE_NOBLANKS);
    if (!doc)
        goto end;

    if (m_bForceKill)
        goto end;

    xpathctx = xmlXPathNewContext(doc);
    if (!xpathctx)
        goto end;

    if (m_bForceKill)
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
        src = NULL;
        text = NULL;
        node = nodeset->nodeTab[i];
        id = xmlGetProp(node, BAD_CAST"id");
        order = xmlGetProp(node, BAD_CAST"playOrder");
        //text = xmlNodeGetContent(node);
        node = node->children;
        flag = 0;
        while (node)
        {
            if (xmlStrcasecmp(node->name, BAD_CAST"content") == 0)
            {
                src = xmlGetProp(node, BAD_CAST"src");
                flag++;
            }
            if (xmlStrcasecmp(node->name, BAD_CAST"navLabel") == 0)
            {
                text = xmlNodeGetContent(node->children);
                flag++;
            }
            if (flag >= 2)
                break;
            node = node->next;
        }

#if 0 // do not check this
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
#endif

        navp = new navpoint_t;
        if (id)
            navp->id = (const char *)id;
        if (order)
            navp->order = atoi((const char *)order);
        if (text)
            navp->text = (const char *)text;
        if (src)
        {
            url_decode((const char *)src, buff);
            navp->src = (const char *)buff;
        }
        mobi.navpoints.insert(std::make_pair(navp->src, navp));
        nav_cnt++;

        if (id)
            xmlFree(id);
        if (order)
            xmlFree(order);
        if (text)
            xmlFree(text);
        if (src)
            xmlFree(src);
        if (m_bForceKill)
            goto end;
    }

    if (!mobi.navpoints.empty())
    {
        ret = TRUE;
    }

    if (nav_cnt != mobi.navpoints.size())   //意味着src出现了重复，目录不可靠
    {
        mobi.navpoints.clear();
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

BOOL MobiBook::ParserOps(file_data_t *fdata, wchar_t **text, int *len, wchar_t **title, int *tlen, BOOL parsertitle)
{
    filelist_t::iterator itor;
    xmlDocPtr doc = NULL;
    xmlChar *value;
    const xmlChar *xpath = NULL;
    xmlXPathContextPtr xpathctx = NULL;
    xmlXPathObjectPtr xpathobj = NULL;
    xmlNodeSetPtr nodeset;
    int i;
    BOOL ret = FALSE;
    xmlChar *format_str = NULL;
    int size;

    xmlKeepBlanksDefault(0);
    doc = htmlReadMemory((const char *)fdata->data, fdata->size, NULL, NULL, XML_PARSE_RECOVER | XML_PARSE_NOBLANKS);
    if (!doc)
        goto end;

    if (m_bForceKill)
        goto end;

#if 1
    
    htmlDocDumpMemoryFormat(doc, &format_str, &size, 1);
    xmlFreeDoc(doc);
    if (!format_str || size <= 0)
    {
        if (format_str)
            xmlFree(format_str);
        goto end;
    }
    
    xmlKeepBlanksDefault(1);
    doc = xmlReadMemory((const char *)format_str, size, NULL, NULL, XML_PARSE_RECOVER | XML_PARSE_HUGE /*| XML_PARSE_NOBLANKS */ ); //XML_PARSE_HUGE 大文件支持
    xmlFree(format_str);
    if (!doc)
        goto end;
    xmlDocDumpFormatMemory(doc, &format_str, &size, 1);
#endif
    
    xpathctx = xmlXPathNewContext(doc);
    if (!xpathctx)
        goto end;

    if (m_bForceKill)
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

            if (!DecodeText((const char *)value, (int)strlen((const char *)value), title, tlen))
            {
                xmlFree(value);
                break;
            }
            xmlFree(value);
            break;
        }
    }

    if (m_bForceKill)
        goto end;

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

            if (!DecodeText((const char *)value, (int)strlen((const char *)value), text, len))
                ret = FALSE;
            else
                ret = TRUE;

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

BOOL MobiBook::ParserChapters(mobi_t &mobi)
{
    typedef struct buffer_t
    {
        wchar_t *text;
        int len;
    } buffer_t;

    spines_t::iterator itspine;
    filelist_t::iterator itflist;
    manifests_t::iterator itmfest;
    navpoints_t::iterator itnav;
    file_data_t *fdata;
    chapter_item_t chapter;
    std::string filename;
    wchar_t *text = NULL;
    wchar_t *title = NULL;
    int len = 0, tlen = 0;
    int index = 0, i=0;
    buffer_t *buffer = NULL;
    int sepidx = 0, offsetlen = 0;
    int spines_size = 0, nav_size = 0;

    m_Length = GetCover() ? 1 : 0;
    buffer = (buffer_t *)malloc(mobi.spines.size() * sizeof(buffer_t));

    spines_size = mobi.spines.size();
    nav_size = mobi.navpoints.size();

    for (itspine = mobi.spines.begin(); itspine != mobi.spines.end(); itspine++)
    {
        len = 0;
        tlen = 0;
        if (m_bForceKill)
            goto end;
        itmfest = mobi.manifests.find(*itspine);    //只有在spines里面的才是有文字的，manifests里面还有图片，暂时不支持
        if (itmfest != mobi.manifests.end())
        {
            filename = mobi.path + itmfest->second->href;
            itflist = m_flist.find(filename);
            itnav = mobi.navpoints.find(itmfest->second->href);
            if (itflist != m_flist.end() /*&& itnav != mobi.navmap.end()*/)
            {
                fdata = &(itflist->second);
                tlen = 0;
                if (ParserOps(fdata, &text, &len, &title, &tlen, itnav == mobi.navpoints.end())) //当nav找不到对应文件时，从文件中获取章节名
                {
                    if (len > 0)
                    {
                        buffer[index].text = text;
                        buffer[index].len = len;
                        chapter.index = m_Length;
                        m_Length += len;

                        if (itnav != mobi.navpoints.end())  //当nav有对应文件时，从nav中获取章节名；之前ParserNCX处理了nav不可靠的情况
                        {
                            DecodeText(itnav->second->text.c_str(), (int)itnav->second->text.size(), &title, &tlen);
                        }
                        
                        if (tlen > 0)
                        {
                            chapter.title = title;
                            chapter.title_len = tlen;
                            m_Chapters.push_back(chapter);
                            
                            free(title);
                        }                        

                        index++;
                    }
                }
            }
        }
    }

end:
    if (m_bForceKill)
    {
        for (i = 0; i < index; i++)
        {
            free(buffer[i].text);
        }
        free(buffer);
        return FALSE;
    }
    if (index > 0)
    {
        if (GetCover())
        {
            len = 1; // add one wchar_t '0x0a' new line for cover
            m_Text = (wchar_t *)malloc(sizeof(wchar_t) * (m_Length + 1));
            m_Text[0] = 0x0A;
        }
        else
        {
            len = 0;
            m_Text = (wchar_t *)malloc(sizeof(wchar_t) * (m_Length + 1));
        }
        m_Text[m_Length] = 0;
        
        for (i = 0; i < index; i++)
        {
            memcpy(m_Text + len, buffer[i].text, buffer[i].len * sizeof(wchar_t));
            len += buffer[i].len;
            free(buffer[i].text);
        }
        
        if (spines_size == 1) //spines_size=1的情况即只有一个part，以nav信息生成目录，从内容中直接找目录字符串，来匹配位置，不一定准确
        {
            std::vector<int> navidx;
            std::wstring buff = m_Text;
            itflist = m_flist.find(mobi.path + mobi.manifests.find(mobi.spines.front())->second->href);
            int total_size = itflist->second.size;
            float idx_ratio = float(len) / float(total_size);
                
            for (itnav = mobi.navpoints.begin(); itnav != mobi.navpoints.end(); itnav++)
            {
                DecodeText(itnav->second->text.c_str(), (int)itnav->second->text.size(), &title, &tlen);

                
                //一个part，src经常是part00000.html#0000047473这样的形式，数字是偏移量
                sepidx = itnav->second->src.find("#");
                offsetlen = itnav->second->src.size();
                int refidx = std::stoi(itnav->second->src.substr(sepidx+1, offsetlen - sepidx-1)) * idx_ratio;
                
                int pos = 0, idx = 0;
                while((idx = buff.find(title, pos)) > 0)  //一般会有三次匹配，头尾都是目录，中间是正文，取第二次的位置；如果多于三次，取第三次的位置
                {
                    navidx.push_back(idx);
                    pos = idx + tlen;
                    if (navidx.size() >= 3)
                        break;
                }
                
                if (!navidx.empty())
                {
                    idx = buff.rfind(title);     //从文件尾查找一次

                    if (navidx.back() != idx)   // 章节名文字出现超过三次，要猜一下哪个是章节名，哪个是文中出现的
                    {
                        if (refidx > navidx[1])
                        {
                            chapter.index = navidx.at(((navidx[1] - navidx[0]) > (navidx[2] - navidx[1])
                                                || (navidx[2] > refidx)) ? 1 : 2);
                        }
                        else
                        {
                            chapter.index = refidx;
                        }
                    }
                    else
                    {
                        chapter.index = navidx.at((navidx.size() > 2) ? 1 : 0);     //三次取中间，两次一次取第一个
                    }
                        

                    chapter.title_len = tlen;
                    chapter.title = title;                    
                    m_Chapters.push_back(chapter);
                    navidx.clear();
                }
            }
        }        
        
    }
    free(buffer);
    return TRUE;
}

BOOL MobiBook::ParserCover(mobi_t &mobi, MOBIData *m)
{
    MOBIPdbRecord *record = NULL;
    MOBIExthHeader *exth = mobi_get_exthrecord_by_tag(m, EXTH_COVEROFFSET);


    filelist_t::iterator itflist;
    manifests_t::iterator itmfest;
    navpoints_t::iterator itnav;

    IStream *pStream = NULL;
    const char *cover_fname = NULL;
    char image_fname[1024] = {0};

    if (m_Cover)
    {
        delete m_Cover;
        m_Cover = NULL;
    }

    if (exth) {
        uint32_t offset = mobi_decode_exthvalue((unsigned char*)exth->data, exth->size);
        size_t first_resource = mobi_get_first_resource_record(m);
        size_t uid = first_resource + offset;
        record = mobi_get_record_by_seqnumber(m, uid);
    }
    if (record == NULL || record->size < 4) {
        printf("Cover not found\n");
        return FALSE;
    }

    const unsigned char jpg_magic[] = "\xff\xd8\xff";
    const unsigned char gif_magic[] = "\x47\x49\x46\x38";
    const unsigned char png_magic[] = "\x89\x50\x4e\x47\x0d\x0a\x1a\x0a";
    const unsigned char bmp_magic[] = "\x42\x4d";
    
    char ext[4] = "raw";
    if (memcmp(record->data, jpg_magic, 3) == 0) {
        snprintf(ext, sizeof(ext), "%s", "jpg");
    } else if (memcmp(record->data, gif_magic, 4) == 0) {
        snprintf(ext, sizeof(ext), "%s", "gif");
    } else if (record->size >= 8 && memcmp(record->data, png_magic, 8) == 0) {
        snprintf(ext, sizeof(ext), "%s", "png");
    } else if (record->size >= 6 && memcmp(record->data, bmp_magic, 2) == 0) {
        const size_t bmp_size = (uint32_t) record->data[2] | ((uint32_t) record->data[3] << 8) |
        ((uint32_t) record->data[4] << 16) | ((uint32_t) record->data[5] << 24);
        if (record->size == bmp_size) {
            snprintf(ext, sizeof(ext), "%s", "bmp");
        }
    }
    
    pStream = SHCreateMemStream((const BYTE *)record->data, record->size);
    m_Cover = new Gdiplus::Bitmap(pStream);
    if (m_Cover)
    {
        if (Gdiplus::Ok != m_Cover->GetLastStatus())
        {
            delete m_Cover;
            m_Cover = NULL;
        }
    }
    pStream->Release();
    return m_Cover != NULL;
}


