#include "RendererBase.h"
#include "RendererGDIPlus.h"
#include "RendererD2D.h"
#include "RendererGDI.h"
#include "Settings.h"

//#pragma comment(lib, "shlwapi.lib")

IRenderer* IRenderer::instance()
{
    static IRenderer* _inst = nullptr;

    if (_inst && _inst->type() != Settings::instance().data().rendererType)
    {
        delete _inst;
        _inst = nullptr;
    }

    if (!_inst)
    {
        RendererType preferredType = Settings::instance().data().rendererType;
        std::vector<RendererType> rendererTypes = { D2D, GDI, GDIPLUS };

        auto it = std::find(rendererTypes.begin(), rendererTypes.end(), preferredType);
        if (it != rendererTypes.end()) std::rotate(rendererTypes.begin(), it, it + 1);  // move preferredType to first

        for (auto i : rendererTypes)
        {
            switch (i)
            {
            case RendererType::GDIPLUS:
                if (ContextGDIPlus::instance()->initialize())
                {
                    _inst = new RendererGDIPlus();
                }
                break;
            case RendererType::GDI:
                if (ContextGDIPlus::instance()->initialize())
                {
                    _inst = new RendererGDI();
                }
                break;
            default:
                if (ContextD2D::instance()->initialize())
                {
                    _inst = new RendererD2D();
                }
                break;
            }

            if (!_inst) continue;
            break;
        }

        if (_inst && _inst->type() != preferredType)
        {
            Settings::instance().data().rendererType = _inst->type();
            Settings::instance().save();
        }
    }
    return _inst;
}

std::vector<RendererType> IRenderer::supports()
{
    std::vector<RendererType> list;

    if (ContextD2D::instance()->initialize())
    {
        list.push_back(RendererType::D2D);
    }

    if (ContextGDIPlus::instance()->initialize())
    {
        list.push_back(RendererType::GDIPLUS);
        list.push_back(RendererType::GDI);
    }
    return list;
}

void IRenderer::switchto(RendererType type)
{
    auto& oldType = Settings::instance().data().rendererType;
    if (oldType != type)
    {
        Settings::instance().data().rendererType = type;
        Settings::instance().save();
        instance();
    }
}

RendererBase::RendererBase()
{
}

RendererBase::~RendererBase()
{
}

IStream* RendererBase::loadResourceToStream(HINSTANCE hInstance, UINT resourceId, const wchar_t* resourceType)
{
    if (!hInstance || resourceId == 0)
    {
        return nullptr;
    }

    HRSRC hResInfo = FindResourceW(hInstance, MAKEINTRESOURCEW(resourceId), resourceType);
    if (!hResInfo)
    {
        return nullptr;
    }

    HGLOBAL hResData = LoadResource(hInstance, hResInfo);
    if (!hResData)
    {
        return nullptr;
    }

    const void* pResLockedData = LockResource(hResData);
    if (!pResLockedData)
    {
        FreeResource(hResData);
        return nullptr;
    }

    DWORD resDataSize = SizeofResource(hInstance, hResInfo);
    if (0 == resDataSize)
    {
        FreeResource(hResData);
        return nullptr;
    }

    HGLOBAL hMemory = GlobalAlloc(GMEM_MOVEABLE, resDataSize);
    if (!hMemory)
    {
        FreeResource(hResData);
        return nullptr;
    }

    void* memBufer = GlobalLock(hMemory);
    if (!memBufer)
    {
        FreeResource(hResData);
        GlobalFree(hMemory);
        return nullptr;
    }

    CopyMemory(memBufer, pResLockedData, resDataSize);
    GlobalUnlock(hMemory);
    FreeResource(hResData);

    IStream* pStream = nullptr;
    if (CreateStreamOnHGlobal(hMemory, TRUE /* free by FreeStream() */, &pStream) != S_OK)
    {
        GlobalFree(hMemory);
        return nullptr;
    }

    return pStream;
}

void RendererBase::freeStream(IStream* stream)
{
    if (stream)
    {
        stream->Release();
    }
}

bool RendererBase::isSameSize(const RECT& rect)
{
    int oldWidth = m_rect.right - m_rect.left;
    int oldHeight = m_rect.bottom - m_rect.top;
    int newWidth = rect.right - rect.left;
    int newHeight = rect.bottom - rect.top;
    return oldWidth == newWidth && oldHeight == newHeight;
}

void RendererBase::normalizeRect(RECT& rect)
{
    if (rect.left > rect.right)
    {
        LONG temp = rect.left;
        rect.left = rect.right;
        rect.right = temp;
    }
    if (rect.top > rect.bottom)
    {
        LONG temp = rect.top;
        rect.top = rect.bottom;
        rect.bottom = temp;
    }
}

