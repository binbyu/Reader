#pragma once
#include <vector>
#include "types.h"

class IMeasurer;

class ISource;

class ILayout
{
public:
    virtual          ~ILayout() = default;
    virtual void     setMeasurer(IMeasurer* measurer) = 0;
    virtual void     setSource(ISource* source) = 0;
    virtual void     setTextStyle(const TextStyleConfig& styles) = 0;
    virtual int      findPrevPageIndex(int end, int width, int height) = 0;
    virtual int      findPrevLineIndex(int end, int n, int width) = 0;
    virtual PageInfo layout(int start, int width, int height) = 0;
};