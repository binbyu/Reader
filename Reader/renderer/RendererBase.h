#pragma once
#include "IRenderer.h"
#include <unordered_map>
#include <list>
#include <functional>
#include <string>

#define GetAValue(argb) (LOBYTE((argb)>>24))

template <typename K, typename V, typename Hash = std::hash<K>>
class LRUCache
{
public:
    using ListType = std::list<std::pair<K, V>>;
    using MapType = std::unordered_map<K, typename ListType::iterator, Hash>;
    using CleanupFunc = std::function<void(V&)>;

    LRUCache(size_t maxCapacity = 1024, CleanupFunc cleanup = nullptr)
        : m_capacity(maxCapacity), m_cleanup(cleanup) {}
    ~LRUCache() { clear(); }

    void setCapacity(size_t capacity) { m_capacity = capacity; trim(); }
    void setCleanup(CleanupFunc cleanup) { m_cleanup = cleanup; }

    bool get(const K& key, V& value)
    {
        auto it = m_map.find(key);
        if (it == m_map.end()) return false;
        m_list.splice(m_list.begin(), m_list, it->second);
        value = it->second->second;
        return true;
    }

    V* find(const K& key)
    {
        auto it = m_map.find(key);
        if (it == m_map.end()) return nullptr;
        m_list.splice(m_list.begin(), m_list, it->second);
        return &(it->second->second);
    }

    void put(const K& key, const V& value)
    {
        auto it = m_map.find(key);
        if (it != m_map.end())
        {
            if (m_cleanup) m_cleanup(it->second->second);
            it->second->second = value;
            m_list.splice(m_list.begin(), m_list, it->second);
            return;
        }
        if (m_list.size() >= m_capacity && !m_list.empty())
        {
            auto& last = m_list.back();
            if (m_cleanup) m_cleanup(last.second);
            m_map.erase(last.first);
            m_list.pop_back();
        }
        m_list.push_front({ key, value });
        m_map[key] = m_list.begin();
    }

    void clear()
    {
        if (m_cleanup)
        {
            for (auto& item : m_list) m_cleanup(item.second);
        }
        m_list.clear();
        m_map.clear();
    }

    size_t size() const { return m_list.size(); }
    bool empty() const { return m_list.empty(); }

private:
    void trim()
    {
        while (m_list.size() > m_capacity && !m_list.empty())
        {
            auto& last = m_list.back();
            if (m_cleanup) m_cleanup(last.second);
            m_map.erase(last.first);
            m_list.pop_back();
        }
    }

    size_t m_capacity;
    CleanupFunc m_cleanup;
    ListType m_list;
    MapType m_map;
};

struct CharShadowKey
{
    wchar_t ch;
    int styleIndex;
    int w;
    int h;
    bool operator==(const CharShadowKey& o) const
    {
        return ch == o.ch && styleIndex == o.styleIndex && w == o.w && h == o.h;
    }
};

struct CharShadowKeyHash
{
    std::size_t operator()(const CharShadowKey& k) const noexcept
    {
        std::size_t h1 = static_cast<std::size_t>(k.ch);
        std::size_t h2 = static_cast<std::size_t>(k.styleIndex);
        std::size_t h3 = (static_cast<std::size_t>(k.w) << 16) | (static_cast<std::size_t>(k.h) & 0xFFFF);
        return h1 ^ (h2 * 2654435761u) ^ (h3 * 2246822519u);
    }
};

struct CharMeasureKey
{
    wchar_t ch;
    int maxWidth;
    int textStyleIndex;
    bool operator==(const CharMeasureKey& o) const
    {
        return ch == o.ch && maxWidth == o.maxWidth && textStyleIndex == o.textStyleIndex;
    }
};

struct CharMeasureKeyHash
{
    std::size_t operator()(const CharMeasureKey& k) const noexcept
    {
        std::size_t h1 = static_cast<std::size_t>(k.ch);
        std::size_t h2 = static_cast<std::size_t>(k.maxWidth);
        std::size_t h3 = static_cast<std::size_t>(k.textStyleIndex);
        return h1 ^ (h2 * 2654435761u) ^ (h3 * 2246822519u);
    }
};

class RendererBase;
class MeasurerBase : public IMeasurer
{
public:
    MeasurerBase(RendererBase* renderer);
    virtual ~MeasurerBase() = default;
    virtual TextMetrics measureText(const wchar_t* text, int length, int maxWidth, int textStyleIndex) override;
    void clearCache();

protected:
    virtual TextMetrics measureTextImpl(const wchar_t* text, int length, int maxWidth, int textStyleIndex) = 0;

protected:
    RendererBase* m_renderer;
    LRUCache<CharMeasureKey, TextMetrics, CharMeasureKeyHash> m_cache;
};

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
    virtual bool createShadowResources(int needW, int needH) = 0;
    virtual void releaseShadowResources() = 0;
    virtual bool isSameSize(const RECT& rect);
    virtual void normalizeRect(RECT& rect);

protected:
    int  shadowPadding(float blur, float offsetX, float offsetY);
    void blurShadowMask(BYTE* mask, BYTE* temp, int width, int height, float blur);
    void colorizeShadowMask(const BYTE* mask, DWORD* dstPBGRA, int width, int height, COLORREF shadowColor);

protected:
    HWND m_hWnd = nullptr;
    HDC  m_hdc = nullptr;
    RECT m_rect = { 0 };
    TextStyles m_styles;
    bool m_isLayered = false;
    bool m_clearType = false;
    bool m_isBlend = false;

    // for shadow
    std::vector<BYTE>  m_shadowMaskBuf;
    std::vector<BYTE>  m_shadowTempBuf;
    std::vector<DWORD> m_shadowPixelBuf;
};
