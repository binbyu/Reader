#pragma once
#include "RendererGDI.h"
#include "Settings.h"

/********************************************************************
*  MeasurerGDI                                                      *
********************************************************************/
MeasurerGDI::MeasurerGDI(RendererGDI* r)
    : MeasurerBase(r)
{
}

TextMetrics MeasurerGDI::measureTextImpl(const wchar_t* text, int length, int maxWidth, int textStyleIndex)
{
    TextMetrics metrics = { 0, 0, 0 };
    if (!text || length <= 0) return metrics;

    RendererGDI* rnd = static_cast<RendererGDI*>(m_renderer);
    if (!rnd || !rnd->m_textDC) return metrics;

    RendererGDI::TextResource* tr = rnd->textResource(textStyleIndex);
    if (!tr || !tr->hFont) return metrics;

    HFONT hOldFont = (HFONT)SelectObject(rnd->m_textDC, tr->hFont);
    SIZE size = { 0 };
    GetTextExtentPoint32W(rnd->m_textDC, text, length, &size);
    SelectObject(rnd->m_textDC, hOldFont);

    metrics.width = size.cx;
    metrics.height = size.cy;
    metrics.count = length;
    return metrics;
}


/********************************************************************
*  RendererGDI                                                      *
********************************************************************/
RendererGDI::RendererGDI()
    : RendererGDIPlusBase()
{
}

RendererGDI::~RendererGDI()
{
    if (m_measurer)
    {
        delete m_measurer;
        m_measurer = nullptr;
    }

    m_charShadowCache.clear();
    releaseShadowResources();
    releaseTextResources();
    releaseDeviceResources();
    releaseLayeredDC();
}

RendererType RendererGDI::type()
{
    return GDI;
}

IMeasurer* RendererGDI::measurer()
{
    if (!m_measurer)
    {
        m_measurer = new MeasurerGDI(this);
    }
    return m_measurer;
}

bool RendererGDI::beginPaint(HWND hWnd, HDC hdc, const RECT& rc, const TextStyles& styles, bool clearType, bool blend)
{
    if (!ContextGDIPlus::instance()->initialize() || !hWnd)
    {
        return false;
    }

    if (!Settings::instance().data().clearType) clearType = false;

    RECT rect = rc;
    normalizeRect(rect);

    if (m_styles != styles || m_clearType != clearType)
    {
        releaseShadowResources();
        releaseTextResources();
    }
    if (!isSameSize(rect))
    {
        releaseDeviceResources();
    }

    m_hWnd = hWnd;
    m_hdc = hdc;
    m_rect = rect;
    m_styles = styles;
    m_clearType = clearType;
    m_isBlend = blend;
#if !ALWAYS_BLEND
    m_textDCUsed = false;
#endif

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
    releaseShadowResources();
    releaseTextResources();
    releaseDeviceResources();
    releaseLayeredDC();
    return false;
}

void RendererGDI::endPaint()
{
    if (m_memDC)
    {
        // alpha blend text to backgroud image
#if !ALWAYS_BLEND
        if (m_textDC && m_textDCUsed)
#else
        if (m_textDC)
#endif
        {
            int w = m_rect.right - m_rect.left;
            int h = m_rect.bottom - m_rect.top;

            BLENDFUNCTION blend = { 0 };
            blend.BlendOp = AC_SRC_OVER;
            blend.BlendFlags = 0;
            blend.SourceConstantAlpha = 0xFF; 
            blend.AlphaFormat = AC_SRC_ALPHA;
            AlphaBlend(m_memDC, 0, 0, w, h, m_textDC, 0, 0, w, h, blend);
        }

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
            UpdateLayeredWindow(m_hWnd, NULL, &ptPos, &sizeWnd, m_memDC, &ptSrc, 0, &blend, ULW_ALPHA);
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
                AlphaBlend(m_hdc, x, y, w, h, m_memDC, 0, 0, w, h, blend);
            }
            else
            {
                BitBlt(m_hdc, x, y, w, h, m_memDC, 0, 0, SRCCOPY);
            }
        }
    }

    //releaseShadowResources();
    //releaseTextResources();
    //releaseDeviceResources();
    releaseLayeredDC();
}

