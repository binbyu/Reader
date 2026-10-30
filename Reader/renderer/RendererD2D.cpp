#pragma once
#include "RendererD2D.h"

//#pragma comment(lib, "Msimg32.lib")
//#pragma comment(lib, "d2d1.lib")
//#pragma comment(lib, "dwrite.lib")
//#pragma comment(lib, "windowscodecs.lib")

#define SAFE_RELEASE(x) if (x) { (x)->Release(); (x) = nullptr; }

/********************************************************************
*  ImageD2D                                                         *
********************************************************************/
ImageD2D::ImageD2D()
    : m_bitmap(nullptr)
    , m_decoder(nullptr)
    , m_frameCount(-1)
    , m_frameIndex(0)
    , m_generationID(-1)
    , m_d2dBitmap(nullptr)
{
}

ImageD2D::~ImageD2D()
{
    SAFE_RELEASE(m_bitmap);
    SAFE_RELEASE(m_decoder);
    SAFE_RELEASE(m_d2dBitmap);
}

int ImageD2D::width() const
{
    if (m_d2dBitmap)
    {
        return static_cast<int>(m_d2dBitmap->GetSize().width);
    }
    if (m_bitmap)
    {
        UINT width = 0, height = 0;
        m_bitmap->GetSize(&width, &height);
        return static_cast<int>(width);
    }
    return 0;
}

int ImageD2D::height() const
{
    if (m_d2dBitmap)
    {
        return static_cast<int>(m_d2dBitmap->GetSize().height);
    }
    if (m_bitmap)
    {
        UINT width = 0, height = 0;
        m_bitmap->GetSize(&width, &height);
        return static_cast<int>(height);
    }
    return 0;
}

void* ImageD2D::bitmap() const
{
    return m_bitmap;
}

int ImageD2D::frameCount() const
{
    if (m_frameCount == -1)
    {
        const_cast<ImageD2D*>(this)->updateFrameCount();
    }
    return m_frameCount;
}

int ImageD2D::frameIndex() const
{
    return m_frameIndex;
}

int ImageD2D::frameDelay(int index) const
{
    int delay = -1;
    if (/*m_bitmap && */m_decoder && index >= 0 && index < frameCount())
    {
        IWICBitmapFrameDecode* pFrame = nullptr;
        if (SUCCEEDED(m_decoder->GetFrame(static_cast<UINT>(index), &pFrame)))
        {
            IWICMetadataQueryReader* pMetadataReader = nullptr;
            if (SUCCEEDED(pFrame->GetMetadataQueryReader(&pMetadataReader)))
            {
                PROPVARIANT value;
                PropVariantInit(&value);
                HRESULT hr = pMetadataReader->GetMetadataByName(L"/grctlext/Delay", &value);
                if (SUCCEEDED(hr) && value.vt == VT_UI2)
                {
                    delay = value.uiVal * 10;
                }
                PropVariantClear(&value);
            }
            SAFE_RELEASE(pMetadataReader);
        }
        SAFE_RELEASE(pFrame);
    }
    return delay;
}

bool ImageD2D::activeFrame(int index)
{
    if (index >= 0 && index < frameCount())
    {
        if (index == m_frameIndex)
        {
            return true;
        }
        return setFrame(index);
    }
    return false;
}

HBITMAP ImageD2D::getHBITMAP() const
{
    HBITMAP hBitmap = nullptr;
    IWICBitmapSource* pSource32bpp = NULL;
    void* pBits = NULL;

    if (SUCCEEDED(WICConvertBitmapSource(GUID_WICPixelFormat32bppBGR, m_bitmap, &pSource32bpp)))
    {
        int w = width();
        int h = height();

        BITMAPINFO bi = { 0 };
        ZeroMemory(&bi, sizeof(BITMAPINFO));
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = w;
        bi.bmiHeader.biHeight = -h;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;
        hBitmap = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, &pBits, NULL, 0);
        if (hBitmap)
        {
            pSource32bpp->CopyPixels(NULL, w * 4, w * h * 4, (BYTE*)pBits);
        }
        SAFE_RELEASE(pSource32bpp);
    }
    return hBitmap;
}

ID2D1Bitmap* ImageD2D::getD2DBitmap(ID2D1RenderTarget * pRT, unsigned int gid) const
{
    if (m_d2dBitmap && gid == m_generationID)
    {
        return m_d2dBitmap;
    }
    const_cast<ImageD2D*>(this)->createD2DBitmap(pRT, gid);
    return m_d2dBitmap;
}

void ImageD2D::updateFrameCount()
{
    if (m_bitmap)
    {
        if (m_decoder)
        {
            UINT count = 0;
            if (SUCCEEDED(m_decoder->GetFrameCount(&count)))
            {
                m_frameCount = static_cast<int>(count);
            }
            else
            {
                m_frameCount = 1;
            }
        }
        else
        {
            m_frameCount = 1;
        }
    }
    else
    {
        m_frameCount = 0;
    }
}

