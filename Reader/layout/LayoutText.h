#pragma once
#include "LayoutBase.h"
#include <vector>

class LayoutText : public LayoutBase
{
private:
    template <typename T>
    class ArrayStack
    {
    public:
        explicit ArrayStack(size_t capacity) { m_data.reserve(capacity); }
        ~ArrayStack() = default;
        ArrayStack(const ArrayStack &) = default;
        ArrayStack &operator=(const ArrayStack &) = default;
        ArrayStack(ArrayStack &&) = default;
        ArrayStack &operator=(ArrayStack &&) = default;

        bool     empty() const { return m_data.empty(); }
        void     push(const T &value) { m_data.push_back(value); }
        void     push(T &&value) { m_data.push_back(std::move(value)); }
        void     pop() { if (empty()) return; m_data.pop_back(); }
        T&       top() { return m_data.back(); }
        const T& top() const { return m_data.back(); }
        void     reset() { m_data.clear(); }
        size_t   size() const { return m_data.size(); }
        size_t   capacity() const { return m_data.capacity();}
    private:
        std::vector<T> m_data;
    };

public:
    LayoutText();
    virtual          ~LayoutText();
    virtual int      findPrevPageIndex(int end, int width, int height);
    virtual int      findPrevLineIndex(int end, int n, int width);
    virtual PageInfo layout(int start, int width, int height);

protected:
    bool paragraphToLines(int start, int length, int renderW, std::vector<LineInfo>& lines);
    void initCharStack(int start, int length);
    int  calcIndentWidth() const;
    int  emptyLineHeight() const;
    int  maxSearchLength() const;
#if ENABLE_TAG
    int  tagIndex(int index);
#endif
    void validateLines(const wchar_t* text, int start, int length, int renderW, int cGap, const std::vector<LineInfo>& lines) const;

protected:
    ArrayStack<CharInfo> m_stack;
    LayoutConfig m_config;
};