void RendererGDI::clear(const COLORREF& color)
{
    if (!m_memDC || !m_memBitmap)
    {
        return;
    }

    int w = m_rect.right - m_rect.left;
    int h = m_rect.bottom - m_rect.top;
    int count = w * h;

    BYTE a = GetAValue(color);
    if (a == 0 && !m_isLayered && !m_isBlend)
    {
        a = 255;
    }

    if (m_memBits && (a < 255 || m_isLayered || m_isBlend))
    {
        DWORD pixel = premultiplyPixel(GetRValue(color), GetGValue(color), GetBValue(color), a);
        DWORD* bits = (DWORD*)m_memBits;
        if (pixel == 0)
        {
            memset(bits, 0, count * sizeof(DWORD));
        }
        else
        {
            DWORD* end = bits + count;
            while (bits < end) *bits++ = pixel;
        }
    }
    else
    {
        HBRUSH hBrush = CreateSolidBrush(color & 0x00FFFFFF);
        RECT rc = { 0, 0, w, h };
        ::FillRect(m_memDC, &rc, hBrush);
        DeleteObject(hBrush);
    }

    if (m_textBits)
    {
        memset(m_textBits, 0, count * sizeof(DWORD));
    }
}

void RendererGDI::fillRect(const COLORREF& color, const RECT& rect)
{
    if (!m_memDC)
    {
        return;
    }

    BYTE a = GetAValue(color);

    if (m_memBits && (a < 255 || m_isLayered || m_isBlend))
    {
        int bmpW = m_rect.right - m_rect.left;
        DWORD source = premultiplyPixel(GetRValue(color), GetGValue(color), GetBValue(color), a);

        int x0 = max(rect.left, 0L);
        int y0 = max(rect.top, 0L);
        int x1 = min(rect.right, m_rect.right - m_rect.left);
        int y1 = min(rect.bottom, m_rect.bottom - m_rect.top);

        for (int y = y0; y < y1; ++y)
        {
            DWORD* row = (DWORD*)(m_memBits + y * bmpW * 4);
            for (int x = x0; x < x1; ++x)
            {
                blendPremultipliedPixel(row[x], source);
            }
        }
    }
    else
    {
        HBRUSH hBrush = CreateSolidBrush(color & 0x00FFFFFF);
        ::FillRect(m_memDC, &rect, hBrush);
        DeleteObject(hBrush);
    }
}

void RendererGDI::fillRect(const Image* image, const RECT& rect, WrapMode mode)
{
    if (!image || !m_memDC)
    {
        return;
    }

    if (mode == STRETCH)
    {
        drawImage(image, rect, false);
        return;
    }

    HBITMAP hSrcBmp = image->getHBITMAP();
    if (!hSrcBmp)
    {
        return;
    }

    int dstW = rect.right - rect.left;
    int dstH = rect.bottom - rect.top;
    int srcW = image->width();
    int srcH = image->height();

    if (srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0)
    {
        DeleteObject(hSrcBmp);
        return;
    }

    HDC hdcSrc = CreateCompatibleDC(m_memDC);
    HBITMAP hOldSrc = (HBITMAP)SelectObject(hdcSrc, hSrcBmp);

    for (int y = 0; y < dstH; y += srcH)
    {
        for (int x = 0; x < dstW; x += srcW)
        {
            int tileW = min(srcW, dstW - x);
            int tileH = min(srcH, dstH - y);

            if (mode == TILEFLIP)
            {
                int tileX = x / srcW;
                int tileY = y / srcH;
                bool flipH = (tileX % 2) != 0;
                bool flipV = (tileY % 2) != 0;

                if (flipH || flipV)
                {
                    int srcX = flipH ? (srcW - 1) : 0;
                    int srcY = flipV ? (srcH - 1) : 0;
                    int stretchW = flipH ? -tileW : tileW;
                    int stretchH = flipV ? -tileH : tileH;
                    StretchBlt(m_memDC, rect.left + x, rect.top + y, tileW, tileH, hdcSrc, srcX, srcY, stretchW, stretchH, SRCCOPY);
                    continue;
                }
            }

            BitBlt(m_memDC, rect.left + x, rect.top + y, tileW, tileH, hdcSrc, 0, 0, SRCCOPY);
        }
    }

    SelectObject(hdcSrc, hOldSrc);
    DeleteDC(hdcSrc);
    DeleteObject(hSrcBmp);
}

