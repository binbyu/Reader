#pragma once
#include "RendererGDIPlus.h"

//#pragma comment(lib, "Msimg32.lib")
//#pragma comment(lib, "gdiplus.lib")

/********************************************************************
*  ImageGDIPlus                                                     *
********************************************************************/
ImageGDIPlus::ImageGDIPlus()
    : m_bitmap(nullptr)
    , m_frameCount(-1)
    , m_frameIndex(0)
{
}

ImageGDIPlus::~ImageGDIPlus()
{
    if (m_bitmap)
    {
        delete m_bitmap;
        m_bitmap = nullptr;
    }
}

int ImageGDIPlus::width() const
{
    return m_bitmap ? m_bitmap->GetWidth() : 0;
}

int ImageGDIPlus::height() const
{
    return m_bitmap ? m_bitmap->GetHeight() : 0;
}

void* ImageGDIPlus::bitmap() const
{
    return m_bitmap;
}

int ImageGDIPlus::frameCount() const
{
    if (m_frameCount == -1)
    {
        const_cast<ImageGDIPlus*>(this)->updateFrameCount();
    }
    return m_frameCount;
}

int ImageGDIPlus::frameIndex() const
{
    return m_frameIndex;
}

int ImageGDIPlus::frameDelay(int index) const
{
    if (m_bitmap && index >= 0 && index < frameCount())
    {
        UINT size = m_bitmap->GetPropertyItemSize(PropertyTagFrameDelay);
        if (size > 0)
        {
            std::vector<BYTE> buffer(size);
            Gdiplus::PropertyItem* item = (Gdiplus::PropertyItem*)buffer.data();
            if (Gdiplus::Ok == m_bitmap->GetPropertyItem(PropertyTagFrameDelay, size, item))
            {
                UINT* delays = (UINT*)item->value;
                if (index < (int)item->length / (int)sizeof(UINT))
                {
                    return delays[index] * 10; // unit is 10ms
                }
            }
        }
    }
    return -1;
}

bool ImageGDIPlus::activeFrame(int index)
{
    if (m_bitmap && index >= 0 && index < frameCount())
    {
        if (index == m_frameIndex)
        {
            return true;
        }

        GUID Guid = Gdiplus::FrameDimensionTime;
        if (Gdiplus::Ok == m_bitmap->SelectActiveFrame(&Guid, index))
        {
            m_frameIndex = index;
            return true;
        }
    }
    return false;
}

HBITMAP ImageGDIPlus::getHBITMAP() const
{
    HBITMAP hBitmap = nullptr;
    m_bitmap->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hBitmap);
    return hBitmap;
}

void ImageGDIPlus::updateFrameCount()
{
    if (m_bitmap)
    {
        UINT count = m_bitmap->GetFrameDimensionsCount();
        if (count == 0)
        {
            m_frameCount = 1;
        }
        else
        {
            GUID* pDimensionIDs = new GUID[count];
            m_bitmap->GetFrameDimensionsList(pDimensionIDs, count);
            m_frameCount = m_bitmap->GetFrameCount(&pDimensionIDs[0]);
            delete[] pDimensionIDs;
        }
    }
    else
    {
        m_frameCount = 0;
    }
}

void ImageGDIPlus::decode()
{
    if (m_bitmap)
    {
#if 1
        Gdiplus::Bitmap* decoded = m_bitmap->Clone(0, 0, m_bitmap->GetWidth(), m_bitmap->GetHeight(), PixelFormat32bppPARGB);
        if (decoded && Gdiplus::Ok == decoded->GetLastStatus())
        {
            delete m_bitmap;
            m_bitmap = decoded;
            decoded = nullptr;
        }
#else
        Gdiplus::Bitmap* decoded = new Gdiplus::Bitmap(m_bitmap->GetWidth(), m_bitmap->GetHeight(), PixelFormat32bppPARGB);
        Gdiplus::Graphics graphics(decoded);
        graphics.DrawImage(m_bitmap, 0, 0);
        delete m_bitmap;
        m_bitmap = decoded;
        decoded = nullptr;
#endif
        if (decoded)
        {
            delete decoded;
        }
    }
}


/********************************************************************
*  MeasurerGDIPlus                                                  *
********************************************************************/
MeasurerGDIPlus::MeasurerGDIPlus(RendererGDIPlus* r)
    : m_renderer(r)
{
}

MeasurerGDIPlus::~MeasurerGDIPlus()
{
}

