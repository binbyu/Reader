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