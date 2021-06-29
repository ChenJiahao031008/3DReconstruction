## ImageBasedModellingEdu V2.1
ImageBasedModellingEdu V2.1 是用于深蓝学院基于图像的三维模型重建课程配套的代码。该代码来源于著名的开源三维重建开源系统MVE(https://github.com/simonfuhrmann/mve)。
我们其基础之上对代码的架构进行了调整，使其与课程更为相关，有助于阅读和学习。该工程项目采用CMake管理，可与方便的进行跨平台的编译。代码包含特征提取与匹配、对极几何、运动恢复结构、稠密重建、表面重建以及纹理贴图、可视化等模块，将随着课程的深入不断进行更新。

### 更新进度

2021/06/29: Update Task3

## 架构

该工程主要包含`core, math, util, features, sfm, mvs, surface, texturing`等主要模块，其中：
- `core`—提供了工程项目需要的所有的基础数据结构，包括`image, depthmap, mesh, view`,以及数据的输入输出等结构和功能；
- `math`—提供矩阵，向量，四元数等基本的数学运算操作；
- `features`—提供特征提取以及特征匹配功能，其中特征类型包括sift和surf两种；
- `sfm`—提供了与运动恢复结构相关的功能，包括相机姿态的恢复，三维点的三角化和捆绑调整等；
- `mvs`—提供立体匹配功能，实现稠密点云匹配；
- `surface`—实现点云到网格的表面重建；
- `texturing`—实现纹理图像的创建；
- `examples`—提供一些关键模块的示例代码；
- `tmp`—存储临时数据

## 编译（Mac和Linux下没有问题，Window下的编译未经过验证）
1.安装依赖库包含`libpng, libjpeg, libtiff, eigen`

 ### Linux（推荐）
```bash
sudo apt-get install libjpeg-dev
sudo apt-get install libtiff-dev
# 安装Eigen，可以选择低一点的版本，验证版本为3.3.1
git clone https://github.com/eigenteam/eigen-git-mirror.git
mkdir build
cd build
cmake  ..
sudo make install
sudo cp -r /usr/local/include/eigen3 /usr/include 
# opegl安装。
sudo apt-get install build-essential
sudo apt-get install libgl1-mesa-dev
sudo apt-get install libglu1-mesa-dev
sudo apt-get install freeglut3-dev
## 由于glew这部分安装，对于不同系统的安装方法不一样，因此请自行百度
```

 ### Mac
```bash
 brew install libpng 
 brew install libjpeg
 brew install libtiff
```

2.执行

```bash
git clone https://github.com/weisui-ad/ImageBasedModellingEdu.git
cd ImageBasedModellingEdu
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release .. 
make -j8 //-j+数字表示编译所用的核心数，根据自己电脑的能力来设置
```

3. 所有作业放在`examples`文件夹下

   **注意：本项目中有大量相对路径存在，除非特殊说明，否则默认的启动位置为./build/文件夹下，如果遇到文件报错并且对相关位置比较模糊，请修正为绝对路径。**
   
   ```bash
   ## 以作业1-6为例，演示运行代码，执行命令如下
   cd build
   ./examples/task1/task1-6_test_matching  ../examples/data/kxm1.jpg ../examples/data/kxm2.jpg ../examples/data/result/
   ```
   
   

 
