#include "RendererBase.h"
#include "RendererGDIPlus.h"
#include "RendererD2D.h"
#include "Logger.h"

//#pragma comment(lib, "shlwapi.lib")

IRenderer* IRenderer::instance()
{
    static IRenderer* _inst = nullptr;
    if (!_inst)
    {
        RendererD2D* pD2D = new RendererD2D;
        if (pD2D->initialize())
        {
            lginfo("[Init] Using Direct2D");
            _inst = pD2D;
        }
        else
        {
            delete pD2D;
            RendererGDIPlus* pGDIPlus = new RendererGDIPlus;
            if (pGDIPlus->initialize())
            {
                lginfo("[Init] Using GDI+");
                _inst = pGDIPlus;
            }
            else
            {
                lgerror("[Init] Not support");
                delete pGDIPlus;
            }
        }
    }
    return _inst;
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