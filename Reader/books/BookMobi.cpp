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
#include <algorithm>

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

std::string BookMobi::formatHref(const std::string& base, const std::string& href)
{
#if 0 // keep anchor for mobi, because after convert mobi -> epub, only one spine
    size_t pos = href.find('#');
    std::string path = pos == std::string::npos ? href : href.substr(0, pos);
#else
    std::string path = href;
#endif
    return decodeHref(resolveHref(base, path));
}

bool BookMobi::splitHtmlByAnchors(const char* data, int size, const std::vector<AnchorInfo>& anchors, std::vector<SlicedHtmlPart>& parts)
{
    parts.clear();
    if (!data || size <= 0 || anchors.empty())
    {
        return false;
    }

    // 1. Parse HTML into DOM
    xmlDocPtr doc = htmlReadMemory(data, size, nullptr, "UTF-8",
                                   HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET);
    if (!doc)
    {
        return false;
    }

    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
    if (!xpathCtx)
    {
        xmlFreeDoc(doc);
        return false;
    }

    // 2. Locate anchor nodes in document
    struct AnchorNode
    {
        xmlNodePtr node;
        std::string fullNavKey;
    };
    std::vector<AnchorNode> anchorNodes;
    anchorNodes.reserve(anchors.size());

    for (const auto& anchor : anchors)
    {
        if (m_abort) break;
        if (anchor.anchorId.empty() || anchor.anchorId.find('\'') != std::string::npos)
        {
            continue;
        }

        std::string expr = "//*[@id='" + anchor.anchorId + "' or @name='" + anchor.anchorId + "']";
        xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression((const xmlChar*)expr.c_str(), xpathCtx);
        if (xpathObj)
        {
            if (xpathObj->nodesetval && xpathObj->nodesetval->nodeNr > 0)
            {
                anchorNodes.push_back({ xpathObj->nodesetval->nodeTab[0], anchor.fullNavKey });
            }
            xmlXPathFreeObject(xpathObj);
        }
    }

    xmlXPathFreeContext(xpathCtx);

    if (m_abort || anchorNodes.empty())
    {
        xmlFreeDoc(doc);
        return false;
    }

    // 3. Find <body> and lowest common container for all chapters
    xmlNodePtr root = xmlDocGetRootElement(doc);
    xmlNodePtr bodyNode = nullptr;
    for (xmlNodePtr cur = root ? root->children : nullptr; cur; cur = cur->next)
    {
        if (cur->type == XML_ELEMENT_NODE && xmlStrcasecmp(cur->name, BAD_CAST "body") == 0)
        {
            bodyNode = cur;
            break;
        }
    }
    if (!bodyNode)
    {
        bodyNode = root;
    }
    if (!bodyNode)
    {
        xmlFreeDoc(doc);
        return false;
    }

    auto getChildUnder = [](xmlNodePtr node, xmlNodePtr parent) -> xmlNodePtr {
        xmlNodePtr cur = node;
        while (cur && cur->parent && cur->parent != parent)
        {
            cur = cur->parent;
        }
        return (cur && cur->parent == parent) ? cur : nullptr;
    };

    xmlNodePtr container = bodyNode;
    while (true)
    {
        xmlNodePtr commonChild = nullptr;
        bool allSame = true;
        for (const auto& an : anchorNodes)
        {
            xmlNodePtr c = getChildUnder(an.node, container);
            if (!c)
            {
                allSame = false;
                break;
            }
            if (!commonChild)
            {
                commonChild = c;
            }
            else if (commonChild != c)
            {
                allSame = false;
                break;
            }
        }
        if (allSame && commonChild && commonChild->children)
        {
            container = commonChild;
        }
        else
        {
            break;
        }
    }

    // 4. Map container's direct child nodes to anchor navKeys
    std::map<xmlNodePtr, std::vector<std::string>> nodeToNavKeys;
    for (const auto& an : anchorNodes)
    {
        xmlNodePtr c = getChildUnder(an.node, container);
        if (c)
        {
            nodeToNavKeys[c].push_back(an.fullNavKey);
        }
    }

    if (nodeToNavKeys.empty())
    {
        xmlFreeDoc(doc);
        return false;
    }

    // 5. Group nodes into slices in document order
    struct Slice
    {
        xmlNodePtr startNode;
        std::vector<std::string> navKeys;
    };
    std::vector<Slice> slices;

    for (xmlNodePtr cur = container->children; cur; cur = cur->next)
    {
        auto it = nodeToNavKeys.find(cur);
        if (it != nodeToNavKeys.end())
        {
            slices.push_back({ cur, it->second });
        }
        else if (slices.empty())
        {
            slices.push_back({ cur, {} }); // Content before the first anchor
        }
    }

    if (slices.size() <= 1)
    {
        xmlFreeDoc(doc);
        return false;
    }

    // 6. Build a standalone XHTML document for each slice
    for (size_t i = 0; i < slices.size(); ++i)
    {
        if (m_abort)
        {
            parts.clear();
            xmlFreeDoc(doc);
            return false;
        }

        xmlDocPtr newDoc = xmlNewDoc(BAD_CAST "1.0");
        xmlNodePtr htmlNode = xmlNewNode(nullptr, BAD_CAST "html");
        xmlDocSetRootElement(newDoc, htmlNode);
        xmlNewNs(htmlNode, BAD_CAST "http://www.w3.org/1999/xhtml", nullptr);

        xmlNodePtr headNode = xmlNewChild(htmlNode, nullptr, BAD_CAST "head", nullptr);
        xmlNewChild(headNode, nullptr, BAD_CAST "title", BAD_CAST "Chapter");

        xmlNodePtr newBodyNode = xmlNewChild(htmlNode, nullptr, BAD_CAST "body", nullptr);

        xmlNodePtr sliceStart = slices[i].startNode;
        xmlNodePtr sliceEnd = (i + 1 < slices.size()) ? slices[i + 1].startNode : nullptr;

        for (xmlNodePtr n = sliceStart; n && n != sliceEnd; n = n->next)
        {
            xmlNodePtr copy = xmlDocCopyNode(n, newDoc, 1);
            if (copy)
            {
                xmlAddChild(newBodyNode, copy);
            }
        }

        xmlChar* mem = nullptr;
        int memSize = 0;
        xmlDocDumpFormatMemory(newDoc, &mem, &memSize, 1);

        SlicedHtmlPart part;
        part.navKeys = slices[i].navKeys;
        if (mem && memSize > 0)
        {
            part.xhtml.assign((const char*)mem, memSize);
            xmlFree(mem);
        }
        xmlFreeDoc(newDoc);

        parts.push_back(std::move(part));
    }

    xmlFreeDoc(doc);
    return !parts.empty();
}