int RendererBase::shadowPadding(float blur, float offsetX, float offsetY)
{
    int maxOffset = (int)(ceilf((std::max)(fabsf(offsetX), fabsf(offsetY))));
    return (int)(ceilf(blur * 2.0f)) + maxOffset + 2;
}

void RendererBase::blurShadowMask(BYTE* mask, BYTE* temp, int width, int height, float blur)
{
    if (!mask || !temp || width <= 0 || height <= 0 || blur <= 0.01f) return;

    float sigma = blur * 0.55f;
    int r = (int)floorf(sqrtf(4.0f * sigma * sigma + 1.0f) * 0.5f);
    if (r < 1) r = 1;
    if (r >= width) r = width - 1;
    if (r >= height) r = height - 1;

    int win = 2 * r + 1;
    uint32_t invWin = (1u << 24) / (uint32_t)win;
    uint32_t half = (uint32_t)win / 2;

    int blurCount = (blur < 1.5f) ? 1 : (blur < 4.5f ? 2 : 3);
    for (int pass = 0; pass < blurCount; ++pass)
    {
        for (int y = 0; y < height; ++y)
        {
            const BYTE* sRow = mask + y * width;
            BYTE* dRow = temp + y * width;
            int sum = sRow[0] * (r + 1);
            for (int i = 1; i <= r; ++i) sum += sRow[(std::min)(i, width - 1)];

            for (int x = 0; x < width; ++x)
            {
                dRow[x] = (BYTE)(((uint64_t)((uint32_t)sum + half) * invWin) >> 24);
                int addIdx = (std::min)(x + r + 1, width - 1);
                int subIdx = (std::max)(x - r, 0);
                sum += sRow[addIdx] - sRow[subIdx];
            }
        }

        for (int x = 0; x < width; ++x)
        {
            int sum = temp[x] * (r + 1);
            for (int i = 1; i <= r; ++i) sum += temp[(std::min)(i, height - 1) * width + x];

            for (int y = 0; y < height; ++y)
            {
                mask[y * width + x] = (BYTE)(((uint64_t)((uint32_t)sum + half) * invWin) >> 24);
                int addIdx = (std::min)(y + r + 1, height - 1);
                int subIdx = (std::max)(y - r, 0);
                sum += temp[addIdx * width + x] - temp[subIdx * width + x];
            }
        }
    }
}

void RendererBase::colorizeShadowMask(const BYTE* mask, DWORD* dstPBGRA, int width, int height, COLORREF shadowColor)
{
    if (!mask || !dstPBGRA || width <= 0 || height <= 0) return;

    BYTE baseA = GetAValue(shadowColor);
    if (baseA == 0) baseA = 255;
    const COLORREF effectiveColor = (shadowColor & 0x00FFFFFF) | ((DWORD)baseA << 24);

    static COLORREF lastColor = 0;
    static bool hasLastColor = false;
    static DWORD lut[256] = { 0 };

    if (!hasLastColor || effectiveColor != lastColor)
    {
        lastColor = effectiveColor;
        hasLastColor = true;
        const int r = GetRValue(effectiveColor), g = GetGValue(effectiveColor), b = GetBValue(effectiveColor);
        for (int cov = 1; cov < 256; ++cov)
        {
            const int a = (cov * baseA + 127) / 255;
            const int pr = (r * a + 127) / 255, pg = (g * a + 127) / 255, pb = (b * a + 127) / 255;
            lut[cov] = ((DWORD)a << 24) | ((DWORD)pr << 16) | ((DWORD)pg << 8) | (DWORD)pb;
        }
    }

    const int count = width * height;
    for (int i = 0; i < count; ++i) dstPBGRA[i] = lut[mask[i]];
}

MeasurerBase::MeasurerBase(RendererBase* renderer)
    : m_renderer(renderer), m_cache(1024)
{
}

TextMetrics MeasurerBase::measureText(const wchar_t* text, int length, int maxWidth, int textStyleIndex)
{
    if (!text) return { 0, 0, 0 };
    if (length < 0)
    {
        length = 0;
        const wchar_t* p = text;
        while (*p++) length++;
    }
    if (length == 0) return { 0, 0, 0 };

    if (length == 1)
    {
        CharMeasureKey key{ text[0], maxWidth, textStyleIndex };
        TextMetrics* cached = m_cache.find(key);
        if (cached) return *cached;
        TextMetrics tm = measureTextImpl(text, length, maxWidth, textStyleIndex);
        m_cache.put(key, tm);
        return tm;
    }
    return measureTextImpl(text, length, maxWidth, textStyleIndex);
}

void MeasurerBase::clearCache()
{
    m_cache.clear();
}