#pragma once

#include "ILayout.h"

class LayoutBase : public ILayout
{
public:
    LayoutBase();
    virtual ~LayoutBase() = default;
    virtual void setMeasurer(IMeasurer* measurer);
    virtual void setSource(ISource* source);
    virtual void setTextStyle(const TextStyleConfig& styles);

protected:
    bool isValid();
    LayoutConfig toPhysical(const LayoutConfig& cfg) const;

protected:
    IMeasurer*      m_measurer;
    ISource*        m_source;
    TextStyleConfig m_styles;
};