bool ImageD2D::setFrame(int index)
{
    bool res = false;
    if (m_decoder)
    {
        IWICBitmapFrameDecode* pFrame = nullptr;
        if (SUCCEEDED(m_decoder->GetFrame(static_cast<UINT>(index), &pFrame)))
        {
            IWICBitmap* pBitmap = decodeFrame(pFrame);
            if (pBitmap)
            {
                SAFE_RELEASE(m_bitmap);
                SAFE_RELEASE(m_d2dBitmap);
                m_generationID = -1;
                m_bitmap = pBitmap;
                pBitmap = nullptr;
                m_frameIndex = index;
                res = true;
            }
        }
        SAFE_RELEASE(pFrame);
    }
    return res;
}

bool ImageD2D::createD2DBitmap(ID2D1RenderTarget * pRT, unsigned int gid)
{
    SAFE_RELEASE(m_d2dBitmap);
    m_generationID = -1;

    if (!m_bitmap || !pRT)
    {
        return false;
    }
    HRESULT hr = pRT->CreateBitmapFromWicBitmap(m_bitmap, nullptr, &m_d2dBitmap);
    if (SUCCEEDED(hr))
    {
        m_generationID = gid;
        return true;
    }
    return false;
}

IWICBitmap* ImageD2D::decodeFrame(IWICBitmapFrameDecode * pFrame)
{
    IWICBitmap* pNewBitmap = nullptr;
    IWICFormatConverter* pConverter = nullptr;
    if (SUCCEEDED(ContextD2D::instance()->WICFactory()->CreateFormatConverter(&pConverter)))
    {
        if (SUCCEEDED(pConverter->Initialize(pFrame, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom)))
        {
            ContextD2D::instance()->WICFactory()->CreateBitmapFromSource(pConverter, WICBitmapCacheOnLoad, &pNewBitmap);
        }
    }
    SAFE_RELEASE(pConverter);
    return pNewBitmap;
}

/********************************************************************
*  MeasurerD2D                                                      *
********************************************************************/
MeasurerD2D::MeasurerD2D(RendererD2D * r)
    : m_renderer(r)
{
}

MeasurerD2D::~MeasurerD2D()
{
}

TextMetrics MeasurerD2D::measureText(const wchar_t* text, int length, int maxWidth, int textStyleIndex)
{
#if 1
    TextMetrics metrics = { 0, 0, 0 };

    if (!text)
    {
        return metrics;
    }
    if (length <= 0)
    {
        length = 0;
        const wchar_t* p = text;
        while (*p++)
            length++;
    }
    if (length == 0)
    {
        return metrics;
    }

    RendererD2D::TextResource* tr = m_renderer->textResource(textStyleIndex);
    if (!tr || !tr->textFormat)
    {
        return metrics;
    }

    IDWriteTextLayout* textLayout = nullptr;
    FLOAT layoutWidth = maxWidth > 0 ? static_cast<FLOAT>(maxWidth) : 10000.0f;
    FLOAT layoutHeight = 10000.0f;

    HRESULT hr = ContextD2D::instance()->DWriteFactory()->CreateTextLayout(
        text,
        static_cast<UINT32>(length),
        tr->textFormat,
        layoutWidth,
        layoutHeight,
        &textLayout
    );

    if (SUCCEEDED(hr) && textLayout)
    {
        if (maxWidth <= 0)
        {
            textLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        }

        DWRITE_TEXT_METRICS dwMetrics;
        textLayout->GetMetrics(&dwMetrics);

        metrics.width = static_cast<int>(ceilf(dwMetrics.widthIncludingTrailingWhitespace));
        metrics.height = static_cast<int>(ceilf(dwMetrics.height));
        metrics.count = length;

        textLayout->Release();
    }
    return metrics;
#else // GetGdiCompatibleGlyphMetrics
    TextMetrics metrics = { 0, 0, 0 };

    if (!text) return metrics;
    if (length <= 0) length = static_cast<int>(wcslen(text));
    if (length == 0) return metrics;

    RendererD2D::TextResource* tr = m_renderer->textResource(textStyleIndex);
    if (!tr || !tr->textFormat || !tr->fontFace)
    {
        return metrics;
    }

    IDWriteFontFace* fontFace = tr->fontFace;

    DWRITE_FONT_METRICS fontMetrics;
    fontFace->GetMetrics(&fontMetrics);

    FLOAT fontSize = tr->textFormat->GetFontSize();
    FLOAT scale = fontSize / fontMetrics.designUnitsPerEm;

    std::vector<UINT32> codePoints(length);
    for (int i = 0; i < length; ++i)
    {
        codePoints[i] = static_cast<UINT32>(text[i]);
    }

    std::vector<UINT16> glyphIndices(length);
    if (FAILED(fontFace->GetGlyphIndices(codePoints.data(), length, glyphIndices.data())))
    {
        return metrics;
    }

    std::vector<DWRITE_GLYPH_METRICS> glyphMetrics(length);
    HRESULT hr = fontFace->GetGdiCompatibleGlyphMetrics(
        fontSize,
        1.0f, // pixelsPerDip
        nullptr,
        FALSE,
        glyphIndices.data(),
        static_cast<UINT32>(length),
        glyphMetrics.data(),
        FALSE
    );

    if (SUCCEEDED(hr))
    {
        FLOAT maxLineWidth = 0.0f;
        FLOAT currentLineWidth = 0.0f;
        FLOAT singleLineHeight = static_cast<FLOAT>(fontMetrics.ascent + fontMetrics.descent + fontMetrics.lineGap) * scale;
        int lineCount = 1;

        for (int i = 0; i < length; ++i)
        {
            if (text[i] == L'\n')
            {
                maxLineWidth = (std::max)(maxLineWidth, currentLineWidth);
                currentLineWidth = 0.0f;
                lineCount++;
                continue;
            }
            if (text[i] == L'\r') continue;

            FLOAT charWidth = static_cast<FLOAT>(glyphMetrics[i].advanceWidth) * scale;

            if (maxWidth > 0 && (currentLineWidth + charWidth) > maxWidth && currentLineWidth > 0)
            {
                maxLineWidth = (std::max)(maxLineWidth, currentLineWidth);
                currentLineWidth = charWidth;
                lineCount++;
            }
            else
            {
                currentLineWidth += charWidth;
            }
        }

        maxLineWidth = (std::max)(maxLineWidth, currentLineWidth);

        metrics.width = static_cast<int>(ceilf(maxLineWidth));
        metrics.height = static_cast<int>(ceilf(lineCount * singleLineHeight));
        metrics.count = length;
    }

    return metrics;
#endif
}