TextMetrics MeasurerGDIPlus::measureText(const wchar_t* text, int length, int maxWidth, int textStyleIndex)
{
#if 1
    TextMetrics metrics = { 0, 0, 0 };
    if (!text || !m_renderer || !m_renderer->m_graphics)
    {
        return metrics;
    }

    RendererGDIPlus::TextResource* tr = m_renderer->textResource(textStyleIndex);
    if (!tr || !tr->font || !tr->stringFormat)
    {
        return metrics;
    }

    Gdiplus::RectF rc;
    m_renderer->m_graphics->MeasureString(text, length, tr->font, Gdiplus::PointF(0, 0), tr->stringFormat, &rc);
    metrics.width = static_cast<int>(rc.Width);
    metrics.height = static_cast<int>(rc.Height);
    metrics.count = length;
    return metrics;
#else
    TextMetrics metrics = { 0, 0, 0 };
    if (!text || !m_renderer)
    {
        return metrics;
    }

    if (length <= 0)
    {
        length = static_cast<int>(wcslen(text));
    }
    if (length == 0)
    {
        return metrics;
    }

    RendererGDIPlus::TextResource* tr = m_renderer->textResource(textStyleIndex);
    if (!tr || !tr->font || !tr->stringFormat)
    {
        return metrics;
    }

    Gdiplus::Graphics* g = m_renderer->m_graphics;
    HDC tempDC = NULL;
    Gdiplus::Graphics* tempG = nullptr;

    if (!g)
    {
        tempDC = ::GetDC(NULL);
        tempG = new Gdiplus::Graphics(tempDC);
        g = tempG;
    }

    Gdiplus::REAL layoutW = (maxWidth > 0) ? static_cast<Gdiplus::REAL>(maxWidth) : 999999.0f;
    Gdiplus::RectF layoutRect(0.0f, 0.0f, layoutW, 999999.0f);

    Gdiplus::CharacterRange cr(0, length);
    
    Gdiplus::StringFormat format;
    format.SetFormatFlags(tr->stringFormat->GetFormatFlags() | Gdiplus::StringFormatFlagsNoClip);
    format.SetMeasurableCharacterRanges(1, &cr);

    Gdiplus::Region region;
    if (g->MeasureCharacterRanges(text, length, tr->font, layoutRect, &format, 1, &region) == Gdiplus::Ok)
    {
        Gdiplus::RectF boundRect;
        region.GetBounds(&boundRect, g);

        metrics.width = static_cast<int>(ceilf(boundRect.Width));
        metrics.height = static_cast<int>(ceilf(boundRect.Height));
        metrics.count = length;
    }
    if (tempG)
    {
        delete tempG;
        ::ReleaseDC(NULL, tempDC);
    }

    return metrics;
#endif
}


/********************************************************************
*  RendererGDIPlus                                                  *
********************************************************************/
RendererGDIPlus::RendererGDIPlus()
{
}

RendererGDIPlus::~RendererGDIPlus()
{
    cleanup();
}

bool RendererGDIPlus::initialize()
{
    if (!m_initialized)
    {
        m_hDll = LoadLibraryW(L"gdiplus.dll");
        if (!m_hDll)
        {
            return false;
        }

        m_pGdiplusStartup = reinterpret_cast<GdiplusStartupFunc>(GetProcAddress(m_hDll, "GdiplusStartup"));
        if (!m_pGdiplusStartup)
        {
            cleanup();
            return false;
        }

        m_pGdiplusShutdown = reinterpret_cast<GdiplusShutdownFunc>(GetProcAddress(m_hDll, "GdiplusShutdown"));
        if (!m_pGdiplusShutdown)
        {
            cleanup();
            return false;
        }

        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartupOutput gdiplusStartupOutput;
        Gdiplus::Status status = m_pGdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, &gdiplusStartupOutput);
        if (status != Gdiplus::Ok)
        {
            cleanup();
            return false;
        }
        m_initialized = true;
    }
    return true;
}

void RendererGDIPlus::cleanup()
{
    if (m_measurer)
    {
        delete m_measurer;
        m_measurer = nullptr;
    }

    releaseTextResources();
    releaseDeviceResources();
    releaseLayeredDC();

    if (m_initialized && m_pGdiplusShutdown)
    {
        m_pGdiplusShutdown(m_gdiplusToken);
        m_gdiplusToken = 0;
        m_initialized = false;
    }
    if (m_hDll)
    {
        FreeLibrary(m_hDll);
        m_hDll = nullptr;
    }

    m_pGdiplusStartup = nullptr;
    m_pGdiplusShutdown = nullptr;
}