void RendererGDI::drawText(const wchar_t* text, int length, const RECT& rect, int textStyleIndex)
{
    if (!text || !m_memDC || !m_textDC || !m_textBits) return;

    if (length < 0)
    {
        length = 0;
        const wchar_t* p = text;
        while (*p++) length++;
    }
    if (length == 0) return;

    TextResource* tr = textResource(textStyleIndex);
    if (!tr || !tr->hFont) return;

#if !ALWAYS_BLEND
    if (m_isLayered || m_isBlend || (tr->haveBgColor && GetAValue(tr->bgColor) < 0xff) || GetAValue(tr->textColor) < 0xff || tr->shadow.enable)
    {
#endif
        int width = m_rect.right - m_rect.left;
        int x0 = max(rect.left, 0L);
        int y0 = max(rect.top, 0L);
        int x1 = min(rect.right, m_rect.right - m_rect.left);
        int y1 = min(rect.bottom, m_rect.bottom - m_rect.top);
        if (x0 >= x1 || y0 >= y1)
        {
            return;
        }

        if (tr->haveBgColor)
        {
            fillRect(tr->bgColor, rect);
        }

        for (int y = y0; y < y1; ++y)
        {
            DWORD* row = (DWORD*)(m_textBits + y * width * 4);
            for (int x = x0; x < x1; ++x)
            {
                row[x] = 0;
            }
        }

        if (tr->shadow.enable)
        {
            const int textW = rect.right - rect.left;
            const int textH = rect.bottom - rect.top;
            const int pad = shadowPadding(tr->shadow.blur, tr->shadow.offsetX, tr->shadow.offsetY);
            const int shadowW = textW + pad * 2;
            const int shadowH = textH + pad * 2;

            bool drawnFromCache = false;
            if (length == 1)
            {
                CharShadowGDI* val = m_charShadowCache.find({ text[0], textStyleIndex, textW, textH });
                if (val)
                {
                    if (!val->pixels.empty() && createShadowResources(val->sw, val->sh))
                    {
                        DWORD* shadowPixels = reinterpret_cast<DWORD*>(m_shadowBits);
                        for (int y = 0; y < val->sh; ++y)
                        {
                            memcpy(shadowPixels + y * m_shadowCapW, val->pixels.data() + y * val->sw, val->sw * sizeof(DWORD));
                        }
                        BLENDFUNCTION shadowBlend = { 0 };
                        shadowBlend.BlendOp = AC_SRC_OVER;
                        shadowBlend.SourceConstantAlpha = 0xFF;
                        shadowBlend.AlphaFormat = AC_SRC_ALPHA;
                        AlphaBlend(m_textDC,
                            rect.left - val->pad + static_cast<int>(tr->shadow.offsetX),
                            rect.top  - val->pad + static_cast<int>(tr->shadow.offsetY),
                            val->sw, val->sh,
                            m_shadowDC, 0, 0, val->sw, val->sh, shadowBlend);
                    }
                    drawnFromCache = true;
                }
            }

            if (!drawnFromCache)
            {
                if (shadowW > 0 && shadowH > 0 && createShadowResources(shadowW, shadowH))
                {
                    const size_t total = static_cast<size_t>(shadowW) * shadowH;
                    DWORD* shadowPixels = reinterpret_cast<DWORD*>(m_shadowBits);
                    for (int y = 0; y < shadowH; ++y)
                    {
                        DWORD* row = shadowPixels + y * m_shadowCapW;
                        for (int x = 0; x < shadowW; ++x) row[x] = 0x00FFFFFF;
                    }

                    HFONT hOldShadowFont = (HFONT)SelectObject(m_shadowDC, tr->hFont);
                    SetTextColor(m_shadowDC, RGB(0, 0, 0));
                    SetBkMode(m_shadowDC, TRANSPARENT);
                    TextOutW(m_shadowDC, pad, pad, text, length);
                    SelectObject(m_shadowDC, hOldShadowFont);

                    if (m_shadowMaskBuf.size() < total) m_shadowMaskBuf.resize(total);
                    if (m_shadowTempBuf.size() < total) m_shadowTempBuf.resize(total);
                    bool hasAlpha = false;
                    for (int y = 0; y < shadowH; ++y)
                    {
                        const DWORD* srcRow = shadowPixels + y * m_shadowCapW;
                        BYTE* dstRow = m_shadowMaskBuf.data() + y * shadowW;
                        for (int x = 0; x < shadowW; ++x)
                        {
                            BYTE cov = textCoverage(srcRow[x]);
                            dstRow[x] = cov;
                            if (cov > 0) hasAlpha = true;
                        }
                    }

                    if (hasAlpha)
                    {
                        blurShadowMask(m_shadowMaskBuf.data(), m_shadowTempBuf.data(), shadowW, shadowH, tr->shadow.blur);
                        for (int y = 0; y < shadowH; ++y)
                        {
                            colorizeShadowMask(m_shadowMaskBuf.data() + y * shadowW, shadowPixels + y * m_shadowCapW, shadowW, 1, tr->shadow.color);
                        }

                        if (length == 1)
                        {
                            std::vector<DWORD> cachedPix(total);
                            for (int y = 0; y < shadowH; ++y)
                            {
                                memcpy(cachedPix.data() + y * shadowW, shadowPixels + y * m_shadowCapW, shadowW * sizeof(DWORD));
                            }
                            m_charShadowCache.put({ text[0], textStyleIndex, textW, textH }, { std::move(cachedPix), pad, shadowW, shadowH });
                        }

                        BLENDFUNCTION shadowBlend = { 0 };
                        shadowBlend.BlendOp = AC_SRC_OVER;
                        shadowBlend.SourceConstantAlpha = 0xFF;
                        shadowBlend.AlphaFormat = AC_SRC_ALPHA;
                        AlphaBlend(m_textDC,
                            rect.left - pad + static_cast<int>(tr->shadow.offsetX),
                            rect.top  - pad + static_cast<int>(tr->shadow.offsetY),
                            shadowW, shadowH,
                            m_shadowDC, 0, 0, shadowW, shadowH, shadowBlend);
                    }
                    else if (length == 1)
                    {
                        m_charShadowCache.put({ text[0], textStyleIndex, textW, textH }, { {}, pad, shadowW, shadowH });
                    }
                }
            }

            if (shadowW > 0 && shadowH > 0 && createShadowResources(shadowW, shadowH))
            {
                DWORD* shadowPixels = reinterpret_cast<DWORD*>(m_shadowBits);
                for (int y = 0; y < shadowH; ++y)
                {
                    DWORD* row = shadowPixels + y * m_shadowCapW;
                    for (int x = 0; x < shadowW; ++x)
                    {
                        row[x] = 0x00FFFFFF;
                    }
                }
                HFONT hOldShadowFont = (HFONT)SelectObject(m_shadowDC, tr->hFont);
                SetTextColor(m_shadowDC, RGB(0, 0, 0));
                SetBkMode(m_shadowDC, TRANSPARENT);
                TextOutW(m_shadowDC, pad, pad, text, length);
                SelectObject(m_shadowDC, hOldShadowFont);

                BYTE a = GetAValue(tr->textColor);
                BYTE r = GetRValue(tr->textColor);
                BYTE g = GetGValue(tr->textColor);
                BYTE b = GetBValue(tr->textColor);
                if (a == 0) a = 0xff;
                for (int y = y0; y < y1; ++y)
                {
                    DWORD* dstRow = reinterpret_cast<DWORD*>(m_textBits + y * width * 4);
                    const DWORD* maskRow = shadowPixels + (y - rect.top + pad) * m_shadowCapW;
                    for (int x = x0; x < x1; ++x)
                    {
                        BYTE coverage = textCoverage(maskRow[x - rect.left + pad]);
                        BYTE alpha = static_cast<BYTE>((coverage * a + 127) / 255);
                        blendPremultipliedPixel(dstRow[x], premultiplyPixel(r, g, b, alpha));
                    }
                }
#if !ALWAYS_BLEND
                m_textDCUsed = true;
#endif
                return;
            }
        }

        // The regular text path uses white as the no-coverage value.
        for (int y = y0; y < y1; ++y)
        {
            DWORD* row = (DWORD*)(m_textBits + y * width * 4);
            for (int x = x0; x < x1; ++x)
            {
                row[x] = 0x00FFFFFF;
            }
        }

        HFONT hOldFont = (HFONT)SelectObject(m_textDC, tr->hFont);
        SetTextColor(m_textDC, RGB(0, 0, 0));
        SetBkMode(m_textDC, TRANSPARENT);
        TextOutW(m_textDC, rect.left, rect.top, text, length);
        SelectObject(m_textDC, hOldFont);

        BYTE a = GetAValue(tr->textColor);
        BYTE r = GetRValue(tr->textColor);
        BYTE g = GetGValue(tr->textColor);
        BYTE b = GetBValue(tr->textColor);
        if (a == 0) a = 0xff;

        for (int y = y0; y < y1; ++y)
        {
            DWORD* row = (DWORD*)(m_textBits + y * width * 4);
            for (int x = x0; x < x1; ++x)
            {
                DWORD maskPixel = row[x];
                row[x] = 0;
                BYTE coverage = textCoverage(maskPixel);
                BYTE alpha = (BYTE)((coverage * a + 127) / 255);
                blendPremultipliedPixel(row[x], premultiplyPixel(r, g, b, alpha));
            }
        }
#if !ALWAYS_BLEND
        m_textDCUsed = true;
    }
    else
    {
        HFONT hOldFont = (HFONT)SelectObject(m_memDC, tr->hFont);
        SetTextColor(m_memDC, tr->textColor & 0x00ffffff);

        if (tr->haveBgColor)
        {
            SetBkColor(m_memDC, tr->bgColor & 0x00ffffff);
            SetBkMode(m_memDC, OPAQUE);
        }
        else
        {
            SetBkMode(m_memDC, TRANSPARENT);
        }

        TextOutW(m_memDC, rect.left, rect.top, text, length);
        SelectObject(m_memDC, hOldFont);
    }
#endif
}