/********************************************************************
*  ContextD2D                                                       *
********************************************************************/
ContextD2D* ContextD2D::instance()
{
    static ContextD2D _inst;
    return &_inst;
}

ContextD2D::ContextD2D()
{
}

ContextD2D::~ContextD2D()
{
    cleanup();
}

bool ContextD2D::initialize()
{
    if (ContextD2D::instance()->isInitialized())
        return true;

    HRESULT hr = S_OK;

    m_hD2DDll = LoadLibraryW(L"d2d1.dll");
    if (!m_hD2DDll)
        return false;

    m_hDWriteDll = LoadLibraryW(L"dwrite.dll");
    if (!m_hDWriteDll)
    {
        cleanup();
        return false;
    }

    m_hWICDll = LoadLibraryW(L"windowscodecs.dll");
    if (!m_hWICDll)
    {
        cleanup();
        return false;
    }

    // create D2DFactory
    typedef HRESULT(WINAPI* D2D1CreateFactoryFunc)(
        D2D1_FACTORY_TYPE factoryType,
        REFIID riid,
        const D2D1_FACTORY_OPTIONS* pFactoryOptions,
        void** ppIFactory
        );

    auto pD2D1CreateFactory = reinterpret_cast<D2D1CreateFactoryFunc>(GetProcAddress(m_hD2DDll, "D2D1CreateFactory"));
    if (!pD2D1CreateFactory)
    {
        cleanup();
        return false;
    }
    hr = pD2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        __uuidof(ID2D1Factory),
        nullptr,
        reinterpret_cast<void**>(&m_pD2DFactory)
    );
    if (FAILED(hr))
    {
        cleanup();
        return false;
    }

    // create DWrite Factory
    typedef HRESULT(WINAPI* DWriteCreateFactoryFunc)(
        DWRITE_FACTORY_TYPE factoryType,
        REFIID iid,
        IUnknown** factory
        );

    auto pDWriteCreateFactory = reinterpret_cast<DWriteCreateFactoryFunc>(
        GetProcAddress(m_hDWriteDll, "DWriteCreateFactory")
        );
    if (!pDWriteCreateFactory)
    {
        cleanup();
        return false;
    }
    hr = pDWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&m_pDWriteFactory)
    );
    if (FAILED(hr))
    {
        cleanup();
        return false;
    }

    // create WIC Factory
    typedef HRESULT(WINAPI* WICCreateImagingFactoryFunc)(
        UINT32 SDKVersion,
        IWICImagingFactory** ppIImagingFactory
        );
    auto pWICCreateImagingFactory = reinterpret_cast<WICCreateImagingFactoryFunc>(
        GetProcAddress(m_hWICDll, "WICCreateImagingFactory_Proxy")
        );
    if (!pWICCreateImagingFactory)
    {
        pWICCreateImagingFactory = reinterpret_cast<WICCreateImagingFactoryFunc>(
            GetProcAddress(m_hWICDll, "WICCreateImagingFactory")
            );
    }
    if (!pWICCreateImagingFactory)
    {
        cleanup();
        return false;
    }
    hr = pWICCreateImagingFactory(WINCODEC_SDK_VERSION, &m_pWICFactory);
    if (FAILED(hr))
    {
        cleanup();
        return false;
    }
    m_initialized = true;
    return true;
}

void ContextD2D::cleanup()
{
    SAFE_RELEASE(m_pWICFactory);
    SAFE_RELEASE(m_pDWriteFactory);
    SAFE_RELEASE(m_pD2DFactory);

    if (m_hWICDll)
    {
        FreeLibrary(m_hWICDll);
        m_hWICDll = nullptr;
    }
    if (m_hDWriteDll)
    {
        FreeLibrary(m_hDWriteDll);
        m_hDWriteDll = nullptr;
    }
    if (m_hD2DDll)
    {
        FreeLibrary(m_hD2DDll);
        m_hD2DDll = nullptr;
    }
    m_initialized = false;
}