EngineType RendererGDIPlus::type()
{
    return GDIPLUS;
}

IMeasurer* RendererGDIPlus::measurer()
{
    if (!m_measurer)
    {
        m_measurer = new MeasurerGDIPlus(this);
    }
    return m_measurer;
}

bool RendererGDIPlus::beginPaint(HWND hWnd, HDC hdc, const RECT& rect, const TextStyles& styles, bool blend)
{
    if (!m_initialized || !hWnd/* || styles.empty()*/)
    {
        return false;
    }

    if (m_styles != styles)
    {
        releaseTextResources();
    }
    if (memcmp(&m_rect, &rect, sizeof(RECT)) != 0)
    {
        releaseDeviceResources();
    }

    m_hWnd = hWnd;
    m_hdc = hdc;
    m_rect = rect;
    m_styles = styles;
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
    return true;

FAILED:
    releaseTextResources();
    releaseDeviceResources();
    releaseLayeredDC();
    return false;
}

void RendererGDIPlus::endPaint()
{
    if (m_memBitmap)
    {
        HDC memdc = CreateCompatibleDC(m_hdc);
        HBITMAP hBitmap = nullptr;
        auto status = m_memBitmap->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hBitmap);
        SelectObject(memdc, hBitmap);

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

        DeleteObject(hBitmap);
        DeleteDC(memdc);
    }

    //releaseTextResources();
    //releaseDeviceResources();
    releaseLayeredDC();
}

void RendererGDIPlus::clear(const COLORREF& color)
{
    if (!m_initialized || !m_graphics)
    {
        return;
    }
    m_graphics->Clear(Gdiplus::Color(GetAValue(color), GetRValue(color), GetGValue(color), GetBValue(color)));
}

void RendererGDIPlus::fillRect(const COLORREF& color, const RECT& rect)
{
    if (!m_initialized || !m_graphics)
    {
        return;
    }

    Gdiplus::RectF bounds(
        static_cast<Gdiplus::REAL>(rect.left),
        static_cast<Gdiplus::REAL>(rect.top),
        static_cast<Gdiplus::REAL>(rect.right - rect.left),
        static_cast<Gdiplus::REAL>(rect.bottom - rect.top)
    );

    Gdiplus::SolidBrush brush(Gdiplus::Color(GetAValue(color), GetRValue(color), GetGValue(color), GetBValue(color)));
    m_graphics->FillRectangle(&brush, bounds);
}

void RendererGDIPlus::fillRect(const Image* image, const RECT& rect, WrapMode mode)
{
    if (!image || !m_initialized || !m_graphics)
    {
        return;
    }

    Gdiplus::WrapMode mode_ = Gdiplus::WrapModeClamp;
    switch (mode)
    {
    case STRETCH:
        drawImage(image, rect, false); //mode_ = Gdiplus::WrapModeClamp;
        return;
    case TILE:
        mode_ = Gdiplus::WrapModeTile;
        break;
    case TILEFLIP:
        mode_ = Gdiplus::WrapModeTileFlipXY;
        break;
    default:
        break;
    }

    Gdiplus::Bitmap* pBitmap = static_cast<Gdiplus::Bitmap*>(image->bitmap());
    if (!pBitmap || pBitmap->GetLastStatus() != Gdiplus::Ok)
    {
        return;
    }

    Gdiplus::RectF bounds(
        static_cast<Gdiplus::REAL>(rect.left),
        static_cast<Gdiplus::REAL>(rect.top),
        static_cast<Gdiplus::REAL>(rect.right - rect.left),
        static_cast<Gdiplus::REAL>(rect.bottom - rect.top)
    );

    Gdiplus::TextureBrush brush(pBitmap, mode_);
    m_graphics->FillRectangle(&brush, bounds);
}

