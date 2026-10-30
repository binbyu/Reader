# Reader

<p align="center">
  <img src="doc/images/reader.ico" alt="Reader Logo" width="80" height="80" onerror="this.style.display='none'"/>
</p>

<p align="center">
  <strong>基于 Win32 API 开发的原生 Windows 电子书阅读器</strong><br>
  轻量简洁 · 低资源占用 · 支持本地与在线阅读
</p>

<p align="center">
  <a href="https://github.com/binbyu/Reader/releases"><img src="https://img.shields.io/github/v/release/binbyu/Reader?color=blue&label=Release" alt="Release"></a>
  <a href="CHANGELOG.md"><img src="https://img.shields.io/badge/Changelog-更新日志-blue.svg" alt="Changelog"></a>
  <a href="https://github.com/binbyu/Reader/stargazers"><img src="https://img.shields.io/github/stars/binbyu/Reader?color=brightgreen" alt="Stars"></a>
  <a href="https://github.com/binbyu/Reader/blob/master/LICENSE"><img src="https://img.shields.io/badge/License-Open%20Source-orange.svg" alt="License"></a>
  <img src="https://img.shields.io/badge/Platform-Windows-lightgrey.svg" alt="Platform">
</p>

<p align="center">
  <a href="#-核心功能">核心功能</a> •
  <a href="#-下载与版本说明">下载与版本</a> •
  <a href="#-使用指南与相关文档">文档指南</a> •
  <a href="#-常用快捷键">快捷键</a> •
  <a href="CHANGELOG.md">更新日志</a> •
  <a href="#-免责声明">免责声明</a>
</p>

---

## 📖 项目简介

**Reader** 是一款开源、免费的 Windows 平台轻量级小说阅读器。程序基于原生 Win32 API 与 C/C++ 开发，不依赖任何第三方运行时框架，体积小巧，启动迅速，内存资源占用低。

软件支持本地主流电子书格式（TXT、EPUB、MOBI），支持基于规则的在线书源解析与阅读，并支持接入局域网内的开源阅读 (Legado) Web 服务。同时提供了细致的排版设置、无边框显示、窗口透明度调节及便捷的快速隐藏功能，适合多场景下的阅读使用。

---

## ✨ 核心功能

- **轻量原生**：基于 C/C++ 与 Win32 API 开发，无臃肿依赖，系统资源开销极小。
- **本地阅读格式支持**：
  - 支持 `TXT` 文件，内置文本编码自动识别机制，避免繁体与非中文系统的乱码问题。
  - 支持 `EPUB`、`MOBI` 格式电子书解析与阅读。
- **在线书源阅读**：
  - 支持基于 XPath 规则的自定义书源配置，可抓取并阅读网络连载小说。
  - 支持简易模式：可直接输入小说目录页 URL 自动提取章节，无需事先编写书源。
  - 支持正文文本过滤（关键字过滤、正则表达式匹配），去除无用文本与广告信息。
  - 支持手动刷新连载目录（`F5`）与章节内容（`Ctrl + F5`）。
- **局域网 Web 阅读**：
  - 支持接入局域网内「开源阅读 (Legado)」的 Web 服务，便于在 PC 端无缝阅读移动端书库。
- **窗口与显示调节**：
  - 支持无边框极简模式（`F12`）、全屏模式（`F11`）。
  - 支持窗口透明度与字体透明度调节（无边框模式下支持背景全透明、文字不透明显示）。
  - 支持快速隐藏窗口（`Alt + H`，或鼠标左右键同时按下）、鼠标移出窗口自动隐藏。
  - 支持自动翻页、左右侧区域点击翻页及滚轮平滑滚动。
- **排版与渲染**：
  - 绘图引擎支持 Direct2D 硬件加速，旧版操作系统自动降级兼容 GDI+。
  - 丰富的排版参数调节：字体、字号、颜色、字距、行距、段距、内边距、压缩空行、首行缩进及英文自动换行（Word wrap）。
  - 支持背景纯色、取色器与背景图片平铺。
  - 支持自定义正则表达式解析小说章节目录。
- **转换工具**：
  - 提供辅助转换工具 [`tool/olweb2txt.exe`](tool/olweb2txt.exe)，可将程序缓存的 `.ol` 和 `.web` 文件转换为标准 `.txt` 纯文本文件。

---

## 🚀 下载与版本说明

