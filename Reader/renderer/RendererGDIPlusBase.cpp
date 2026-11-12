#include "RendererGDIPlusBase.h"

/********************************************************************
*  ImageGDIPlus                                                     *
********************************************************************/
ImageGDIPlus::ImageGDIPlus()
    : m_bitmap(nullptr)
    , m_frameCount(-1)
    , m_frameIndex(0)
    , m_hasAlpha(false)
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
    if (m_bitmap)
    {
        m_bitmap->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hBitmap);
    }
    return hBitmap;
}

bool ImageGDIPlus::hasAlpha() const
{
    return m_hasAlpha;
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
            std::vector<GUID> dimensionIDs(count);
            m_bitmap->GetFrameDimensionsList(dimensionIDs.data(), count);
            m_frameCount = m_bitmap->GetFrameCount(&dimensionIDs[0]);
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
        m_hasAlpha = Gdiplus::IsAlphaPixelFormat(m_bitmap->GetPixelFormat());
        Gdiplus::Bitmap* decoded = m_bitmap->Clone(0, 0, m_bitmap->GetWidth(), m_bitmap->GetHeight(), PixelFormat32bppPARGB);
        if (decoded && Gdiplus::Ok == decoded->GetLastStatus())
        {
            delete m_bitmap;
            m_bitmap = decoded;
        }
        else if (decoded)
        {
            delete decoded;
        }
    }
}


/********************************************************************
*  ContextGDIPlus                                                   *
********************************************************************/
ContextGDIPlus* ContextGDIPlus::instance()
{
    static ContextGDIPlus _inst;
    return &_inst;
}

ContextGDIPlus::ContextGDIPlus()
{
}

ContextGDIPlus::~ContextGDIPlus()
{
    cleanup();
}

bool ContextGDIPlus::initialize()
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

void ContextGDIPlus::cleanup()
{
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


/********************************************************************
*  RendererGDIPlusBase                                              *
********************************************************************/
RendererGDIPlusBase::RendererGDIPlusBase()
    : RendererBase()
{
    ContextGDIPlus::instance()->initialize();
}

RendererGDIPlusBase::~RendererGDIPlusBase()
{
}

Image* RendererGDIPlusBase::loadImageFromPath(const wchar_t* path)
{
    if (!path) return nullptr;

    Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromFile(path);
    if (!pBitmap || pBitmap->GetLastStatus() != Gdiplus::Ok)
    {
        if (pBitmap) delete pBitmap;
        return nullptr;
    }

    ImageGDIPlus* image = new ImageGDIPlus;
    image->m_bitmap = pBitmap;
    image->decode();
    return image;
}

Image* RendererGDIPlusBase::loadImageFromStream(const BYTE* data, int size)
{
    if (!data || size <= 0) return nullptr;

    IStream* pStream = SHCreateMemStream((const BYTE*)data, size);
    if (!pStream) return nullptr;
    Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromStream(pStream);
    if (!pBitmap || pBitmap->GetLastStatus() != Gdiplus::Ok)
    {
        if (pBitmap) delete pBitmap;
        pStream->Release();
        return nullptr;
    }

    ImageGDIPlus* image = new ImageGDIPlus;
    image->m_bitmap = pBitmap;
    image->decode();
    pStream->Release();
    return image;
}

Image* RendererGDIPlusBase::loadImageFromResource(HINSTANCE hInstance, UINT resourceId, const wchar_t* resourceType)
{
    if (!resourceType) return nullptr;

    IStream* pStream = loadResourceToStream(hInstance, resourceId, resourceType);
    if (!pStream) return nullptr;
    Gdiplus::Bitmap* pBitmap = Gdiplus::Bitmap::FromStream(pStream);
    if (!pBitmap || pBitmap->GetLastStatus() != Gdiplus::Ok)
    {
        if (pBitmap) delete pBitmap;
        freeStream(pStream);
        return nullptr;
    }

    ImageGDIPlus* image = new ImageGDIPlus;
    image->m_bitmap = pBitmap;
    image->decode();
    freeStream(pStream);
    return image;
}

Image* RendererGDIPlusBase::createImageFromBGColor(const COLORREF& color, BYTE alpha, int width, int height)
{
    if (width <= 0 || height <= 0) return nullptr;

    Gdiplus::Bitmap* pBitmap = new Gdiplus::Bitmap(width, height, PixelFormat32bppPARGB);
    if (!pBitmap || pBitmap->GetLastStatus() != Gdiplus::Ok)
    {
        if (pBitmap) delete pBitmap;
        return nullptr;
    }

    Gdiplus::Graphics graphics(pBitmap);
    Gdiplus::Rect rect(0, 0, width, height);
    Gdiplus::SolidBrush brush(Gdiplus::Color(alpha, GetRValue(color), GetGValue(color), GetBValue(color)));
    graphics.FillRectangle(&brush, rect);

    ImageGDIPlus* image = new ImageGDIPlus;
    image->m_bitmap = pBitmap;
    image->m_hasAlpha = alpha < 0xff;
    return image;
}

Image* RendererGDIPlusBase::createImageFromBGImage(const Image* bgImage, BYTE alpha, int width, int height, WrapMode mode)
{
    if (!bgImage || width <= 0 || height <= 0) return nullptr;

    Gdiplus::Bitmap* bgBitmap = static_cast<Gdiplus::Bitmap*>(bgImage->bitmap());
    if (!bgBitmap || bgBitmap->GetLastStatus() != Gdiplus::Ok) return nullptr;

    Gdiplus::Bitmap* pBitmap = new Gdiplus::Bitmap(width, height, PixelFormat32bppPARGB);
    if (!pBitmap || pBitmap->GetLastStatus() != Gdiplus::Ok)
    {
        if (pBitmap) delete pBitmap;
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
        delete graphics;
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
    image->m_hasAlpha = (alpha < 0xff) || (bgImage && bgImage->hasAlpha());
    return image;
}