void RendererGDIPlus::drawText(const wchar_t* text, int length, const RECT& rect, int textStyleIndex)
{
    if (!text || !m_initialized || !m_graphics)
    {
        return;
    }

    TextResource* tr = textResource(textStyleIndex);
    if (!tr || !tr->font || !tr->stringFormat || !tr->textBrush)
    {
        return;
    }

    Gdiplus::RectF bounds(
        static_cast<Gdiplus::REAL>(rect.left),
        static_cast<Gdiplus::REAL>(rect.top),
        static_cast<Gdiplus::REAL>(rect.right - rect.left),
        static_cast<Gdiplus::REAL>(rect.bottom - rect.top)
    );

    if (tr->bgBrush)
    {
        m_graphics->FillRectangle(tr->bgBrush, bounds);
    }

    Gdiplus::PointF origin(bounds.X, bounds.Y);
    m_graphics->DrawString(text, length, tr->font, origin, tr->stringFormat, tr->textBrush);
}

void RendererGDIPlus::drawImage(const Image* image, const RECT& rect, bool keepRatio)
{
    if (!image || !m_initialized || !m_graphics)
    {
        return;
    }
    Gdiplus::Bitmap* pBitmap = static_cast<Gdiplus::Bitmap*>(image->bitmap());
    if (!pBitmap || pBitmap->GetLastStatus() != Gdiplus::Ok)
    {
        return;
    }

    Gdiplus::Rect src(0, 0, pBitmap->GetWidth(), pBitmap->GetHeight());
    Gdiplus::Rect dst(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
    if (keepRatio)
    {
        int w = rect.right - rect.left;
        int h = rect.bottom - rect.top;
        int bw = pBitmap->GetWidth();
        int bh = pBitmap->GetHeight();
        double d = ((double)w) / h;
        double bd = ((double)bw) / bh;
        if (bd > d)
        {
            // image is too wide
            bw = w;
            bh = (int)(bw / bd);
        }
        else if (bd < d)
        {
            // image is too high
            bh = h;
            bw = (int)(bd * bh);
        }
        dst.X = (w - bw) / 2;
        dst.Y = (h - bh) / 2;
        dst.Width = bw;
        dst.Height = bh;
    }
    m_graphics->DrawImage(pBitmap, dst, src.X, src.Y, src.Width, src.Height, Gdiplus::UnitPixel);
}

Image* RendererGDIPlus::loadImageFromPath(const wchar_t* path)
{
    if (!path || !m_initialized)
    {
        return nullptr;
    }
    Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromFile(path);
    if (!pBitmap || pBitmap->GetLastStatus() != Gdiplus::Ok)
    {
        if (pBitmap)
        {
            delete pBitmap;
        }
        return nullptr;
    }

    ImageGDIPlus* image = new ImageGDIPlus;
    image->m_bitmap = pBitmap;
    image->decode();
    return image;
}

Image* RendererGDIPlus::loadImageFromStream(const BYTE* data, int size)
{
    if (!data || size <= 0 || !m_initialized)
    {
        return nullptr;
    }
    IStream* pStream = NULL;
    pStream = SHCreateMemStream((const BYTE*)data, size);
    Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromStream(pStream);
    if (!pBitmap || pBitmap->GetLastStatus() != Gdiplus::Ok)
    {
        if (pBitmap)
        {
            delete pBitmap;
        }
        pStream->Release();
        return nullptr;
    }

    ImageGDIPlus* image = new ImageGDIPlus;
    image->m_bitmap = pBitmap;
    image->decode();
    pStream->Release();
    return image;
}

Image* RendererGDIPlus::loadImageFromResource(HINSTANCE hInstance, UINT resourceId, const wchar_t* resourceType)
{
    if (!resourceType || !m_initialized)
    {
        return nullptr;
    }
    IStream* pStream = loadResourceToStream(hInstance, resourceId, resourceType);
    if (pStream)
    {
        Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromStream(pStream);
        if (!pBitmap || pBitmap->GetLastStatus() != Gdiplus::Ok)
        {
            if (pBitmap)
            {
                delete pBitmap;
            }
            freeStream(pStream);
            return nullptr;
        }

        ImageGDIPlus* image = new ImageGDIPlus;
        image->m_bitmap = pBitmap;
        image->decode();
        freeStream(pStream);
        return image;
    }
    return nullptr;
}

Image* RendererGDIPlus::createImageFromBGColor(const COLORREF& color, BYTE alpha, int width, int height)
{
    if (width <= 0 || height <= 0 || !m_initialized)
    {
        return nullptr;
    }

    Gdiplus::Bitmap* pBitmap = new Gdiplus::Bitmap(width, height, PixelFormat32bppPARGB);
    if (!pBitmap || pBitmap->GetLastStatus() != Gdiplus::Ok)
    {
        if (pBitmap)
        {
            delete pBitmap;
        }
        return nullptr;
    }

    Gdiplus::Graphics graphics(pBitmap);
    Gdiplus::Rect rect(0, 0, width, height);
    Gdiplus::SolidBrush brush(Gdiplus::Color(alpha, GetRValue(color), GetGValue(color), GetBValue(color)));
    graphics.FillRectangle(&brush, rect);

    ImageGDIPlus* image = new ImageGDIPlus;
    image->m_bitmap = pBitmap;
    return image;
}

Image* RendererGDIPlus::createImageFromBGImage(const Image* bgImage, BYTE alpha, int width, int height, WrapMode mode)
{
    if (!bgImage || width <= 0 || height <= 0)
    {
        return nullptr;
    }
    Gdiplus::Bitmap* bgBitmap = static_cast<Gdiplus::Bitmap*>(bgImage->bitmap());
    if (!bgBitmap || bgBitmap->GetLastStatus() != Gdiplus::Ok)
    {
        return nullptr;
    }

    Gdiplus::Bitmap* pBitmap = new Gdiplus::Bitmap(width, height, PixelFormat32bppPARGB);
    if (!pBitmap || pBitmap->GetLastStatus() != Gdiplus::Ok)
    {
        if (pBitmap)
        {
            delete pBitmap;
        }
        return nullptr;
    }

    Gdiplus::RectF rcDrawRect;
    rcDrawRect.X = 0.0;
    rcDrawRect.Y = 0.0;
    rcDrawRect.Width = (float)width;
    rcDrawRect.Height = (float)height;
    Gdiplus::Graphics* graphics = Gdiplus::Graphics::FromImage(pBitmap);
    if (!graphics)
    {
        delete pBitmap;
        return nullptr;
    }
#if 0 // too slow
    graphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
#endif
    if (graphics->GetLastStatus() != Gdiplus::Ok)
    {
        delete pBitmap;
        return nullptr;
    }

    Gdiplus::ColorMatrix colorMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, alpha / 255.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f
    };

    Gdiplus::ImageAttributes imgAtt;
    imgAtt.SetColorMatrix(&colorMatrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);
    switch (mode)
    {
    case STRETCH:
        graphics->DrawImage(bgBitmap, rcDrawRect, 0.0, 0.0, (float)bgBitmap->GetWidth(), (float)bgBitmap->GetHeight(), Gdiplus::UnitPixel, &imgAtt);
        break;
    case TILE:
        imgAtt.SetWrapMode(Gdiplus::WrapModeTile);
        graphics->DrawImage(bgBitmap, rcDrawRect, 0.0, 0.0, (float)width, (float)height, Gdiplus::UnitPixel, &imgAtt);
        break;
    case TILEFLIP:
        imgAtt.SetWrapMode(Gdiplus::WrapModeTileFlipXY);
        graphics->DrawImage(bgBitmap, rcDrawRect, 0.0, 0.0, (float)width, (float)height, Gdiplus::UnitPixel, &imgAtt);
        break;
    default:
        graphics->DrawImage(bgBitmap, rcDrawRect, 0.0, 0.0, (float)bgBitmap->GetWidth(), (float)bgBitmap->GetHeight(), Gdiplus::UnitPixel, &imgAtt);
        break;
    }

    delete graphics;
    ImageGDIPlus* image = new ImageGDIPlus;
    image->m_bitmap = pBitmap;
    return image;
}

