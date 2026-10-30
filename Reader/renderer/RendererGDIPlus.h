#pragma once
#include "RendererBase.h"
#include <objidl.h>
#include <gdiplus.h>

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
public:
    void updateFrameCount();
    void decode();
public:
    Gdiplus::Bitmap* m_bitmap;
    int m_frameCount;
    int m_frameIndex;
};

class RendererGDIPlus;
class MeasurerGDIPlus : public IMeasurer
{
public:
    MeasurerGDIPlus(RendererGDIPlus* r);
    ~MeasurerGDIPlus();
    virtual TextMetrics measureText(const wchar_t* text, int length, int maxWidth, int textStyleIndex);
private:
    RendererGDIPlus* m_renderer;
};

class RendererGDIPlus : public RendererBase
{
    friend class MeasurerGDIPlus;
public:
    RendererGDIPlus();
    virtual ~RendererGDIPlus();

    virtual bool initialize();
    virtual void cleanup();
    virtual EngineType type();
    virtual IMeasurer* measurer();

    virtual bool beginPaint(HWND hWnd, HDC hdc, const RECT& rect, const TextStyles& styles, bool blend = false);
    virtual void endPaint();

    virtual void clear(const COLORREF& color);
    virtual void fillRect(const COLORREF& color, const RECT& rect);
    virtual void fillRect(const Image* image, const RECT& rect, WrapMode mode);
    virtual void drawText(const wchar_t* text, int length, const RECT& rect, int textStyleIndex);
    virtual void drawImage(const Image* image, const RECT& rect, bool keepRatio = false);

    virtual Image* loadImageFromPath(const wchar_t* path);
    virtual Image* loadImageFromStream(const BYTE* data, int size);
    virtual Image* loadImageFromResource(HINSTANCE hInstance, UINT resourceId, const wchar_t* resourceType);
    virtual Image* createImageFromBGColor(const COLORREF& color, BYTE alpha, int width, int height);
    virtual Image* createImageFromBGImage(const Image* image, BYTE alpha, int width, int height, WrapMode mode);

protected:
    virtual bool createDeviceResources();
    virtual void releaseDeviceResources();
    virtual bool createTextResources();
    virtual void releaseTextResources();

protected:
    bool createLayeredDC();
    void releaseLayeredDC();
    Gdiplus::Font* createFont(const LOGFONTW& logfont);

private:
    struct TextResource
    {
        Gdiplus::Font* font = nullptr;
        Gdiplus::SolidBrush* textBrush = nullptr;
        Gdiplus::SolidBrush* bgBrush = nullptr;
        Gdiplus::StringFormat* stringFormat = nullptr;
    };
    TextResource* textResource(int index);

private:
    typedef Gdiplus::Status(WINAPI* GdiplusStartupFunc)(
        ULONG_PTR* token,
        const Gdiplus::GdiplusStartupInput* input,
        Gdiplus::GdiplusStartupOutput* output
        );

    typedef void (WINAPI* GdiplusShutdownFunc)(
        ULONG_PTR token
        );

private:
    bool m_initialized = false;
    HMODULE m_hDll = nullptr;
    GdiplusStartupFunc m_pGdiplusStartup = nullptr;
    GdiplusShutdownFunc m_pGdiplusShutdown = nullptr;
    ULONG_PTR m_gdiplusToken = 0;

    Gdiplus::Graphics* m_graphics = nullptr;
    Gdiplus::Bitmap* m_memBitmap = nullptr;
    std::vector<TextResource> m_trs;
    MeasurerGDIPlus* m_measurer = nullptr;
};
