#pragma once
#include "IRenderer.h"

#define GetAValue(argb) (LOBYTE((argb)>>24))

class RendererBase : public IRenderer
{
protected:
    RendererBase();
    virtual ~RendererBase();
    IStream* loadResourceToStream(HINSTANCE hInstance, UINT resourceId, const wchar_t* resourceType);
    void freeStream(IStream* stream);

protected:
    virtual bool createDeviceResources() = 0;
    virtual void releaseDeviceResources() = 0;
    virtual bool createTextResources() = 0;
    virtual void releaseTextResources() = 0;

protected:
    HWND m_hWnd = nullptr;
    HDC  m_hdc = nullptr;
    RECT m_rect = { 0 };
    TextStyles m_styles;
    bool m_isLayered = false;
	bool m_isBlend = false;
};