bool RendererGDIPlus::createDeviceResources()
{
    if (!m_graphics || !m_memBitmap)
    {
        m_memBitmap = new Gdiplus::Bitmap(m_rect.right - m_rect.left, m_rect.bottom - m_rect.top, PixelFormat32bppPARGB);
        if (!m_memBitmap || m_memBitmap->GetLastStatus() != Gdiplus::Ok)
        {
            return false;
        }

        m_graphics = new Gdiplus::Graphics(m_memBitmap);
        if (!m_graphics)
        {
            return false;
        }
        m_graphics->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
#if 1
        m_graphics->SetSmoothingMode(Gdiplus::SmoothingModeNone);
        m_graphics->SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
#else
        m_graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        m_graphics->SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit);
#endif
        if (m_graphics->GetLastStatus() != Gdiplus::Ok)
        {
            return false;
        }
    }
    else
    {
        clear(0);
    }
    return true;
}

void RendererGDIPlus::releaseDeviceResources()
{
    if (m_memBitmap)
    {
        delete m_memBitmap;
        m_memBitmap = nullptr;
    }
    if (m_graphics)
    {
        delete m_graphics;
        m_graphics = nullptr;
    }
}

bool RendererGDIPlus::createTextResources()
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

            // Create Font
            tr.font = createFont(s.font);
            if (!tr.font || tr.font->GetLastStatus() != Gdiplus::Ok)
            {
                return false;
            }

            tr.textBrush = new Gdiplus::SolidBrush(Gdiplus::Color(GetAValue(s.textColor), GetRValue(s.textColor), GetGValue(s.textColor), GetBValue(s.textColor)));
            if (!tr.textBrush || tr.textBrush->GetLastStatus() != Gdiplus::Ok)
            {
                return false;
            }

            if (s.haveBackgroundColor)
            {
                tr.bgBrush = new Gdiplus::SolidBrush(Gdiplus::Color(GetAValue(s.backgroundColor), GetRValue(s.backgroundColor), GetGValue(s.backgroundColor), GetBValue(s.backgroundColor)));
                if (!tr.bgBrush || tr.bgBrush->GetLastStatus() != Gdiplus::Ok)
                {
                    return false;
                }
            }

            tr.stringFormat = new Gdiplus::StringFormat(Gdiplus::StringFormat::GenericTypographic());
            if (!tr.stringFormat)
            {
                return false;
            }
            tr.stringFormat->SetAlignment(Gdiplus::StringAlignmentNear);
            tr.stringFormat->SetLineAlignment(Gdiplus::StringAlignmentNear);
            tr.stringFormat->SetTrimming(Gdiplus::StringTrimmingNone);
            tr.stringFormat->SetFormatFlags(Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsNoClip | Gdiplus::StringFormatFlagsMeasureTrailingSpaces);
            if (tr.stringFormat->GetLastStatus() != Gdiplus::Ok)
            {
                return false;
            }
        }
    }
    return true;
}