bool ContextD2D::isInitialized()
{
    return m_initialized;
}

ID2D1Factory* ContextD2D::D2DFactory()
{
    return m_pD2DFactory;
}

IDWriteFactory* ContextD2D::DWriteFactory()
{
    return m_pDWriteFactory;
}

IWICImagingFactory* ContextD2D::WICFactory()
{
    return m_pWICFactory;
}


/********************************************************************
*  RendererD2D                                                      *
********************************************************************/
RendererD2D::RendererD2D()
{
}

RendererD2D::~RendererD2D()
{
    cleanup();
}

bool RendererD2D::initialize()
{
    return ContextD2D::instance()->initialize();
}

void RendererD2D::cleanup()
{
    if (m_measurer)
    {
        delete m_measurer;
        m_measurer = nullptr;
    }
    releaseTextResources();
    releaseDeviceResources();
    releaseLayeredDC();
    ContextD2D::instance()->cleanup();
}

EngineType RendererD2D::type()
{
    return D2D;
}

IMeasurer* RendererD2D::measurer()
{
    if (!m_measurer)
    {
        m_measurer = new MeasurerD2D(this);
    }
    return m_measurer;
}

bool RendererD2D::beginPaint(HWND hWnd, HDC hdc, const RECT& rect, const TextStyles&styles, bool blend)
{
    if (!ContextD2D::instance()->isInitialized() || !hWnd/* || styles.empty()*/)
    {
        return false;
    }

    if (m_hWnd != hWnd)
    {
        releaseTextResources();
        releaseDeviceResources();
    }
    else if (memcmp(&m_rect, &rect, sizeof(RECT)) != 0)
    {
        releaseTextResources();
        releaseDeviceResources();
    }
    else if (m_styles != styles)
    {
        releaseTextResources();
    }

    m_hWnd = hWnd;
    m_hdc = hdc;
    m_styles = styles;
    m_rect = rect;
	m_isBlend = blend;

    if (!createLayeredDC())
    {
        goto FAILED;
    }
    if (!createDeviceResources())
    {
        goto FAILED;
    }
    if (!createTextResources())
    {
        goto FAILED;
    }
    m_pRenderTarget->BeginDraw();
    m_pRenderTarget->Clear(D2D1::ColorF(0, 0, 0, 0));
    return true;

FAILED:
    releaseTextResources();
    releaseDeviceResources();
    releaseLayeredDC();
    return false;
}

void RendererD2D::endPaint()
{
    if (m_pRenderTarget)
    {
        HDC memdc = nullptr;
        HRESULT hr = m_pInteropTarget->GetDC(D2D1_DC_INITIALIZE_MODE_COPY, &memdc);
        if (SUCCEEDED(hr))
        {
            if (m_isLayered) // Layered Windows
            {
                RECT winRect = m_rect;
                ClientToScreen(m_hWnd, reinterpret_cast<POINT*>(&winRect.left));
                ClientToScreen(m_hWnd, reinterpret_cast<POINT*>(&winRect.right));

                BLENDFUNCTION blend = { 0 };
                blend.BlendOp = AC_SRC_OVER;
                blend.BlendFlags = 0;
                blend.SourceConstantAlpha = 0xFF;
                blend.AlphaFormat = AC_SRC_ALPHA;
                POINT ptPos = { winRect.left, winRect.top };
                SIZE sizeWnd = { winRect.right - winRect.left, winRect.bottom - winRect.top };
                POINT ptSrc = { 0, 0 };
                UpdateLayeredWindow(m_hWnd, /*m_hdc*/NULL, &ptPos, &sizeWnd, memdc, &ptSrc, 0, &blend, ULW_ALPHA);
            }
            else // Normal Window
            {
                int x = m_rect.left;
                int y = m_rect.top;
                int w = m_rect.right - m_rect.left;
                int h = m_rect.bottom - m_rect.top;
                if (m_isBlend)
                {
                    BLENDFUNCTION blend = { 0 };
                    blend.BlendOp = AC_SRC_OVER;
                    blend.BlendFlags = 0;
                    blend.SourceConstantAlpha = 0xFF;
                    blend.AlphaFormat = AC_SRC_ALPHA;
                    AlphaBlend(m_hdc, x, y, w, h, memdc, 0, 0, w, h, blend);
                }
                else
                {
                    BitBlt(m_hdc, x, y, w, h, memdc, 0, 0, SRCCOPY);
                }
            }
            RECT rcDirty = {};
            m_pInteropTarget->ReleaseDC(&rcDirty);
        }

        hr = m_pRenderTarget->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET)
        {
            releaseTextResources();
            releaseDeviceResources();
            releaseLayeredDC();
            return;
        }
    }

    //releaseTextResources();
    //releaseDeviceResources();
    releaseLayeredDC();
}

void RendererD2D::clear(const COLORREF & color)
{
    if (!m_pRenderTarget)
    {
        return;
    }
    m_pRenderTarget->Clear(colorRefToD2D(color, GetAValue(color)));
}

