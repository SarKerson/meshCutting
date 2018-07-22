# MESHCUTTING
------

该软件主要用于，对导入的三维网格模型进行简单多边形切割操作；并可以与导入的三维网格进行合并生成新的三维网格模型的操作，总结起来有以下功能：

> * 三维网格自定义多边形切割
> * 三维网格位移、缩放、旋转
> * 三维网格的合并
> * 三维网格四面体化
> * 导入假体自动定位粘合
> * 保存已操作的三维网格

## 运行说明
```
cd <path here>
cd build
cmake ..
make
../bin/meshcutting 执行程序
```
1. qt-gui代码在./core文件里
    1. 主程序在./core/gui下
    2. 四面体网格化的tetgen库在./core/tetgen下
    3. 另外一个回调在./core/cellPicker中
    4. ./core/strutil是文件输入输出辅助
2. 生成的可执行文件在./bin中
3. console版本（非qt版本）在./console目录下
4. ./data中为测试.stl文件

**程序详细说明见pdf及mp4文件**

## 目前瓶颈地方
1. ./script目录下拟放abaqus脚本，在cpp代码中调用并通信。然而由于abaqus无法直接导入stl，因此，此步骤卡住没继续完成。
2. 切割后的三维物体没有保证delaunay。因此，./core/gui/mainwindow.cpp中的afterProc()函数，目前没完成切割后的三维物体的四面体网格化，只完成对导入的规则化三维网格进行四面体网格化并显示。**详细参见注释**