void RendererGDI::drawImage(const Image* image, const RECT& rect, bool keepRatio)
{
    if (!image || !m_memDC)
    {
        return;
    }

    HBITMAP hSrcBmp = image->getHBITMAP();
    if (!hSrcBmp)
    {
        return;
    }

    int srcW = image->width();
    int srcH = image->height();
    int dstX = rect.left;
    int dstY = rect.top;
    int dstW = rect.right - rect.left;
    int dstH = rect.bottom - rect.top;

    if (keepRatio && srcW > 0 && srcH > 0)
    {
        double d = ((double)dstW) / dstH;
        double bd = ((double)srcW) / srcH;
        int bw, bh;
        if (bd > d)
        {
            bw = dstW;
            bh = (int)(bw / bd);
        }
        else if (bd < d)
        {
            bh = dstH;
            bw = (int)(bd * bh);
        }
        else
        {
            bw = dstW;
            bh = dstH;
        }
        dstX = rect.left + (dstW - bw) / 2;
        dstY = rect.top + (dstH - bh) / 2;
        dstW = bw;
        dstH = bh;
    }

    HDC hdcSrc = CreateCompatibleDC(m_memDC);
    HBITMAP hOldSrc = (HBITMAP)SelectObject(hdcSrc, hSrcBmp);

    BLENDFUNCTION bf = { 0 };
    bf.BlendOp = AC_SRC_OVER;
    bf.BlendFlags = 0;
    bf.SourceConstantAlpha = 0xFF;
    bf.AlphaFormat = AC_SRC_ALPHA;
    AlphaBlend(m_memDC, dstX, dstY, dstW, dstH, hdcSrc, 0, 0, srcW, srcH, bf);

    SelectObject(hdcSrc, hOldSrc);
    DeleteDC(hdcSrc);
    DeleteObject(hSrcBmp);
}

