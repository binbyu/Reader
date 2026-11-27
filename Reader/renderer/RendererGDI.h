#pragma once
#include "RendererGDIPlusBase.h"

#define ALWAYS_BLEND    (0)

class RendererGDI;
class MeasurerGDI : public MeasurerBase
{
public:
    MeasurerGDI(RendererGDI* r);
    virtual ~MeasurerGDI() = default;
protected:
    virtual TextMetrics measureTextImpl(const wchar_t* text, int length, int maxWidth, int textStyleIndex) override;
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
    virtual bool createShadowResources(int needW, int needH);
    virtual void releaseShadowResources();

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
        ShadowInfo shadow;
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

    // for shadow
    struct CharShadowGDI
    {
        std::vector<DWORD> pixels;
        int pad = 0;
        int sw = 0;
        int sh = 0;
    };

    HDC     m_shadowDC = nullptr;
    HBITMAP m_shadowBitmap = nullptr;
    HBITMAP m_oldShadowBitmap = nullptr;
    BYTE*   m_shadowBits = nullptr;
    int     m_shadowCapW = 0;
    int     m_shadowCapH = 0;
    LRUCache<CharShadowKey, CharShadowGDI, CharShadowKeyHash> m_charShadowCache;
};
