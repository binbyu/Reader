#pragma once
#include "RendererGDIPlusBase.h"

#define ALWAYS_BLEND    (0)

class RendererGDI;
class MeasurerGDI : public IMeasurer
{
public:
    MeasurerGDI(RendererGDI* r);
    ~MeasurerGDI();
    virtual TextMetrics measureText(const wchar_t* text, int length, int maxWidth, int textStyleIndex);
private:
    RendererGDI* m_renderer;
};

class RendererGDI : public RendererGDIPlusBase
{
    friend class MeasurerGDI;
public:
    RendererGDI();
    virtual ~RendererGDI();
    virtual RendererType type();
    virtual IMeasurer* measurer();

    virtual bool beginPaint(HWND hWnd, HDC hdc, const RECT& rect, const TextStyles& styles, bool clearType = false, bool blend = false);
    virtual void endPaint();

    virtual void clear(const COLORREF& color);
    virtual void fillRect(const COLORREF& color, const RECT& rect);
    virtual void fillRect(const Image* image, const RECT& rect, WrapMode mode);
    virtual void drawText(const wchar_t* text, int length, const RECT& rect, int textStyleIndex);
    virtual void drawImage(const Image* image, const RECT& rect, bool keepRatio = false);

protected:
    virtual bool createDeviceResources();
    virtual void releaseDeviceResources();
    virtual bool createTextResources();
    virtual void releaseTextResources();

protected:
    bool createLayeredDC();
    void releaseLayeredDC();
    inline DWORD premultiplyPixel(BYTE red, BYTE green, BYTE blue, BYTE alpha);
    inline void  blendPremultipliedPixel(DWORD& destination, DWORD source);
    inline BYTE  textCoverage(DWORD pixel);

private:
    struct TextResource
    {
        HFONT    hFont = nullptr;
        COLORREF textColor = 0;
        COLORREF bgColor = 0;
        bool     haveBgColor = false;
    };
    TextResource* textResource(int index);

private:
    HDC     m_memDC = nullptr;
    HBITMAP m_memBitmap = nullptr;
    HBITMAP m_oldBitmap = nullptr;
    BYTE*   m_memBits = nullptr;

    HDC     m_textDC = nullptr;
    HBITMAP m_textBitmap = nullptr;
    HBITMAP m_oldTextBitmap = nullptr;
    BYTE*   m_textBits = nullptr;
#if !ALWAYS_BLEND
    BOOL    m_textDCUsed = false;
#endif

    std::vector<TextResource> m_trs;
    MeasurerGDI* m_measurer = nullptr;
};