bool RendererGDI::createDeviceResources()
{
    if (!m_memDC || !m_memBitmap)
    {
        m_memDC = CreateCompatibleDC(m_hdc);
        if (!m_memDC)
        {
            return false;
        }

        int w = m_rect.right - m_rect.left;
        int h = m_rect.bottom - m_rect.top;

        BITMAPINFO bmi = { 0 };
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = w;
        bmi.bmiHeader.biHeight = -h;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        m_memBitmap = CreateDIBSection(m_memDC, &bmi, DIB_RGB_COLORS, (void**)&m_memBits, nullptr, 0);
        if (!m_memBitmap)
        {
            releaseDeviceResources();
            return false;
        }
        m_oldBitmap = (HBITMAP)SelectObject(m_memDC, m_memBitmap);

        m_textDC = CreateCompatibleDC(m_hdc);
        if (!m_textDC)
        {
            releaseDeviceResources();
            return false;
        }
        m_textBitmap = CreateDIBSection(m_textDC, &bmi, DIB_RGB_COLORS, (void**)&m_textBits, nullptr, 0);
        if (!m_textBitmap)
        {
            releaseDeviceResources();
            return false;
        }
        m_oldTextBitmap = (HBITMAP)SelectObject(m_textDC, m_textBitmap);
        clear(0);
    }
    else
    {
        clear(0);
    }
    return true;
}

