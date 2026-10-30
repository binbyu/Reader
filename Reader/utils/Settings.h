#pragma once
#include "types.h"
#include <thread>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <memory>
#include <functional>

class Settings
{
public:
    static Settings& instance();
    void  setHwnd(HWND hWnd);
    void  save(int delayms = 0);
    Data& data();
    void  defaultData(Data& data);
    void  defaultKeys(KeysetInfo& keyset);
    void  defaultFont(LOGFONTW& font);
    void  restoreDefault();
    void  toLogical(Data& data);
    void  toPhysical(Data& data);

private:
    Settings();
    ~Settings();
    void saveReal();
    void load();
    void flush();
    void writer();
    void notify(DWORD error);
    void fixedWindowInfo(WindowInfo& window);
    std::wstring fileName();

private:
    std::thread m_thread;
    Data m_data;
    std::shared_ptr<Data> m_pending;
    HWND m_hWnd;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::vector<uint8_t> m_bytes;
    bool m_stop;
};