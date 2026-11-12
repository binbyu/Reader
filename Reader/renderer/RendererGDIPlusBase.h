#pragma once
#include "RendererBase.h"
#include <objidl.h>
#include <gdiplus.h>

/********************************************************************
*  ImageGDIPlus                                                     *
********************************************************************/
class ImageGDIPlus : public Image
{
public:
    ImageGDIPlus();
    virtual ~ImageGDIPlus();
    virtual int   width() const;
    virtual int   height() const;
    virtual void* bitmap() const;
    virtual int   frameCount() const;
    virtual int   frameIndex() const;
    virtual int   frameDelay(int index) const;
    virtual bool  activeFrame(int index);
    virtual HBITMAP getHBITMAP() const;
    virtual bool  hasAlpha() const;
public:
    void updateFrameCount();
    void decode();
public:
    Gdiplus::Bitmap* m_bitmap;
    int m_frameCount;
    int m_frameIndex;
    bool m_hasAlpha;
};

/********************************************************************
*  ContextGDIPlus                                                   *
********************************************************************/
class ContextGDIPlus
{
private:
    ContextGDIPlus();
    ~ContextGDIPlus();
public:
    static ContextGDIPlus* instance();
    bool initialize();
    void cleanup();
private:
    typedef Gdiplus::Status(WINAPI* GdiplusStartupFunc)(ULONG_PTR* token, const Gdiplus::GdiplusStartupInput* input, Gdiplus::GdiplusStartupOutput* output);
    typedef void (WINAPI* GdiplusShutdownFunc)(ULONG_PTR token);

    bool m_initialized = false;
    HMODULE m_hDll = nullptr;
    GdiplusStartupFunc m_pGdiplusStartup = nullptr;
    GdiplusShutdownFunc m_pGdiplusShutdown = nullptr;
    ULONG_PTR m_gdiplusToken = 0;
};

/********************************************************************
*  RendererGDIPlusBase                                              *
********************************************************************/
class RendererGDIPlusBase : public RendererBase
{
public:
    RendererGDIPlusBase();
    virtual ~RendererGDIPlusBase();

    virtual Image* loadImageFromPath(const wchar_t* path);
    virtual Image* loadImageFromStream(const BYTE* data, int size);
    virtual Image* loadImageFromResource(HINSTANCE hInstance, UINT resourceId, const wchar_t* resourceType);
    virtual Image* createImageFromBGColor(const COLORREF& color, BYTE alpha, int width, int height);
    virtual Image* createImageFromBGImage(const Image* image, BYTE alpha, int width, int height, WrapMode mode);
};
