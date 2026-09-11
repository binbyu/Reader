#ifndef __WEB_BOOK_H__
#define __WEB_BOOK_H__

#ifdef ENABLE_NETWORK

#include "Book.h"
#include <string>

/*
 * Web 阅读：通过局域网里的 legado(开源阅读) web 服务读书
 *
 *   GET  {server}/getBookshelf                            -> 书架（含 durChapterIndex/durChapterPos 进度）
 *   GET  {server}/getChapterList?url={bookUrl}            -> 章节列表
 *   GET  {server}/getBookContent?url={bookUrl}&index={n}  -> 章节正文（UTF-8 纯文本）
 *   POST {server}/saveBookProgress                        -> 回写进度（手机端 legado 会更新书架进度）
 *
 * 网络层用系统自带的 WinHTTP，而不是工程里原来的 libhttps：
 * 这个版本的 libhttps 处理不了带端口的 URL（http://host:80/ 直接失败），
 * 而 legado web 服务恰恰跑在 192.168.x.x:1122 这种地址上。
 *
 * 一本书 = 一个本地 .web 暂存文件（JSON 元数据），路径作为书籍身份，进入阅读列表后可再次打开。
 * 正文一次只加载当前章节，进度以 chapter_index + chapter_pos 记录，与手机端 durChapterIndex /
 * durChapterPos 一一对应（chapter_pos 就是正文里的字符偏移，正文不做任何重排，保证偏移一致）。
 */

// WM_BOOK_EVENT 的 wParam（仅 WebBook 自己处理，取不冲突的值）
#define WEB_BE_CONTENT          301
#define WEB_BE_CONTENT_FAIL     302

// .web 暂存文件内容，也是从书架到 Reader 传递的书籍信息
typedef struct web_book_meta_t
{
    std::string server;         // http://192.168.1.100:1122
    std::string book_url;       // legado 的 bookUrl
    std::string name;
    std::string author;
    int         chapter_index;  // 手机端 durChapterIndex
    int         chapter_pos;    // 手机端 durChapterPos（章节内字符偏移）
    long long   chapter_time;   // 手机端 durChapterTime（毫秒），用于判断谁更新
    long long   fetch_time;     // 本软件最近一次与手机同步的时间（毫秒）
    std::string chapter_title;
} web_book_meta_t;

// 同步 HTTP 请求（WinHTTP）。url 为 UTF-8；body/bodylen 可空。
// 返回 HTTP 状态码，0 表示连接失败。成功时 *out_body 为 malloc 的响应体（NUL 结尾，调用方 free）。
int WebHttpRequest(const char* method, const char* url, const char* body, int bodylen,
                   char** out_body, int* out_len, int timeout_ms);

class WebBook : public Book
{
public:
    WebBook();
    virtual ~WebBook();

public:
    virtual book_type_t GetBookType(void);
    virtual BOOL SaveBook(HWND hWnd);
    virtual BOOL UpdateChapters(int offset);
    virtual BOOL IsLoading(void);
    virtual void JumpChapter(HWND hWnd, int index);
    virtual void JumpPrevChapter(HWND hWnd);
    virtual void JumpNextChapter(HWND hWnd);
    virtual int GetCurChapterIndex(void);
    virtual LRESULT OnBookEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    // 把当前进度写回 .web 文件，并同步给手机（force=FALSE 时按 5 秒节流）
    void SyncProgress(HWND hWnd, BOOL force);
    // 节流窗口结束后补推一次（由定时器调用），保证最后一次阅读位置也会同步到手机
    void FlushProgress(void);

protected:
    virtual BOOL ParserBook(HWND hWnd);
    virtual BOOL OnDrawPageEvent(HWND hWnd);
    virtual BOOL OnUpDownEvent(HWND hWnd, int draw_type);

private:
    BOOL LoadMeta(void);
    BOOL SaveMeta(void);
    BOOL GetChapterList(void);
    wchar_t* GetChapterContent(int index, int* len);
    BOOL ApplyChapter(HWND hWnd, int index, int pos, wchar_t* text, int len);
    void RequestChapter(HWND hWnd, int index, int pos);
    void RefreshProgressFromServer(void);
    void PostProgress(BOOL wait_done);
    std::string BuildProgressJson(void);
    std::string BaseUrl(const char* api) const;
    std::string ContentUrl(int index) const;

private:
    web_book_meta_t m_Meta;
    int   m_CurIndex;       // 当前章节序号
    int   m_PendingPos;     // >=0 时在下次绘制前套用（章节内字符偏移）
    int   m_PosInChapter;   // 当前页面在章节内的偏移
    int   m_SavedPos;       // 已落盘的偏移
    int   m_SavedIndex;     // 已落盘的章节
    DWORD m_LastSyncTick;   // 上次同步给手机的时刻（节流用）
    BOOL  m_IsLoading;
    int   m_FetchSeq;       // 换章请求序号：只接受最新一次的结果
    int   m_FetchingIndex;
};

// 由书架对话框调用：写好 .web 文件后走正常的打开流程（param = web_book_meta_t*）
void OnOpenWebBook(HWND hWnd, void* param);

#endif // ENABLE_NETWORK
#endif // __WEB_BOOK_H__