bool BookMobi::splitHtmlByNavpoints(Files& files)
{
    if (m_abort) return false;
    Mobi* m_mobi = mobi();
    if (!m_mobi) return false;
    if (m_mobi->navpoints.empty()) return true;

    // 1. Group anchors by base HTML relative file path
    std::map<std::string, std::vector<AnchorInfo>> fileAnchorMap;
    for (const auto& navPair : m_mobi->navpoints)
    {
        if (m_abort) return false;
        const std::string& href = navPair.second.href;
        size_t hashPos = href.find('#');
        if (hashPos != std::string::npos && hashPos + 1 < href.size())
        {
            std::string filePath = href.substr(0, hashPos);
            std::string anchorId = href.substr(hashPos + 1);
            fileAnchorMap[filePath].push_back({ anchorId, navPair.first });
        }
    }

    if (fileAnchorMap.empty()) return true;

    // 2. Process each HTML file
    for (const auto& item : fileAnchorMap)
    {
        if (m_abort) return false;
        const std::string& sourceHref = item.first;
        const std::vector<AnchorInfo>& anchors = item.second;

        auto fileIt = files.find(sourceHref);
        if (fileIt == files.end() || !fileIt->second.data || fileIt->second.size <= 0)
        {
            continue;
        }

        std::vector<SlicedHtmlPart> parts;
        if (!splitHtmlByAnchors(fileIt->second.data, fileIt->second.size, anchors, parts) || parts.size() <= 1)
        {
            continue;
        }

        // File naming
        std::string baseDir;
        std::string baseName = sourceHref;
        size_t lastSlash = sourceHref.find_last_of('/');
        if (lastSlash != std::string::npos)
        {
            baseDir = sourceHref.substr(0, lastSlash + 1);
            baseName = sourceHref.substr(lastSlash + 1);
        }
        size_t dotPos = baseName.find_last_of('.');
        if (dotPos != std::string::npos)
        {
            baseName = baseName.substr(0, dotPos);
        }

        // Save generated part files and update metadata
        for (size_t i = 0; i < parts.size(); ++i)
        {
            if (m_abort) return false;

            std::string newFileName = baseDir + baseName + "_part_" + std::to_string(i) + ".html";
            std::string newId = baseName + "_part_" + std::to_string(i);

            // Store sliced data in files map
            FileData fdata;
            fdata.size = (int)parts[i].xhtml.size();
            fdata.data = (char*)malloc(fdata.size + 1);
            if (fdata.data)
            {
                memcpy(fdata.data, parts[i].xhtml.c_str(), fdata.size);
                fdata.data[fdata.size] = 0;
                fdata.ownData = true;
                files.insert(std::make_pair(newFileName, fdata));
            }

            // Add new Manifest
            Manifest manifest;
            manifest.id = newId;
            manifest.href = newFileName;
            manifest.mediaType = "application/xhtml+xml";
            m_mobi->manifests.insert(std::make_pair(newId, std::move(manifest)));

            // Update Navpoints
            for (const auto& navKey : parts[i].navKeys)
            {
                auto navIt = m_mobi->navpoints.find(navKey);
                if (navIt != m_mobi->navpoints.end())
                {
                    Navpoint updatedNav = std::move(navIt->second);
                    updatedNav.href = newFileName;
                    m_mobi->navpoints.erase(navIt);
                    m_mobi->navpoints.insert(std::make_pair(newFileName, std::move(updatedNav)));
                }
            }
        }

        // Rebuild m_mobi->spines
        auto spineIt = std::find_if(m_mobi->spines.begin(), m_mobi->spines.end(),
            [&sourceHref](const Manifest* m) { return m->href == sourceHref; });

        if (spineIt != m_mobi->spines.end())
        {
            size_t index = std::distance(m_mobi->spines.begin(), spineIt);
            m_mobi->spines.erase(spineIt);

            for (size_t i = 0; i < parts.size(); ++i)
            {
                std::string newId = baseName + "_part_" + std::to_string(i);
                auto mfIt = m_mobi->manifests.find(newId);
                if (mfIt != m_mobi->manifests.end())
                {
                    m_mobi->spines.insert(m_mobi->spines.begin() + index + i, &(mfIt->second));
                }
            }
        }

        // Clean up original file entry
        if (fileIt->second.ownData && fileIt->second.data)
        {
            free(fileIt->second.data);
            fileIt->second.data = nullptr;
            fileIt->second.size = 0;
        }
        files.erase(fileIt);
    }
    return true;
}

Mobi* BookMobi::mobi() const
{
    return dynamic_cast<Mobi*>(m_epub);
}