void RendererGDI::releaseDeviceResources()
{
    if (m_textDC && m_oldTextBitmap)
    {
        SelectObject(m_textDC, m_oldTextBitmap);
        m_oldTextBitmap = nullptr;
    }
    if (m_textBitmap)
    {
        DeleteObject(m_textBitmap);
        m_textBitmap = nullptr;
    }
    if (m_textDC)
    {
        DeleteDC(m_textDC);
        m_textDC = nullptr;
    }
    m_textBits = nullptr;

    if (m_memDC && m_oldBitmap)
    {
        SelectObject(m_memDC, m_oldBitmap);
        m_oldBitmap = nullptr;
    }
    if (m_memBitmap)
    {
        DeleteObject(m_memBitmap);
        m_memBitmap = nullptr;
    }
    if (m_memDC)
    {
        DeleteDC(m_memDC);
        m_memDC = nullptr;
    }
    m_memBits = nullptr;
}

bool RendererGDI::createTextResources()
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
            TextStyle s = m_styles[i];
            TextResource& tr = m_trs[i];

            if (m_clearType) s.font.lfQuality = CLEARTYPE_NATURAL_QUALITY;
         // else             s.font.lfQuality = ANTIALIASED_QUALITY;
            tr.hFont = CreateFontIndirectW(&s.font);
            if (!tr.hFont)
            {
                return false;
            }

            tr.textColor = s.textColor;
            tr.haveBgColor = s.haveBackgroundColor;
            tr.bgColor = s.backgroundColor;
            tr.shadow = s.shadow;
        }
    }
    return true;
}

void RendererGDI::releaseTextResources()
{
    for (auto& tr : m_trs)
    {
        if (tr.hFont)
        {
            DeleteObject(tr.hFont);
            tr.hFont = nullptr;
        }
    }
    m_trs.clear();
    m_charShadowCache.clear();
    if (m_measurer) m_measurer->clearCache();
}

