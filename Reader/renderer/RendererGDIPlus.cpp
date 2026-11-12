#pragma once
#include "RendererGDIPlus.h"
#include "Settings.h"

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
    : RendererGDIPlusBase()
{
}

RendererGDIPlus::~RendererGDIPlus()
{
    if (m_measurer)
    {
        delete m_measurer;
        m_measurer = nullptr;
    }

    releaseTextResources();
    releaseDeviceResources();
    releaseLayeredDC();
}

RendererType RendererGDIPlus::type()
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

bool RendererGDIPlus::beginPaint(HWND hWnd, HDC hdc, const RECT& rc, const TextStyles& styles, bool clearType, bool blend)
{
    if (!hWnd/* || styles.empty()*/)
    {
        return false;
    }

    if (!Settings::instance().data().clearType) clearType = false;

    RECT rect = rc;
    normalizeRect(rect);

    if (m_styles != styles)
    {
        releaseTextResources();
    }
    if (!isSameSize(rect) || m_clearType != clearType)
    {
        releaseDeviceResources();
    }

    m_hWnd = hWnd;
    m_hdc = hdc;
    m_rect = rect;
    m_styles = styles;
    m_clearType = clearType;
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
        m_memBitmap->GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &hBitmap);
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
    if (!m_graphics)
    {
        return;
    }

    m_graphics->Clear(Gdiplus::Color(GetAValue(color), GetRValue(color), GetGValue(color), GetBValue(color)));
}

void RendererGDIPlus::fillRect(const COLORREF& color, const RECT& rect)
{
    if (!m_graphics)
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
    if (!image || !m_graphics)
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
    if (!text || !m_graphics)
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
    if (!image || !m_graphics)
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
        //m_graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        if (m_clearType) m_graphics->SetTextRenderingHint(Gdiplus::TextRenderingHintClearTypeGridFit); // !!! Clear Type not support alpha backgroud
        else             m_graphics->SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
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

            // create Font
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
    Gdiplus::Font* font = new Gdiplus::Font(m_hdc, &logfont);
    if (font)
    {
        if (font->GetLastStatus() == Gdiplus::Ok) return font;
        delete font;
        font = nullptr;
    }

    font = new Gdiplus::Font(Gdiplus::FontFamily::GenericSansSerif(), 12.0f, Gdiplus::FontStyleRegular, Gdiplus::UnitPixel);
    if (font)
    {
        if (font->GetLastStatus() == Gdiplus::Ok) return font;
        delete font;
    }
    return nullptr;
}
