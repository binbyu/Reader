#include "LayoutBase.h"
#include "LayoutText.h"
#include "Settings.h"
#include "DpiAwareness.h"

LayoutBase::LayoutBase()
    : m_measurer(nullptr)
    , m_source(nullptr)
    , m_styles({0})
{
}

void LayoutBase::setMeasurer(IMeasurer* measurer)
{
    m_measurer = measurer;
}

void LayoutBase::setSource(ISource* source)
{
    m_source = source;
}

void LayoutBase::setTextStyle(const TextStyleConfig& styles)
{
    m_styles = styles;
}

bool LayoutBase::isValid()
{
    return m_measurer
        && m_source
        && m_styles.textIdx >= 0
        && m_styles.titleIdx >= 0
#if ENABLE_TAG
        && m_styles.tagIdx.size() == Setting::instance().data().tags.size()
#endif
        ;
}

LayoutConfig LayoutBase::toPhysical(const LayoutConfig& cfg) const
{
    LayoutConfig pcfg = cfg;
    DpiAwareness::instance().logicalToPhysical(cfg.borderL, pcfg.borderL);
    DpiAwareness::instance().logicalToPhysical(cfg.borderT, pcfg.borderT);
    DpiAwareness::instance().logicalToPhysical(cfg.borderR, pcfg.borderR);
    DpiAwareness::instance().logicalToPhysical(cfg.borderB, pcfg.borderB);
    DpiAwareness::instance().logicalToPhysical(cfg.charGap, pcfg.charGap);
    DpiAwareness::instance().logicalToPhysical(cfg.lineGap, pcfg.lineGap);
    DpiAwareness::instance().logicalToPhysical(cfg.paragraphGap, pcfg.paragraphGap);
    return pcfg;
}