bool RendererGDI::createShadowResources(int needW, int needH)
{
    if (needW <= 0 || needH <= 0)
    {
        return false;
    }

    if (m_shadowDC && m_shadowBitmap && m_shadowBits &&
        needW <= m_shadowCapW && needH <= m_shadowCapH)
    {
        return true;
    }

    const int targetW = (std::max)(needW, m_shadowCapW);
    const int targetH = (std::max)(needH, m_shadowCapH);
    releaseShadowResources();

    m_shadowDC = CreateCompatibleDC(m_hdc);
    if (!m_shadowDC)
    {
        releaseShadowResources();
        return false;
    }

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = targetW;
    bmi.bmiHeader.biHeight = -targetH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    m_shadowBitmap = CreateDIBSection(m_shadowDC, &bmi, DIB_RGB_COLORS,
        reinterpret_cast<void**>(&m_shadowBits), nullptr, 0);
    if (!m_shadowBitmap || !m_shadowBits)
    {
        releaseShadowResources();
        return false;
    }

    m_oldShadowBitmap = (HBITMAP)SelectObject(m_shadowDC, m_shadowBitmap);
    if (!m_oldShadowBitmap)
    {
        releaseShadowResources();
        return false;
    }

    m_shadowCapW = targetW;
    m_shadowCapH = targetH;
    return true;
}

void RendererGDI::releaseShadowResources()
{
    m_charShadowCache.clear();
    if (m_shadowDC && m_oldShadowBitmap)
    {
        SelectObject(m_shadowDC, m_oldShadowBitmap);
        m_oldShadowBitmap = nullptr;
    }
    if (m_shadowBitmap)
    {
        DeleteObject(m_shadowBitmap);
        m_shadowBitmap = nullptr;
    }
    if (m_shadowDC)
    {
        DeleteDC(m_shadowDC);
        m_shadowDC = nullptr;
    }
    m_shadowBits = nullptr;
    m_shadowCapW = 0;
    m_shadowCapH = 0;
}

bool RendererGDI::createLayeredDC()
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

void RendererGDI::releaseLayeredDC()
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

inline DWORD RendererGDI::premultiplyPixel(BYTE red, BYTE green, BYTE blue, BYTE alpha)
{
    BYTE premultipliedRed = (BYTE)((red * alpha + 127) / 255);
    BYTE premultipliedGreen = (BYTE)((green * alpha + 127) / 255);
    BYTE premultipliedBlue = (BYTE)((blue * alpha + 127) / 255);
    return (alpha << 24) | (premultipliedRed << 16) | (premultipliedGreen << 8) | premultipliedBlue;
}

inline void RendererGDI::blendPremultipliedPixel(DWORD& destination, DWORD source)
{
    BYTE alpha = (BYTE)(source >> 24);
    if (alpha == 0) return;

    BYTE inverseAlpha     = 255 - alpha;
    BYTE destinationAlpha = (BYTE)(destination >> 24);
    BYTE sourceRed        = (BYTE)(source >> 16);
    BYTE sourceGreen      = (BYTE)(source >> 8);
    BYTE sourceBlue       = (BYTE)source;
    BYTE destinationRed   = (BYTE)(destination >> 16);
    BYTE destinationGreen = (BYTE)(destination >> 8);
    BYTE destinationBlue  = (BYTE)destination;

    BYTE outputAlpha = alpha + (destinationAlpha * inverseAlpha + 127) / 255;
    BYTE outputRed   = sourceRed + (destinationRed * inverseAlpha + 127) / 255;
    BYTE outputGreen = sourceGreen + (destinationGreen * inverseAlpha + 127) / 255;
    BYTE outputBlue  = sourceBlue + (destinationBlue * inverseAlpha + 127) / 255;

    destination = (outputAlpha << 24) | (outputRed << 16) | (outputGreen << 8) | outputBlue;
}

inline BYTE RendererGDI::textCoverage(DWORD pixel)
{
    BYTE red      = (BYTE)(pixel >> 16);
    BYTE green    = (BYTE)(pixel >> 8);
    BYTE blue     = (BYTE)pixel;
    int luminance = (red * 299 + green * 587 + blue * 114 + 500) / 1000;
    return (BYTE)(255 - luminance);
}

RendererGDI::TextResource* RendererGDI::textResource(int index)
{
    if (index >= 0 && index < static_cast<int>(m_trs.size()))
    {
        return &m_trs[index];
    }
    return nullptr;
}