void RendererGDIPlus::releaseTextResources()
{
    for (auto& tr : m_trs)
    {
        if (tr.font)
        {
            delete tr.font;
        }
        if (tr.textBrush)
        {
            delete tr.textBrush;
        }
        if (tr.bgBrush)
        {
            delete tr.bgBrush;
        }
        if (tr.stringFormat)
        {
            delete tr.stringFormat;
        }
    }
    m_trs.clear();
}

bool RendererGDIPlus::createLayeredDC()
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

void RendererGDIPlus::releaseLayeredDC()
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

RendererGDIPlus::TextResource* RendererGDIPlus::textResource(int index)
{
    if (index >= 0 && index < static_cast<int>(m_trs.size()))
    {
        return &m_trs[index];
    }
    return nullptr;
}

Gdiplus::Font* RendererGDIPlus::createFont(const LOGFONTW& logfont)
{
    Gdiplus::FontStyle style = Gdiplus::FontStyleRegular;
    if (logfont.lfItalic)
    {
        style = static_cast<Gdiplus::FontStyle>(style | Gdiplus::FontStyleItalic);
    }
    if (logfont.lfWeight >= FW_BOLD)
    {
        style = static_cast<Gdiplus::FontStyle>(style | Gdiplus::FontStyleBold);
    }
    if (logfont.lfUnderline)
    {
        style = static_cast<Gdiplus::FontStyle>(style | Gdiplus::FontStyleUnderline);
    }
    if (logfont.lfStrikeOut)
    {
        style = static_cast<Gdiplus::FontStyle>(style | Gdiplus::FontStyleStrikeout);
    }

    Gdiplus::REAL fontSize = static_cast<Gdiplus::REAL>(std::abs(logfont.lfHeight));
    if (fontSize <= 0)
    {
        fontSize = 12.0f;
    }

    Gdiplus::FontFamily family(logfont.lfFaceName);
    if (family.GetLastStatus() != Gdiplus::Ok)
    {
        return new Gdiplus::Font(Gdiplus::FontFamily::GenericSansSerif(), fontSize, style, Gdiplus::UnitPixel);
    }

    Gdiplus::Font* font = new Gdiplus::Font(&family, fontSize, style, Gdiplus::UnitPixel);
    if (!font || font->GetLastStatus() != Gdiplus::Ok)
    {
        delete font;
        font = new Gdiplus::Font(Gdiplus::FontFamily::GenericSansSerif(), fontSize, style, Gdiplus::UnitPixel);
    }
    return font;
}