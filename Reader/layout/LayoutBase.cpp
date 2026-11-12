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
        && m_styles.tagIdx.size() == Settings::instance().data().tags.size()
#endif
        ;
}
