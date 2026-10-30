#pragma once
#include <string>
#include <vector>
#include "types.h"

enum EngineType
{
    D2D,
    GDIPLUS,
};

class Image
{
public:
    virtual        ~Image() = default;
    virtual int     width() const = 0;
    virtual int     height() const = 0;
    virtual void*   bitmap() const = 0;
    virtual int     frameCount() const = 0;
    virtual int     frameIndex() const = 0;
    virtual int     frameDelay(int index) const = 0;
    virtual bool    activeFrame(int index) = 0;
    virtual HBITMAP getHBITMAP() const = 0;
};

class IMeasurer
{
public:
    virtual ~IMeasurer() = default;
    virtual TextMetrics measureText(const wchar_t* text, int length, int maxWidth, int textStyleIndex) = 0;
};

class IRenderer
{
public:
    static IRenderer* instance();

public:
    virtual ~IRenderer() = default;

    virtual bool initialize() = 0;
    virtual void cleanup() = 0;
    virtual EngineType type() = 0;
    virtual IMeasurer* measurer() = 0;

    // if hdc is nullptr, is Layered Window
    virtual bool beginPaint(HWND hWnd, HDC hdc, const RECT& rect, const TextStyles& styles, bool blend = false) = 0;
    virtual void endPaint() = 0;

    virtual void clear(const COLORREF& color) = 0;
    virtual void fillRect(const COLORREF& color, const RECT& rect) = 0;
    virtual void fillRect(const Image* image, const RECT& rect, WrapMode mode) = 0;
    virtual void drawText(const wchar_t* text, int length, const RECT& rect, int textStyleIndex) = 0;
    virtual void drawImage(const Image* image, const RECT& rect, bool keepRatio = false) = 0;

    virtual Image* loadImageFromPath(const wchar_t* path) = 0;
    virtual Image* loadImageFromStream(const BYTE* data, int size) = 0;
    virtual Image* loadImageFromResource(HINSTANCE hInstance, UINT resourceId, const wchar_t* resourceType) = 0;
    virtual Image* createImageFromBGColor(const COLORREF& color, BYTE alpha, int width, int height) = 0;
    virtual Image* createImageFromBGImage(const Image* image, BYTE alpha, int width, int height, WrapMode mode) = 0;
};
