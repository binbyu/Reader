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

private:
    Mobi* mobi() const;
};