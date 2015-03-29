# Version 2.0.0 #
## English Version ##
It's easy to use this extension to capture visible content of a tab, a region of a web page, or the whole page as a PNG image.

You can also edit your captured image before saving it as a PNG image. Highlighting, redacting and adding text are supported.

What set this extension apart are:
  * Super fast when capturing page image, esp. for large pages
  * Support horizontal scroll for large pages that do not fit in one tab screen horizontally
  * Intelligently detect floating objects on a page and avoid repeating capture of the same floating objects if whole page capture requires scrolling
  * Works on Windows, Linux and Mac

New in Version 2:
  * Fix crash bug when trying to save a large image
  * More integrated and easily configured editing tools, with blur redacting and line drawing
  * Autosave captured image

Due to a bug in Chrome 5, capturing lossless screenshots on Windows will have distorted text. Please use the "Lossy screenshots" option instead. This issue is addressed in Chrome 6.

By installing this extension, you agree to the Terms of Service at https://chrome.google.com/extensions/intl/en/gallery_tos.html

## 中文版 ##
该扩展可以轻松截取当前标签页的可见区域，当前网页的指定区域，或是整张网页的页面。

截图后，可以利用图片编辑工具编辑图片，然后将编辑后的图片保存为PNG格式的图片文件。目前我们提供高亮工具，涂改工具和文字添加工具。

该扩展的主要亮点有：
  * 截取整张超大网页可以迅速完成
  * 可以完整截取带横向滚动条的网页
  * 自动检测网页上的浮动元素，在整张网页截图上只会出现一次
  * 支持Windows，Linux和Mac

第2版新增功能：
  * 修正保存大网页时的页面崩溃问题
  * 整合的编辑工具更易配置，新增模糊和画线功能
  * 新增自动截图保存功能

由于Chrome 5中存在的已知问题，在Windows平台下进行无损截图会导致文字失真，请换用“有损截图”选项。Chrome 6解决了这个问题。

安装该扩展即表示您同意以下服务条款：https://chrome.google.com/extensions/intl/zh-CN/gallery_tos.html

# Version 0.1.0 #
## English Version ##
It's easy to use this extension to capture visible content of a tab, a region of a web page, or the whole page as a PNG image.

You can also edit your captured image before saving it as a PNG image. Highlighting, redacting and adding text are supported.

What set this extension apart are:
  * Super fast when capturing page image, esp. for large pages;
  * Support horizontal scroll for large pages that do not fit in one tab screen horizontally;
  * Intelligently detect floating objects on a page and avoid repeating capture of the same floating objects if whole page capture requires scrolling;
  * Works on Windows, Linux and Mac.

Known Bug: When you save a screenshot of a very large page, the context menu may not show when you right-click on the screenshot, and dragging the screenshot may crash the extension process. If this happens, you will need to restart Chrome to get your extension back. This bug is due to the data URI length limitation in Chrome which is 2M characters long. We'll fix it in our next version coming soon.

Known [https://code.google.com/p/chrome-screen-capture/issues/detail?id=](https://code.google.com/p/chrome-screen-capture/issues/detail?id=): If you are using Chrome stable or beta version in Windows and choose to capture lossless screenshots, your screenshots will have distorted texts due to an extension API bug: crbug.com/44758. If you see distorted texts in your screenshots, please try switching to "Lossy screenshots" in "Screenshot Quality Settings" in the option page.

By installing this extension, you agree to the Terms of Service at https://chrome.google.com/extensions/intl/en/gallery_tos.html

## 中文版 ##
该扩展可以轻松截取当前标签页的可见区域，当前网页的指定区域，或是整张网页的页面。

截图后，可以利用图片编辑工具编辑图片，然后将编辑后的图片保存为PNG格式的图片文件。目前我们提供高亮工具，涂改工具和文字添加工具。

该扩展的主要亮点有：
  * 截取整张超大网页可以迅速完成；
  * 可以完整截取带横向滚动条的网页；
  * 自动检测网页上的浮动元素，在整张网页截图上只会出现一次；
  * 支持Windows，Linux和Mac。

已知问题：由于Chrome限制data URI的长度不能超过2M字节，当你保存大网页的截图时，在截图上点击右键可能无法弹出右键菜单，并且拖动截图会让页面崩溃，扩展消失。重启Chrome可以让扩展恢复正常。我们会尽快推出下一版本解决这个问题。

已知问题2：如果你在使用Windows系统和Chrome稳定版或Beta版并选择截取无损截图，你的截图中的文字会失真，这是由一个API问题导致的（crbug.com/44758）。如果你发现截图中的文字失真，请将选项页面中的“截图质量设置”改为“有损截图”。

安装该扩展即表示您同意以下服务条款：https://chrome.google.com/extensions/intl/zh-CN/gallery_tos.html