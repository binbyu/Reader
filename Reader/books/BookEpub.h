#pragma once
#include "BookBase.h"
#include "HtmlParser.h"
#include <map>

struct FileData
{
    char *data;
    int size;
    bool ownData = true;
};

struct Manifest
{
    std::string id;
    std::string href;
    std::string mediaType;
};

struct Navpoint
{
    std::string id;
    std::string href;
    std::string text;
    int order;
};

struct CoverMani
{
    std::string xhtmlHref; // cover.xhtml
    std::string imageHref; // cover.jpg
};

typedef std::map<std::string, FileData> Files;      // href, fileData
typedef std::map<std::string, Manifest> Manifests;  // id, Manifest
typedef std::vector<Manifest*>          Spines;
typedef std::map<std::string, Navpoint> Navpoints;  // href, Navpoint

struct Epub
{
    virtual ~Epub() {}

    std::string ocf;
    std::string opf;
    std::string nav; // EPUB 3 nav.xhtml
    std::string ncx; // EPUB 2 toc.ncx
    Manifests   manifests;
    Spines      spines;
    Navpoints   navpoints;
    CoverMani   cover;
};

class BookEpub : public BookBase
{
public:
    BookEpub(const std::wstring& fileName);
    virtual ~BookEpub();
    virtual BookType type() const;

protected:
    virtual BookErrno parseBook();
    virtual bool loadBook(Files& files);
    virtual void freeBook(Files& files);
    virtual bool parseOcf(Files& files);
    virtual bool parseOpf(Files& files);
    virtual bool parseNav(Files& files);
    virtual bool parseNcx(Files& files);
    virtual bool parseCover(Files& files);
    virtual bool parseChapters(Files& files);
    virtual bool parseOps(const FileData& file, wchar_t **text, int *len, wchar_t **title, int *tlen, bool needTitle);
    virtual bool splitHtmlByNavpoints(Files& files);

protected:
    virtual std::string formatHref(const std::string& base, const std::string& href);
    std::string decodeHref(const std::string& href);
    std::string resolveHref(const std::string& base, const std::string& href);

protected:
    Epub* m_epub;
};