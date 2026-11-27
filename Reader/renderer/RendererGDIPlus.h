#pragma once
#include "RendererGDIPlusBase.h"

class RendererGDIPlus;
class MeasurerGDIPlus : public MeasurerBase
{
public:
    MeasurerGDIPlus(RendererGDIPlus* r);
    virtual ~MeasurerGDIPlus() = default;
protected:
    virtual TextMetrics measureTextImpl(const wchar_t* text, int length, int maxWidth, int textStyleIndex) override;
};

class RendererGDIPlus : public RendererGDIPlusBase
{
    friend class MeasurerGDIPlus;
public:
    RendererGDIPlus();
    virtual ~RendererGDIPlus();
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
    Gdiplus::Font* createFont(const LOGFONTW& logfont);

private:
    struct TextResource
    {
        Gdiplus::Font* font = nullptr;
        Gdiplus::SolidBrush* textBrush = nullptr;
        Gdiplus::SolidBrush* bgBrush = nullptr;
        Gdiplus::SolidBrush* shadowBrush = nullptr;
        Gdiplus::StringFormat* stringFormat = nullptr;
        ShadowInfo shadow;
    };
    TextResource* textResource(int index);

private:
    struct CharShadowGDIPlus
    {
        Gdiplus::Bitmap* bitmap = nullptr;
        int pad = 0;
        int sw = 0;
        int sh = 0;
    };

    Gdiplus::Graphics* m_graphics = nullptr;
    Gdiplus::Bitmap* m_memBitmap = nullptr;
    std::vector<TextResource> m_trs;
    MeasurerGDIPlus* m_measurer = nullptr;

    // for shadow
    Gdiplus::Bitmap* m_pShadowBitmap = nullptr;
    Gdiplus::SolidBrush* m_pShadowWhiteBrush = nullptr;
    int m_shadowCapW = 0;
    int m_shadowCapH = 0;
    LRUCache<CharShadowKey, CharShadowGDIPlus, CharShadowKeyHash> m_charShadowCache;
};