void RendererD2D::fillRect(const COLORREF & color, const RECT & rect)
{
    if (!m_pRenderTarget)
    {
        return;
    }

    D2D1_RECT_F rc = {
        static_cast<FLOAT>(rect.left),
        static_cast<FLOAT>(rect.top),
        static_cast<FLOAT>(rect.right),
        static_cast<FLOAT>(rect.bottom)
    };

    ID2D1SolidColorBrush* pBrush = nullptr;
    if (SUCCEEDED(m_pRenderTarget->CreateSolidColorBrush(colorRefToD2D(color, GetAValue(color)), &pBrush)))
    {
        m_pRenderTarget->FillRectangle(rc, pBrush);
    }
    SAFE_RELEASE(pBrush);
}

void RendererD2D::fillRect(const Image * image, const RECT & rect, WrapMode mode)
{
    if (!image || !m_pRenderTarget)
    {
        return;
    }

    D2D1_EXTEND_MODE extendMode = D2D1_EXTEND_MODE_CLAMP;
    switch (mode)
    {
    case STRETCH:
        drawImage(image, rect, false); //extendMode = D2D1_EXTEND_MODE_CLAMP;
        return;
    case TILE:
        extendMode = D2D1_EXTEND_MODE_WRAP;
        break;
    case TILEFLIP:
        extendMode = D2D1_EXTEND_MODE_MIRROR;
        break;
    default:
        break;
    }
    const ImageD2D* imd2d = dynamic_cast<const ImageD2D*>(image);
    if (!imd2d || imd2d->width() <= 0 || imd2d->height() <= 0)
    {
        return;
    }
    ID2D1Bitmap* pBitmap = imd2d->getD2DBitmap(m_pRenderTarget, m_generationID);
    if (!pBitmap)
    {
        return;
    }

    ID2D1BitmapBrush* pBrush = nullptr;
    D2D1_BITMAP_BRUSH_PROPERTIES brushProps;
    brushProps.extendModeX = extendMode;
    brushProps.extendModeY = extendMode;
    brushProps.interpolationMode = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;
    if (SUCCEEDED(m_pRenderTarget->CreateBitmapBrush(pBitmap, brushProps, &pBrush)))
    {
        D2D1_RECT_F rc = {
        static_cast<FLOAT>(rect.left),
        static_cast<FLOAT>(rect.top),
        static_cast<FLOAT>(rect.right),
        static_cast<FLOAT>(rect.bottom)
        };

        m_pRenderTarget->FillRectangle(rc, pBrush);
    }
    SAFE_RELEASE(pBrush);
}

void RendererD2D::drawText(const wchar_t* text, int length, const RECT & rect, int textStyleIndex)
{
    if (!text || !m_pRenderTarget)
    {
        return;
    }

    TextResource* tr = textResource(textStyleIndex);
    if (!tr || !tr->textFormat || !tr->textBrush)
    {
        return;
    }

    if (length < 0)
    {
        length = 0;
        const wchar_t* p = text;
        while (*p++)
            length++;
    }

    if (length == 0)
    {
        return;
    }

    D2D1_RECT_F bounds = D2D1::RectF(
        static_cast<FLOAT>(rect.left),
        static_cast<FLOAT>(rect.top),
        static_cast<FLOAT>(rect.right),
        static_cast<FLOAT>(rect.bottom)
    );

    if (tr->bgBrush)
    {
        m_pRenderTarget->FillRectangle(bounds, tr->bgBrush);
    }

    IDWriteTextLayout* textLayout = nullptr;
    FLOAT layoutWidth = bounds.right - bounds.left;
    FLOAT layoutHeight = bounds.bottom - bounds.top;

    HRESULT hr = ContextD2D::instance()->DWriteFactory()->CreateTextLayout(
        text,
        static_cast<UINT32>(length),
        tr->textFormat,
        layoutWidth,
        layoutHeight,
        &textLayout
    );
    if (SUCCEEDED(hr))
    {
        m_pRenderTarget->DrawTextLayout(
            D2D1::Point2F(bounds.left, bounds.top),
            textLayout,
            tr->textBrush
        );
    }
    SAFE_RELEASE(textLayout);
}