最新程序包可在 [GitHub Releases](https://github.com/binbyu/Reader/releases) 页面获取。各构建版本说明如下：

| 版本分类 | 运行环境与说明 |
| :--- | :--- |
| **正式版 x64** | 适用于 64 位 Windows 系统（不支持 Windows XP 及以下系统） |
| **正式版 x86/win32** | 适用于 32 位及 64 位 Windows 系统，**兼容 Windows XP 及以上系统** |
| **无网络版 (Networkless)** | 移除了网络请求与爬虫相关模块，适用于纯本地离线阅读环境 |
| **调试版 (Debug)** | 输出详细的运行日志与诊断信息，便于排查书源配置规则 |

> 详细的各版本演进历程与更新明细，请直接查阅：👉 **[CHANGELOG.md (版本更新日志)](CHANGELOG.md)**。

---

## 📚 使用指南与相关文档

- **[版本更新日志 (CHANGELOG.md)](CHANGELOG.md)**：查看最新功能、历史版本迭代及 Bug 修复记录。
- **[书源配置说明 (doc/bs.md)](doc/bs.md)**：包含详细的书源规则说明、XPath 提取方法及参数配置教学。
- **[书源参考模板 (misc/bs.json)](misc/bs.json)**：预置书源示例文件，可在软件“书源配置”页面通过“导入”功能加载。
- **[缓存转换工具 (tool/olweb2txt.exe)](tool/olweb2txt.exe)**：可将缓存的 `.ol` / `.web` 文件转换为本地 `.txt` 格式。
- **[完整使用说明 (misc/readme.txt)](misc/readme.txt)**：包含按键配置、高级功能、目录解析正则等详细参考手册。

---

## ⌨️ 常用快捷键

> 所有快捷键与热键均可在菜单 **【设置】 > 【按键设置】** 中自行修改。

| 类别 | 快捷键 | 功能说明 |
| :--- | :--- | :--- |
| **窗口控制** | `Alt + H` | 快速隐藏 / 显示窗口（亦可鼠标左右键同时按下） |
| | `F12` | 隐藏 / 显示窗口边框 |
| | `Ctrl + 鼠标滚轮` | 调节窗口透明度（无边框下支持背景透明、文字不透明） |
| | `Alt + 鼠标滚轮` | 调节字体透明度 |
| | `Ctrl + Alt + Shift + P` | 开启 / 关闭“鼠标移出窗口后自动隐藏” |
| **界面视图** | `F11` / `Esc` | 全屏切换 / 退出全屏 |
| | `Ctrl + =` / `Ctrl + -` | 字体放大 / 缩小 |
| | `Ctrl + T` | 窗口置顶 / 取消置顶 |
| **翻页与导航** | `空格键` | 开始 / 停止自动翻页 |
| | `←` / `→` | 上一页 / 下一页 |
| | `Ctrl + ←` / `Ctrl + →` | 跳转至上一章 / 下一章 |
| | `↑` / `↓` | 向上 / 向下滚动指定行数 |
| **阅读操作** | `Ctrl + O` | 打开文件（亦支持文件直接拖拽入窗口） |
| | `Ctrl + F` | 全文关键字查找与定位 |
| | `Ctrl + G` | 按百分比快速跳转进度 |
| | `Ctrl + M` | 添加书签 |
| | `Ctrl + E` | 进入文本编辑与复制模式 |
| **在线小说** | `F5` | 刷新当前在线小说的章节列表 |
| | `Ctrl + F5` | 强制刷新并重新获取当前章节内容 |

---

## ❓ 常见问题 (FAQ)

<details>
<summary><b>1. 按 F12 无法隐藏边框？</b></summary>

部分笔记本电脑默认开启了 Fn 功能键锁定，请尝试按 `Fn + F12`；或进入菜单 **【设置】>【按键设置】** 将该功能重新绑定到其他按键。
</details>

<details>
<summary><b>2. 打开小说后出现乱码？</b></summary>

软件内置编码自动识别逻辑。若遇到极少数编码异常情况，请检查菜单 **【设置】>【字体设置】**，确认未选用以 `@` 字符开头的竖排字体。
</details>

<details>
<summary><b>3. 英文排版时单词被断行截断？</b></summary>

请在菜单 **【设置】>【显示设置】** 中勾选 **【Word wrap】** 选项，即可按照完整英文单词进行换行。
</details>

<details>
<summary><b>4. 关于杀毒软件报毒提示？</b></summary>

本项目源码完全开放，无任何恶意行为。由于个人开源项目未购买商业代码签名证书，部分杀毒软件可能产生误报，属于正常现象，请加入信任白名单即可。
</details>

<details>
<summary><b>5. 软件异常或需要恢复初始状态？</b></summary>

删除程序同级目录下的配置文件 `.settings.json`（早期版本为隐藏文件 `.cache.dat`），重启程序即可恢复至默认设置。
</details>

---

## 📜 软件策略与免责声明

1. **项目性质**：Reader 为个人开源、免费软件，主要用于学习交流与电子书阅读，版权归作者所有。
2. **使用限制**：本软件严禁用于非法用途，严禁用于任何商业目的。
3. **内容免责**：软件本身不提供、不存储任何网络文学版权内容。在线书源及 Web 服务所获取的文本均来自第三方公开网页或用户自建服务，第三方内容的合法性与准确性由对应来源负责。
4. **版权保护**：所有图书作品版权归原作者及权利人所有，倡导大家前往正规渠道阅读与支持正版文学作品。

---

## 🤝 鸣谢与联系

感谢所有向本项目提出反馈建议和提交代码的朋友。

- **更新日志**：[CHANGELOG.md](CHANGELOG.md)
- **作者邮箱**：binbyu@qq.com
- **项目主页**：[https://github.com/binbyu/Reader](https://github.com/binbyu/Reader)
