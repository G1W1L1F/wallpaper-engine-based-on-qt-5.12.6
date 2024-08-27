# wallpaper-engine-based-on-qt-5.12.6

## 基本描述
一款基于qt creator5.12.6版本开发的windows桌面壁纸更换软件
可以加载图片和视频作为桌面壁纸，并在软件内生成对应的缩略图或者gif，双击图标即可更换壁纸

## 依赖项
需要安装qt对应版本的的webengine（可在国内镜像库中下载）并安装ffmpeg和Lavfilters软件以正确处理GIF生成和视频播放功能

## 来源
本项目更换壁纸的基本功能实现来自于开源项目https://github.com/helloheidi/my_wallpaper

## 功能实现
处理了其中的图片壁纸契合度调整不佳，视频壁纸读取失败等bug，完善了原视频和原图片的查看功能，添加取消壁纸功能，并在关闭软件后可自动恢复系统设置壁纸。添加了壁纸收藏，改变填充色，设置开机自启动，更改软件背景，调整视频背景音量等诸多自定义功能。内置一个网址游览器，默认可跳转到壁纸网站https://wallhaven.cc/
网站（需要梯子，当然也可访问其他网站），可直接在网页内游览图片并保存到软件内的图库文件夹，方便添加到软件内的可选壁纸栏中。最好完善并优化用户交互界面

## 可执行程序
打包好的exe文件可在链接处获得：https://pan.baidu.com/s/1LPgfmNmDSK9upwIrAcDBkQ?pwd=GWLF
提取码：GWLF
如果只是使用该exe文件，则需安装ffmpeg和Lavfilters软件以正确处理GIF生成和视频播放功能的正确实现


