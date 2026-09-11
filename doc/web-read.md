# Web 阅读（局域网 legado / 开源阅读 web 服务）

在 Reader 里直接读手机端「阅读(legado)」的书架，并和手机双向同步阅读进度。

## 怎么用

1. 手机上打开 legado → 我的 → **Web 服务**（阅读 web 服务），确认端口（例如 1122）。
2. PC 与手机连同一个局域网。
3. Reader 菜单：**设置 → Web 阅读**，填入地址（例如 `http://192.168.1.100:1122`），点「连接」。
   - 地址可以省略 `http://`，也可以带结尾 `/`，程序会自动补齐/去掉。
   - 地址会记到 `<exe目录>\.web\server.txt`，下次打开自动连接；也可以点「网页版」用浏览器打开手机的那套网页界面。
4. 列表里选中一本书，双击书名（或选中后按「打开」）即可阅读。
   - **搜索**：拿到书架后光标会停在搜索框，直接打字即可即时筛选（大小写不敏感，支持空格分隔多个
     关键词、全部命中才算；匹配 书名 / 作者 / 当前章节 / 最新章节）。筛完会自动选中第一行，
     **按回车直接打开第一本**。点「清空」回到全部。
   - 列表按「最近阅读」排序，最后一列「来源」显示这本书是网络书源（显示主机名）还是**手机本地书**。
   - **手机本地书也能读**：`origin=loc_book` 的书（`bookUrl` 形如 `content://…` 或
     `/storage/emulated/0/…`）同样走 `/getChapterList` + `/getBookContent`，正文由手机端解析后传回来，
     阅读与进度同步和网络书一个流程（实测 784 章的手机本地 txt 正常打开并续读到手机上读到的第 230 章）。
5. 阅读时进度自动同步：
   - 打开书时，若手机上的进度更新，就从手机的位置接着读；
   - PC 上翻页/滚动后，本地立即记录，**5 秒节流**回写一次手机（切章、退出时立即回写）。

## 用到的接口（手机 web 服务）

| 用途 | 请求 |
|---|---|
| 书架 + 进度 | `GET {server}/getBookshelf` → `data[]`，字段 `name/author/bookUrl/origin/durChapterIndex/durChapterPos/durChapterTime/durChapterTitle/totalChapterNum` |
| 章节列表 | `GET {server}/getChapterList?url={bookUrl}` |
| 章节正文 | `GET {server}/getBookContent?url={bookUrl}&index={n}` → `data` 是纯文本（UTF-8） |
| 回写进度 | `POST {server}/saveBookProgress`，JSON：`{name, author, durChapterIndex, durChapterPos, durChapterTime, durChapterTitle}` |

进度字段语义与手机端一致：`durChapterIndex` 是章节序号（从 0 开始），`durChapterPos` 是**该章正文内的字符偏移**。
为了偏移一一对应，正文不做任何重排（不压缩空行、不重排缩进），本地位置直接就是手机端的位置。

## 文件与实现

| 文件 | 作用 |
|---|---|
| `Reader/WebBook.h` / `WebBook.cpp` | `WebBook : public Book`（新增书籍类型 `book_web`）：拉章节列表与正文、换章、进度同步 |
| `Reader/WebDlg.h` / `WebDlg.cpp` | 「Web 阅读」对话框（IDD_WEB / IDM_WEB） |
| `<exe目录>\.web\<书名>-<hash>.web` | 每本书一个暂存文件（JSON：服务地址、bookUrl、书名、作者、上次章节与偏移），作为书籍身份进入阅读列表 |
| `<exe目录>\.web\server.txt` | 上次使用的服务地址（UTF-16LE） |

- 网络层用 **WinHTTP**（系统自带 `winhttp.dll`，`WINHTTP_ACCESS_TYPE_NO_PROXY`），不是工程里的 libhttps：
  这个版本的 libhttps 处理不了带端口的 URL（`http://host:80/` 直接 `errno=-1`），而 legado web 服务就在 `host:1122` 上。
- 打开阶段用同步请求（本来就在工作线程里）；换章/回写进度在自己的后台线程里做同步请求，结果用
  `SendMessage(WM_BOOK_EVENT, ...)` 回到界面线程；只有界面线程会改 `m_Text/m_Chapters/m_Index`。
- 全部代码都在 `#ifdef ENABLE_NETWORK` 内：无网络版编译时这两个文件是空的，菜单项也会被
  `RemoveMenus()` 摘掉。

## 已知限制

- 阅读时一次只在内存里放当前章节（与「在线小说」一致），目录树跳章会重新拉取该章正文。
  （手机本地书若在 legado 里章节数很少，例如整本书算 1 章，就会一次把整章正文读进来。）
- 回写进度按 `name + author` 匹配（和手机网页版行为一致）；同名同作者的两本书会互相覆盖。
- 手机上的本地书如果是从 legado 里删掉、或文件被移走，服务端会返回失败，此时打开会提示重试。
