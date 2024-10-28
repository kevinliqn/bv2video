# bv2video
## 一个用于转换bilibili客户端视频为常见视频封装的软件
软件的制作依赖了copilot的帮助
## 如何运行这个程序？
方法一：在release里有打包好的包，将库也一起打包了，可以直接用将bilibili客户端下的`download`下的文件全部放入`bilibili_video`下，双击`bv2video.exe`可开始转换，完成后的视频文件在`videotrans`目录下
***
方法二：克隆本项目

      git clone https://github.com/kevinliqn/bv2video.git

bv2video使用visual studio 2022开发，在项目属性中添加链接器——输出——附加依赖项中添加`avformat.lib;avutil.lib;avcodec.lib;cjson.lib`,bv2video_include文件夹里包含了项目所需的头文件，此外，编译后还需`ffmpeg`的库文件将编译后的ffmpeg库文件放入编译好后的bv2video同目录下，
在程序铜目录下创建`bilibili_video`和`videotrans`文件夹

## Libraries

* `libavcodec` provides implementation of a wider range of codecs.
* `libavformat` implements streaming protocols, container formats and basic I/O access.
* `libavutil` includes hashers, decompressors and miscellaneous utility functions.
* `libavfilter` provides means to alter decoded audio and video through a directed graph of connected filters.
* `libavdevice` provides an abstraction to access capture and playback devices.
* `libswresample` implements audio mixing and resampling routines.
* `libswscale` implements color conversion and scaling routines.

## License
bv2video 原始代码在 LGPLv2.1 下发布。上游仓库中的所有代码都保留在其原始许可证下（有关更多详细信息，请参阅上游仓库的许可证信息）。

## 在最后
欢迎大家提出不足，我收到大家的反馈会及时改正的，也欢迎大家将想要的功能提出来，欢迎提交代码
## 踢作者
<kevinliqn@linvk.com>