void RendererD2D::drawImage(const Image * image, const RECT & rect, bool keepRatio)
{
    if (!image || !m_pRenderTarget)
    {
        return;
    }

    const ImageD2D* imd2d = dynamic_cast<const ImageD2D*>(image);
    if (!imd2d || imd2d->width() <= 0 || imd2d->height() <= 0)
    {
        return;
    }
    ID2D1Bitmap* pBitmap = imd2d->getD2DBitmap(m_pRenderTarget, m_generationID);
    if (!pBitmap)
    {
        return;
    }

    if (keepRatio)
    {
        int w = rect.right - rect.left;
        int h = rect.bottom - rect.top;
        FLOAT bw = pBitmap->GetSize().width;
        FLOAT bh = pBitmap->GetSize().height;

        double d = static_cast<double>(w) / h;
        double bd = static_cast<double>(bw) / bh;

        int drawX = rect.left;
        int drawY = rect.top;
        int drawW = w;
        int drawH = h;

        if (bd > d)
        {
            // image is too wide
            drawW = w;
            drawH = static_cast<int>(drawW / bd);
            drawY = rect.top + (h - drawH) / 2;
        }
        else if (bd < d)
        {
            // image is too high
            drawH = h;
            drawW = static_cast<int>(bd * drawH);
            drawX = rect.left + (w - drawW) / 2;
        }

        D2D1_RECT_F dstRect = D2D1::RectF(
            static_cast<FLOAT>(drawX),
            static_cast<FLOAT>(drawY),
            static_cast<FLOAT>(drawX + drawW),
            static_cast<FLOAT>(drawY + drawH)
        );

        D2D1_RECT_F srcRect = D2D1::RectF(0, 0, bw, bh);

        m_pRenderTarget->DrawBitmap(pBitmap, dstRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, srcRect);
    }
    else
    {
        D2D1_RECT_F dstRect = D2D1::RectF(
            static_cast<FLOAT>(rect.left),
            static_cast<FLOAT>(rect.top),
            static_cast<FLOAT>(rect.right),
            static_cast<FLOAT>(rect.bottom)
        );
        m_pRenderTarget->DrawBitmap(pBitmap, dstRect, 1.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
    }
}

Image* RendererD2D::loadImageFromPath(const wchar_t* path)
{
    if (!path)
    {
        return nullptr;
    }
    ImageD2D* image = new ImageD2D;
    if (SUCCEEDED(ContextD2D::instance()->WICFactory()->CreateDecoderFromFilename(path, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &image->m_decoder)))
    {
        if (image->setFrame(0))
        {
            return image;
        }
    }
    delete image;
    return nullptr;
}

Image* RendererD2D::loadImageFromStream(const BYTE * data, int size)
{
    if (!data || size <= 0)
    {
        return nullptr;
    }
    IStream* pStream = NULL;
    pStream = SHCreateMemStream((const BYTE*)data, size);
    if (!pStream)
    {
        return nullptr;
    }
    ImageD2D* image = new ImageD2D;
    if (SUCCEEDED(ContextD2D::instance()->WICFactory()->CreateDecoderFromStream(pStream, nullptr, WICDecodeMetadataCacheOnDemand, &image->m_decoder)))
    {
        if (image->setFrame(0))
        {
            pStream->Release();
            return image;
        }
    }
    delete image;
    pStream->Release();
    return nullptr;
}

Image* RendererD2D::loadImageFromResource(HINSTANCE hInstance, UINT resourceId, const wchar_t* resourceType)
{
    if (!resourceType)
    {
        return nullptr;
    }
    IStream* pStream = loadResourceToStream(hInstance, resourceId, resourceType);
    if (!pStream)
    {
        return nullptr;
    }
    ImageD2D* image = new ImageD2D;
    if (SUCCEEDED(ContextD2D::instance()->WICFactory()->CreateDecoderFromStream(pStream, nullptr, WICDecodeMetadataCacheOnDemand, &image->m_decoder)))
    {
        if (image->setFrame(0))
        {
            freeStream(pStream);
            return image;
        }
    }
    delete image;
    freeStream(pStream);
    return nullptr;
}

Image* RendererD2D::createImageFromBGColor(const COLORREF & color, BYTE alpha, int width, int height)
{
    if (width <= 0 || height <= 0 || !m_pRenderTarget)
    {
        return nullptr;
    }
    ImageD2D* imd2d = nullptr;
    ID2D1BitmapRenderTarget* pCompatRT = nullptr;
    D2D1_SIZE_F size = D2D1::SizeF(static_cast<FLOAT>(width), static_cast<FLOAT>(height));
    if (SUCCEEDED(m_pRenderTarget->CreateCompatibleRenderTarget(size, &pCompatRT)))
    {
        ID2D1SolidColorBrush* pBrush = nullptr;
        if (SUCCEEDED(pCompatRT->CreateSolidColorBrush(colorRefToD2D(color, GetAValue(color)), &pBrush)))
        {
            pCompatRT->BeginDraw();
            pCompatRT->Clear(D2D1::ColorF(0, 0));
            D2D1_RECT_F targetRect = D2D1::RectF(0, 0, static_cast<FLOAT>(width), static_cast<FLOAT>(height));

            pCompatRT->FillRectangle(targetRect, pBrush);

            if (SUCCEEDED(pCompatRT->EndDraw()))
            {
                ID2D1Bitmap* pDstD2DBitmap = nullptr;
                if (SUCCEEDED(pCompatRT->GetBitmap(&pDstD2DBitmap)))
                {
                    imd2d = new ImageD2D;
                    imd2d->m_d2dBitmap = pDstD2DBitmap;
                    imd2d->m_generationID = m_generationID;
                    pDstD2DBitmap = nullptr;
                }
                SAFE_RELEASE(pDstD2DBitmap);
            }
        }
        SAFE_RELEASE(pBrush);
    }
    SAFE_RELEASE(pCompatRT);
    return imd2d;
}

Image* RendererD2D::createImageFromBGImage(const Image * image, BYTE alpha, int width, int height, WrapMode mode)
{
    if (width <= 0 || height <= 0 || !image || !m_pRenderTarget)
    {
        return nullptr;
    }

    const ImageD2D* srcImd2d = dynamic_cast<const ImageD2D*>(image);
    if (!srcImd2d || srcImd2d->width() <= 0 || srcImd2d->height() <= 0)
    {
        return nullptr;
    }
    ID2D1Bitmap* pSrcD2DBitmap = srcImd2d->getD2DBitmap(m_pRenderTarget, m_generationID);
    if (!pSrcD2DBitmap)
    {
        return nullptr;
    }

    ImageD2D* dstImd2d = nullptr;
    ID2D1BitmapRenderTarget* pCompatRT = nullptr;
    D2D1_SIZE_F size = D2D1::SizeF(static_cast<FLOAT>(width), static_cast<FLOAT>(height));
    if (SUCCEEDED(m_pRenderTarget->CreateCompatibleRenderTarget(size, &pCompatRT)))
    {
        pCompatRT->BeginDraw();
        pCompatRT->Clear(D2D1::ColorF(0, 0));
        D2D1_RECT_F targetRect = D2D1::RectF(0, 0, static_cast<FLOAT>(width), static_cast<FLOAT>(height));

        if (STRETCH == mode)
        {
            pCompatRT->DrawBitmap(pSrcD2DBitmap, targetRect, alpha / 255.0f, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        }
        else
        {
            ID2D1BitmapBrush* pBrush = nullptr;
            D2D1_BITMAP_BRUSH_PROPERTIES brushProps;
            D2D1_EXTEND_MODE extendMode = D2D1_EXTEND_MODE_CLAMP;
            switch (mode)
            {
            case TILE:
                extendMode = D2D1_EXTEND_MODE_WRAP;
                break;
            case TILEFLIP:
                extendMode = D2D1_EXTEND_MODE_MIRROR;
                break;
            default:
                break;
            }
            brushProps.extendModeX = extendMode;
            brushProps.extendModeY = extendMode;
            brushProps.interpolationMode = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR;
            D2D1_BRUSH_PROPERTIES brushProps2 = D2D1::BrushProperties(alpha / 255.0f);
            if (SUCCEEDED(pCompatRT->CreateBitmapBrush(pSrcD2DBitmap, brushProps, brushProps2, &pBrush)))
            {
                pCompatRT->FillRectangle(targetRect, pBrush);
            }
            SAFE_RELEASE(pBrush);
        }

        if (SUCCEEDED(pCompatRT->EndDraw()))
        {
            ID2D1Bitmap* pDstD2DBitmap = nullptr;
            if (SUCCEEDED(pCompatRT->GetBitmap(&pDstD2DBitmap)))
            {
                dstImd2d = new ImageD2D;
                dstImd2d->m_d2dBitmap = pDstD2DBitmap;
                dstImd2d->m_generationID = m_generationID;
                pDstD2DBitmap = nullptr;
            }
            SAFE_RELEASE(pDstD2DBitmap);
        }
    }
    SAFE_RELEASE(pCompatRT);
    return dstImd2d;
}

bool RendererD2D::createDeviceResources()
{
    if (!m_pRenderTarget)
    {
        IWICBitmap* pWicBitmap = nullptr;
        HRESULT hr = ContextD2D::instance()->WICFactory()->CreateBitmap(
            m_rect.right - m_rect.left, m_rect.bottom - m_rect.top,
            GUID_WICPixelFormat32bppPBGRA,
            WICBitmapCacheOnLoad,
            &pWicBitmap);
        if (!SUCCEEDED(hr))
        {
            return false;
        }

        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            0.0f, 0.0f,
            D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE
        );

        hr = ContextD2D::instance()->D2DFactory()->CreateWicBitmapRenderTarget(
            pWicBitmap,
            props,
            &m_pRenderTarget
        );
        SAFE_RELEASE(pWicBitmap);

        if (!SUCCEEDED(hr))
        {
            m_pRenderTarget = nullptr;
            return false;
        }

        hr = m_pRenderTarget->QueryInterface(
            IID_ID2D1GdiInteropRenderTarget,
            reinterpret_cast<void**>(&m_pInteropTarget));
        if (!SUCCEEDED(hr))
        {
            SAFE_RELEASE(m_pRenderTarget);
            m_pInteropTarget = nullptr;
            return false;
        }

        m_generationID++;
    }
    return true;
}

void RendererD2D::releaseDeviceResources()
{
    SAFE_RELEASE(m_pRenderTarget);
    SAFE_RELEASE(m_pInteropTarget);
}

bool RendererD2D::createTextResources()
{
    if (m_styles.empty())
    {
        m_trs.clear();
        return true;
    }

    if (m_trs.empty())
    {
        m_trs.resize(m_styles.size());
        for (size_t i = 0; i < m_styles.size(); ++i)
        {
            const TextStyle& s = m_styles[i];
            TextResource& tr = m_trs[i];

            FLOAT fontSize = static_cast<FLOAT>(std::abs(s.font.lfHeight));
            if (fontSize <= 0)
                fontSize = 12.0f;

            HRESULT hr = ContextD2D::instance()->DWriteFactory()->CreateTextFormat(
                s.font.lfFaceName,
                nullptr,
                getDWriteFontWeight(s.font.lfWeight),
                getDWriteFontStyle(s.font.lfItalic),
                DWRITE_FONT_STRETCH_NORMAL,
                fontSize,
                L"", // locale
                &tr.textFormat
            );

            if (FAILED(hr) || !tr.textFormat)
            {
                return false;
            }

            tr.textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            tr.textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            tr.textFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

            { // FontFace
                IDWriteFontCollection *collection = nullptr;
                tr.textFormat->GetFontCollection(&collection);
                if (!collection)
                {
                    ContextD2D::instance()->DWriteFactory()->GetSystemFontCollection(&collection);
                }

                if (collection)
                {
                    UINT32 index = 0;
                    BOOL exists = FALSE;
                    if (SUCCEEDED(collection->FindFamilyName(s.font.lfFaceName, &index, &exists)) && exists)
                    {
                        IDWriteFontFamily *fontFamily = nullptr;
                        if (SUCCEEDED(collection->GetFontFamily(index, &fontFamily)))
                        {
                            IDWriteFont *font = nullptr;
                            if (SUCCEEDED(fontFamily->GetFirstMatchingFont(
                                    tr.textFormat->GetFontWeight(),
                                    tr.textFormat->GetFontStretch(),
                                    tr.textFormat->GetFontStyle(),
                                    &font)))
                            {
                                font->CreateFontFace(&tr.fontFace);
                                font->Release();
                            }
                            fontFamily->Release();
                        }
                    }
                    collection->Release();
                }
            }

            D2D1_COLOR_F textColor = colorRefToD2D(s.textColor, GetAValue(s.textColor));
            hr = m_pRenderTarget->CreateSolidColorBrush(textColor, &tr.textBrush);
            if (FAILED(hr) || !tr.textBrush)
            {
                endPaint();
                return false;
            }

            if (s.haveBackgroundColor)
            {
                D2D1_COLOR_F bgColor = colorRefToD2D(s.backgroundColor, GetAValue(s.backgroundColor));
                hr = m_pRenderTarget->CreateSolidColorBrush(bgColor, &tr.bgBrush);
                if (FAILED(hr) || !tr.bgBrush)
                {
                    endPaint();
                    return false;
                }
            }
        }
    }
    return true;
}

void RendererD2D::releaseTextResources()
{
    for (auto& tr : m_trs)
    {
        if (tr.textFormat)
        {
            tr.textFormat->Release();
        }
        if (tr.textBrush)
        {
            tr.textBrush->Release();
        }
        if (tr.bgBrush)
        {
            tr.bgBrush->Release();
        }
        if (tr.fontFace)
        {
            tr.fontFace->Release();
        }
    }
    m_trs.clear();
}

bool RendererD2D::createLayeredDC()
{
    releaseLayeredDC();
    m_isLayered = m_hdc == nullptr;
    if (m_isLayered)
    {
        m_hdc = GetDC(NULL);
        return m_hdc != nullptr;
    }
    return true;
}

void RendererD2D::releaseLayeredDC()
{
    if (m_isLayered)
    {
        if (m_hdc)
        {
            ReleaseDC(NULL, m_hdc);
            m_hdc = nullptr;
        }
        m_isLayered = false;
    }
}

RendererD2D::TextResource* RendererD2D::textResource(int index)
{
    if (index >= 0 && index < static_cast<int>(m_trs.size()))
    {
        return &m_trs[index];
    }
    return nullptr;
}

D2D1_COLOR_F RendererD2D::colorRefToD2D(COLORREF color, BYTE alpha)
{
    return D2D1::ColorF(
        GetRValue(color) / 255.0f,
        GetGValue(color) / 255.0f,
        GetBValue(color) / 255.0f,
        alpha / 255.0f
    );
}

DWRITE_FONT_WEIGHT RendererD2D::getDWriteFontWeight(LONG lfWeight)
{
    if (lfWeight >= FW_HEAVY) return DWRITE_FONT_WEIGHT_ULTRA_BLACK;
    if (lfWeight >= FW_EXTRABOLD) return DWRITE_FONT_WEIGHT_EXTRA_BOLD;
    if (lfWeight >= FW_BOLD) return DWRITE_FONT_WEIGHT_BOLD;
    if (lfWeight >= FW_SEMIBOLD) return DWRITE_FONT_WEIGHT_SEMI_BOLD;
    if (lfWeight >= FW_MEDIUM) return DWRITE_FONT_WEIGHT_MEDIUM;
    if (lfWeight >= FW_NORMAL) return DWRITE_FONT_WEIGHT_NORMAL;
    if (lfWeight >= FW_LIGHT) return DWRITE_FONT_WEIGHT_LIGHT;
    if (lfWeight >= FW_EXTRALIGHT) return DWRITE_FONT_WEIGHT_EXTRA_LIGHT;
    return DWRITE_FONT_WEIGHT_THIN;
}

DWRITE_FONT_STYLE RendererD2D::getDWriteFontStyle(BYTE lfItalic)
{
    return lfItalic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
}