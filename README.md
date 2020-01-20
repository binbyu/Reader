# Reader
A win32 txt file reader

****
最新release版本：`v1.5.3.0`<br/>
(百度网盘)<br/>
链接: [https://pan.baidu.com/s/1astdGSbrGfr3a8vQfdP3Lg](https://pan.baidu.com/s/1astdGSbrGfr3a8vQfdP3Lg)<br/>
提取码: `zb7d`
****
<br/>
<br/>
<br/>
<br/>
# v1.5.1.0 2020/1/16 bug修复<br/>
1. 修复v1.5.0.0版本引入的，Ctrl+F搜索框无焦点bug（搜索框无法使用和关闭）<br/>
2. 修复小说目录部分章节丢失bug<br/>
<br/>
<br/>
<br/>
# v1.5.0.0 2020/1/9 功能更新<br/>
1. 支持epub电子书<br/>
  1.1 软件里面只是对epub进行解码。<br/>
  1.2 解码后的文本，按照txt的方式显示。额外多了一个封面图片渲染。<br/>
  1.3 软件内不支持html渲染，如果要增加，需要引入webkit，软件会变得很大很臃肿。<br/>
  1.4 此次更新使用了开源库：zlib和libxml2<br/>
2. 修改目录为 TreeView<br/>
  2.1 支持 鼠标滚轮操作<br/>
  2.2 支持 打开时定位到当前章节<br/>
<br/>
版本：`v1.5.0.0`<br/>
链接: https://pan.baidu.com/s/1vDQCNNK6Z8F4_D-ZzWgTGg<br/>
提取码: hf9b<br/>
<br/>
<br/>
<br/>
# v1.4.0.0 2020/1/2 功能更新<br/>
1. 增加全屏显示功能<br/>
  1.1 F11：全屏/退出全屏<br/>
  1.2 Esc: 退出全屏<br/>
2. 增加背景图片设置功能<br/>
  2.1 setting > image 里面可以设置<br/>
3. 增加窗口透明度调节功能<br/>
  3.1 调节方式：Ctrl + “鼠标滚轮”<br/>
4. 增加自动翻页功能<br/>
  4.1 快捷键：“空格键” 开始/停止自动翻页<br/>
  4.2 setting > setting > config > 自动翻页时间间隔，可以配置，默认为3000ms<br/>
<br/>
版本：`v1.4.0.0`<br/>
链接: https://pan.baidu.com/s/1MV8Di5F3Dd1is6vMQaVOZg<br/>
提取码: r994<br/>
<br/>
<br/>
<br/>
# v1.3.1.0 2019/12/23 功能更新<br/>
适配网友windows pad，无法使用鼠标右键的场景，新增鼠标左键点击左右侧翻页<br/>
1.  翻页模式一：(setting > config > 鼠标左右键点击翻页)<br/>
    1.1.  鼠标左键单击：下一页<br/>
    1.2.  鼠标右键单击：上一页<br/>
    1.3.  详见readme.txt使用说明<br/>
2.  翻页模式二：(setting > config > 鼠标左键点击左右侧翻页)<br/>
    2.1.  鼠标左键点击界面右侧：下一页<br/>
    2.2.  鼠标左键点击界面左侧：上一页<br/>
    2.3.  详见readme.txt使用说明<br/>
3. release版本使用MT模式编译。避免出现vs运行时库依赖问题<br/>
<br/>
版本：`v1.3.1.0`<br/>
链接: https://pan.baidu.com/s/1IuIMWKpgyzgUvUzYej_gWw<br/>
提取码: rman<br/>
<br/>
<br/>
<br/>
# v1.3.0.0 2019/12/13 功能更新<br/>
1. 新增逐行翻页<br/>
  1.1 为了支持逐行翻页，重新编写了翻页算法<br/>
  1.2 按键 ↓：下N行 (N: setting > config > 文本滚动速度，默认为1)<br/>
  1.3 按键 ↑：上N行<br/>
  1.4 鼠标向下滚动：下N行<br/>
  1.5 鼠标向上滚动：上N行<br/>
  1.6 取消原鼠标滚动直接翻页<br/>
2. 新增进度百分比跳转功能<br/>
  2.1 快捷键：Ctrl + G<br/>
3. 新增readme.txt，使用说明文档<br/>
  3.1 readme.txt会放在release包内，请参考<br/>
<br/>
版本：`v1.3.0.0`<br/>
链接: https://pan.baidu.com/s/1-SYmA5BmbpU3Kc249fjyow<br/>
提取码: sw6p<br/>
<br/>
<br/>
<br/>
# v1.2.0.0 2019/11/05 功能更新<br/>
1. 新增热键支持<br/>
  1.1 Alt + T ：窗口置顶/取消置顶<br/>
  1.2 Alt + H ：窗口隐藏/显示<br/>
  1.3 支持热键修改：setting > config<br/>
  1.4 热键支持两键组合 和 三键组合<br/>
  1.5 如果发现默认按键(1.1)无效，说明该热键已经被占用，请参照1.3修改热键<br/>
2. 新增章节快速跳转<br/>
  2.1 ctrl + → ：跳转到下一章节<br/>
  2.1 ctrl + ← ：跳转到上一章节<br/>
3. 支持文件拖拽打开<br/>
  3.1 仅支持单个txt文件操作<br/>
4. 修改再次打开程序会出现两个view的bug<br/>
<br/>
版本：`v1.2.0.0`<br/>
链接: https://pan.baidu.com/s/1TZbU0F5qLe53ggoru5LKfQ<br/>
提取码: qj6i<br/>
<br/>
<br/>
<br/>
# v1.1.0.0 2019/09/17 功能更新<br/>
1. 新增设置项：setting > config<br/>
  1.1 行距间隙：显示行高 = 字体默认高度 + ”行距间隙“<br/>
  1.2 内部边框：文本与边框之间的距离<br/>
  1.3 翻页方式：<br/>
      1.3.1 鼠标点击：鼠标左键单击：下一页，  鼠标右键单击：上一页<br/>
      1.3.2 鼠标滚动：向上滚动：上一页， 向下滚动：下一页<br/>
2. 取消原鼠标左键双击打开文件功能<br/>
3. 取消原鼠标右键双击隐藏/显示，改为快捷键F12<br/>
4. 取消原鼠标按住拖动窗口功能，改为在“隐藏边框”模式时，鼠标左键按住窗口上部区域进行拖动。<br/>
<br/>
<br/>
# v1.0.0.3 2019/09/17 bug修复<br/>
1. 修改行距bug<br/>
<br/>
<br/>
# v1.0.0.2 2019/01/02 更新<br/>
1. 增加鼠标翻页支持：<br/>
     使用方法：鼠标滚动事件（向上滚动：上一页， 向下滚动：下一页） 
<br/>
<br/> 
# v1.0.0.0 首次提交<br/>
软件介绍： 这是一款使用VS2010开发的一款win32 txt小说阅读器。<br/>
主要功能和特点如下：<br/>
1. 支持BOM-UTF8/UTF8/UTF16/BOM-UNICODE LE or BE/UNICODE/GB2312/ANSI等txt文本格式<br/>
2. 支持字体设置<br/>
3. 支持背景颜色设置<br/>
4. 支持窗口大小任意调整<br/>
5. 支持关键字查找，并跳转进度（Ctrl+F）<br/>
6. 自动解析生成txt小说章节目录，可跳转到指定章节<br/>
7. 自动保存每本小说阅读进度，多本小说切换互不影响<br/>
8. 支持右键双击隐藏窗体边框，再次右键双击恢复<br/>
<br/>
