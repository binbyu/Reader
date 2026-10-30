#pragma once
#include "RendererBase.h"
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#include <shlwapi.h>
#include <vector>
#include <memory>
#include <algorithm>

class ImageD2D : public Image
{
public:
    ImageD2D();
    virtual ~ImageD2D();
    virtual int     width() const;
    virtual int     height() const;
    virtual void*   bitmap() const;
    virtual int     frameCount() const;
    virtual int     frameIndex() const;
    virtual int     frameDelay(int index) const;
    virtual bool    activeFrame(int index);
    virtual HBITMAP getHBITMAP() const;
    ID2D1Bitmap*    getD2DBitmap(ID2D1RenderTarget* pRT, unsigned int gid) const;
public:
    void updateFrameCount();
    bool setFrame(int index);
    bool createD2DBitmap(ID2D1RenderTarget* pRT, unsigned int gid);
    IWICBitmap* decodeFrame(IWICBitmapFrameDecode* pFrame);
public:
    IWICBitmap* m_bitmap;
    IWICBitmapDecoder* m_decoder;
    int m_frameCount;
    int m_frameIndex;
    unsigned int m_generationID;
    ID2D1Bitmap* m_d2dBitmap;
};

class RendererD2D;
class MeasurerD2D : public IMeasurer
{
public:
    MeasurerD2D(RendererD2D* r);
    ~MeasurerD2D();
    virtual TextMetrics measureText(const wchar_t* text, int length, int maxWidth, int textStyleIndex);
private:
    RendererD2D* m_renderer;
};

class ContextD2D
{
private:
    ContextD2D();
    ~ContextD2D();
public:
    static ContextD2D* instance();
    bool initialize();
    void cleanup();
    bool isInitialized();
    ID2D1Factory* D2DFactory();
    IDWriteFactory* DWriteFactory();
    IWICImagingFactory* WICFactory();
private:
    bool m_initialized = false;
    HMODULE m_hD2DDll = nullptr;
    HMODULE m_hDWriteDll = nullptr;
    HMODULE m_hWICDll = nullptr;
    ID2D1Factory* m_pD2DFactory = nullptr;
    IDWriteFactory* m_pDWriteFactory = nullptr;
    IWICImagingFactory* m_pWICFactory = nullptr;
};

class RendererD2D : public RendererBase
{
    friend class MeasurerD2D;
public:
    RendererD2D();
    virtual ~RendererD2D();

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

private:
    struct TextResource
    {
        IDWriteTextFormat* textFormat = nullptr;
        ID2D1SolidColorBrush* textBrush = nullptr;
        ID2D1SolidColorBrush* bgBrush = nullptr;

        // for fast text measure
        IDWriteFontFace* fontFace = nullptr;
    };
    TextResource* textResource(int index);

private:
    D2D1_COLOR_F colorRefToD2D(COLORREF color, BYTE alpha = 255);
    DWRITE_FONT_WEIGHT getDWriteFontWeight(LONG lfWeight);
    DWRITE_FONT_STYLE getDWriteFontStyle(BYTE lfItalic);

private:
    ID2D1RenderTarget* m_pRenderTarget = nullptr;
    ID2D1GdiInteropRenderTarget* m_pInteropTarget = nullptr;
    unsigned int m_generationID = 0;
    std::vector<TextResource> m_trs;
    MeasurerD2D* m_measurer = nullptr;
};