/**
 * @file BookMobi.h
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

#pragma once
#include "BookEpub.h"
#include "mobi.h"

struct Mobi : public Epub
{
    Mobi() : m(nullptr), rawml(nullptr) {}
    virtual ~Mobi()
    {
        if (rawml) { mobi_free_rawml(rawml); rawml = nullptr; }
        if (m)     { mobi_free(m); m = nullptr; }
    }

    struct MOBIData* m;
    MOBIRawml *rawml;
};

struct AnchorInfo
{
    std::string anchorId;
    std::string fullNavKey;
};

struct SlicedHtmlPart
{
    std::string xhtml;
    std::vector<std::string> navKeys;
};

class BookMobi : public BookEpub
{
public:
    BookMobi(const std::wstring& fileName);
    virtual ~BookMobi();
    virtual BookType type() const;

protected:
    virtual bool loadBook(Files& files);
    virtual bool parseOcf(Files& files);
    virtual bool parseCover(Files& files);

    // split html is written by AI
    virtual bool splitHtmlByNavpoints(Files& files);
    bool splitHtmlByAnchors(const char* data, int size, const std::vector<AnchorInfo>& anchors, std::vector<SlicedHtmlPart>& parts);

protected:
    virtual std::string formatHref(const std::string& base, const std::string& href);

private:
    Mobi* mobi() const;
};