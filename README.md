# 图像处理实验平台 (opencvAndHyperlpr)

基于 Qt Quick（QML）和 OpenCV 的数字图像处理教学软件，支持基础图像处理算法和车牌识别功能（传统算法 + HyperLPR 深度学习）。

> CMake 工程名为 `opencvAndHyperlpr`，运行时程序名为「图像处理实验平台」。

> opencv(安装到C:\Program Files): https://github.com/opencv/opencv/releases/download/4.12.0/opencv-4.12.0-windows.exe

> 依赖 MNN和hyperlpr ： https://gitee.com/wujianlifer/mnn、https://gitee.com/wujianlifer/hyper-lpr

## 1. 项目简介

本项目是一个数字图像处理教学软件，旨在帮助学生学习和理解各种图像处理算法。软件采用 **QML 前端 + C++ 后端** 架构，提供直观的 GUI 界面，支持实时预览处理效果，并集成了传统车牌识别算法和基于 HyperLPR 的深度学习车牌识别。图像处理在后台线程执行，界面不会卡顿。

## 2. 技术框架

| 组件 | 版本 | 用途 |
|------|------|------|
| **Qt** | 6.10.3 | GUI 界面构建（Qt Quick / QML） |
| **OpenCV** | 4.12.0 | 图像处理算法 |
| **MNN** | 3.6.0 | 深度学习推理引擎 |
| **HyperLPR** | 3.x | 车牌识别深度学习模型 |
| **CMake** | 3.24+ | 构建工具 |
| **MSVC** | 2022 | C++ 编译器 |

## 3. 开发环境

- **操作系统**：Windows 10/11 (x64)
- **IDE**：Qt Creator 12.x / CLion
- **编译器**：MSVC 2022 (v194+)
- **构建工具**：CMake 3.24+ / Ninja

## 4. 目录结构

```
opencvAndHyperlpr/
├── sources/                  # 源文件目录
│   ├── core/                 # 入口与后端核心
│   │   ├── main.cpp          # 程序入口，注册 QML 类型与上下文
│   │   ├── image_processor.cpp/.hpp   # 主后端/数据模型，向 QML 暴露操作与参数
│   │   ├── processor_worker.cpp/.hpp  # 后台工作线程，执行耗时的图像处理
│   │   ├── image_provider.cpp/.hpp    # QML 图像提供器（跨线程安全）
│   │   ├── based.cpp/.hpp    # 基础操作（旋转/翻转/缩放/灰度）
│   │   └── util.cpp/.hpp     # 工具函数
│   ├── manipulators/         # 图像处理算法实现
│   │   ├── manipulator.cpp/.hpp
│   │   ├── segmentation_manipulator.cpp/.hpp   # 分割（Canny/直线/阈值）
│   │   ├── frequency_manipulator.cpp/.hpp      # 频率域（傅里叶/低通）
│   │   ├── dimension_manipulator.cpp/.hpp      # 空间域（均衡/各类滤波）
│   │   ├── morphology_manipulator.cpp/.hpp     # 形态学（腐蚀/膨胀/开/闭）
│   │   └── arts_manipulator.cpp/.hpp           # 艺术效果
│   └── recognition/          # 车牌识别
│       ├── hyperlpr_wrapper.cpp/.hpp           # HyperLPR 封装
│       └── licence_recognition_util.cpp/.hpp   # 传统车牌识别算法
├── headers/                  # 头文件（与 sources 对应：core / manipulators / recognition）
├── qml/                      # QML 前端界面
│   ├── main.qml
│   ├── components/           # ImagePanel / MainMenu / ParameterPanel
│   └── dialogs/              # About / Alert / Histogram / ImageViewer / Open / Save 对话框
├── ui/icons/                # 图标资源（resources.qrc）
├── thirdparty/               # 预编译依赖库
│   ├── MNN.dll / MNN.lib
│   ├── hyperlpr3.dll / hyperlpr3.lib
│   ├── opencv_world4120.dll
│   └── opencv_world4120d.dll
├── resource/                 # 资源文件（模型、字体，构建时自动复制）
├── pictures/                 # 字符模板（传统车牌识别）/ 示例图片
├── HyperLPR-master/          # HyperLPR 源码与资源
├── legacy/                   # 已弃用的 Widgets 对话框（不再编译）
├── build/                    # 构建目录
├── CMakeLists.txt            # 构建配置
├── qml.qrc                    # QML 资源
└── README.md                 # 项目说明
```

## 5. 软件架构

- **前端（QML）**：使用 Qt Quick Controls 2（Fusion 风格）构建界面，包含主菜单、图像显示面板（`ImagePanel`）与参数面板（`ParameterPanel`）。
- **后端（C++）**：`ImageProcessor` 作为 `QObject` 数据模型，通过 `setContextProperty("processor", ...)` 暴露给 QML 上下文；操作类型枚举通过 `qmlRegisterUncreatableType<ImageProcessor>(...)` 注册，QML 中以 `ImageProcessor.BASED_ROTATE` 这类类型级枚举访问（注意：实例级枚举访问会解析为 `undefined`，不可用）。
- **后台线程**：耗时的图像处理由 `ProcessorWorker` 在独立线程中执行，通过任务序号丢弃过期结果；处理结果经 `ImageProvider` 跨线程安全地推送到 QML 显示，避免界面卡顿与数据竞争。
- **算法实现**：具体算法按模块归类在 `sources/manipulators/`，车牌识别在 `sources/recognition/`。

## 6. 功能模块

### 6.1 基础操作
- 图像打开/保存
- 水平翻转 / 垂直翻转
- 旋转
- 缩放
- 灰度转换

### 6.2 图像分割
- 边缘检测（Canny）
- 直线检测（Hough）
- 阈值处理

### 6.3 频率域增强
- 傅里叶变换
- 低通滤波

### 6.4 空间域增强
- 直方图均衡化
- 中值滤波
- 高斯滤波
- 拉普拉斯滤波
- 索贝尔滤波

### 6.5 形态学操作
- 腐蚀
- 膨胀
- 开运算
- 闭运算

### 6.6 艺术效果
- 水彩艺术画
- 怀旧照片
- 素描

### 6.7 噪声处理
- 添加噪声（斑点噪声、椒盐噪声、高斯噪声）

### 6.8 车牌识别
- **传统算法模式**：基于颜色分割、轮廓检测、模板匹配
- **HyperLPR 模式**：基于深度学习的端到端车牌识别

## 7. 参数输入说明

选择操作后，参数面板（`ParameterPanel`）会根据当前操作动态显示对应的参数控件：

- **单参数操作**（如膨胀、缩放、阈值处理）使用 `SpinBox` 输入。
- **双参数操作**（如 Canny 边缘检测、直线检测、高斯滤波）的第一个参数用 `SpinBox`，第二个参数用 `TextField`（支持小数）。所有输入框的文本均**水平居中对齐**，风格统一。
- **选项类操作**（添加噪声、车牌识别）使用 `ComboBox` 下拉选择。
- **操作标题会显示参数的允许范围**，便于了解输入边界，例如：
  - `膨胀-请输入膨胀核大小（0-50）`
  - `图像缩放-请输入缩放比例（1-100）`
  - `阈值处理——请输入阈值（0-255）`
  - `低通滤波-请输入低通滤波核大小（0-15）`
  - `图像旋转-请输入角度（0-360）`

各操作的参数范围一览：

| 操作 | 参数 | 允许范围 |
|------|------|----------|
| 旋转 | 角度 | 0–360（度） |
| 缩放 | 缩放比例 | 1–100（%） |
| 阈值处理 | 阈值 | 0–255 |
| 低通滤波 | 滤波半径 | 0–15 |
| 中值滤波 | 半径 | 0–15 |
| 拉普拉斯滤波 | 半径 | 0–15 |
| 索贝尔滤波 | 半径 | 0–15 |
| 膨胀 / 腐蚀 / 开运算 / 闭运算 | 核大小 | 0–50 |
| Canny 边缘检测 / 直线检测 | 低阈值、高阈值 | 0–500 |
| 高斯滤波 | 核大小 / 标准差 | 核 0–15 / 标准差（小数） |
| 添加噪声 | 噪声类型 | 斑点 / 椒盐 / 高斯（选项） |
| 车牌识别 | 识别模式 | HyperLPR 深度学习 / 传统算法（选项） |

## 8. 编译和运行

### 8.1 环境准备

1. 安装 Qt 6.10.3（选择 MSVC2022_64 工具链）
2. 安装 OpenCV 4.12.0（预编译包，vc16）
3. 安装 Visual Studio 2022 Build Tools
4. 安装 CMake 3.24+
5、编译MNN：https://gitee.com/wujianlifer/mnn
6、编译hyperlpr：https://gitee.com/wujianlifer/hyper-lpr
7、把刚才编译过后的 hyperlpr3.dll、hyperlpr3.lib、MNN.dll、MNN.lib、opencv_world4120.dll、opencv_world4120.lib 移动到 `(thirtyparty)` 
### 8.2 配置和编译

**方式一：Qt Creator（推荐）**

1. 打开 Qt Creator
2. 打开项目 → 选择 `CMakeLists.txt`
3. 在"Projects"面板中添加构建配置，选择 MSVC2022_64 工具链
4. 点击"Configure"按钮
5. 点击"Build"按钮

**方式二：命令行**

```bash
# 创建构建目录
mkdir build && cd build

# 配置（指定 Qt 和 OpenCV 路径）
cmake .. ^
    -DCMAKE_PREFIX_PATH="E:/Qt/6.10.3/msvc2022_64" ^
    -DOpenCV_DIR="C:/Program Files/opencv/build/x64/vc16/lib" ^
    -G "Ninja"

# 编译
cmake --build . --config Debug
```

### 8.3 运行

编译成功后，运行生成的 `opencvAndHyperlpr.exe`，或在 Qt Creator 中点击"Run"按钮。构建过程会通过 `windeployqt` 自动复制 Qt 运行依赖，并复制 `MNN.dll`、`hyperlpr3.dll`、`opencv_world4120*.dll` 及模型/字体资源到构建目录。

## 9. 车牌识别使用说明

> ⚠️ **传统算法识别率太低，不推荐使用，默认使用 HyperLPR 识别**

### 9.1 ~~传统算法模式~~

1. ~~打开车牌识别对话框~~
2. ~~选择图片（支持 JPG、PNG 等格式）~~
3. ~~确保选择"传统算法"模式~~
4. ~~点击"开始识别"~~
5. ~~可以通过"上一步"/"下一步"查看处理流程~~

### 9.2 HyperLPR 模式

1. 打开车牌识别对话框
2. 选择图片（建议尺寸 ≥ 640x480）
3. 选择"HyperLPR深度学习"模式
4. 点击"开始识别"
5. 识别结果会显示在图片上（绿色框 + 红色文字）

### 9.3 识别结果

- 车牌号码和置信度会显示在文本框中
- 图片上会标注车牌位置（绿色矩形框）和识别结果

## 10. 注意事项

### 10.1 Qt / OpenCV 路径配置

`CMakeLists.txt` 中默认路径：
```cmake
set(CMAKE_PREFIX_PATH "E:/Qt/6.10.3/msvc2022_64")   # 未定义时生效
set(OpenCV_DIR "C:/Program Files/opencv/build/x64/vc16/lib")
```

如果路径不同，请通过以下方式之一修改：

1. **修改 CMakeLists.txt**：直接修改上述变量的值
2. **命令行参数**：`cmake .. -DCMAKE_PREFIX_PATH="你的路径" -DOpenCV_DIR="你的路径"`
3. **环境变量**：设置 `CMAKE_PREFIX_PATH` / `OpenCV_DIR` 环境变量

### 10.2 Debug/Release 构建类型

- **Debug 模式**：使用 `thirdparty/` 中的 Debug 版本库（推荐开发调试）
- **Release 模式**：需要重新编译 MNN 和 HyperLPR 的 Release 版本库

### 10.3 依赖库版本匹配

所有依赖库必须使用相同的编译环境：
- 编译器：MSVC 2022
- 架构：x64
- 构建类型：全部 Debug 或全部 Release

### 10.4 模型与字体文件

HyperLPR 模型文件位于 `HyperLPR-master/resource/models/r2_mobile/`，字体位于 `HyperLPR-master/resource/font/platech.ttf`，构建时会自动复制到构建目录的 `resource/` 下。

### 10.5 运行时依赖

运行程序需要以下 DLL 文件（编译时会自动复制）：
- Qt 相关：Qt6Core.dll, Qt6Gui.dll, Qt6Qml.dll, Qt6Quick.dll, Qt6QuickControls2.dll 等（由 windeployqt 处理）
- OpenCV：opencv_world4120.dll (Release) / opencv_world4120d.dll (Debug)
- MNN：MNN.dll
- HyperLPR：hyperlpr3.dll

## 11. 移植到其他电脑

### 11.1 需要复制的文件

```
opencvAndHyperlpr/
├── sources/
├── headers/
├── qml/
├── ui/
├── thirdparty/
├── resource/
├── pictures/
├── HyperLPR-master/
├── CMakeLists.txt
└── qml.qrc
```

### 11.2 目标电脑需要安装的软件

- Qt 6.10.3（MSVC2022_64）
- OpenCV 4.12.0（vc16 预编译包）
- Visual Studio 2022 Build Tools
- CMake 3.24+

## 12. 常见问题

### Q1: 编译时找不到 OpenCV
**A**: 确保 `OpenCV_DIR` 指向正确的路径，例如 `C:/Program Files/opencv/build/x64/vc16/lib`

### Q2: 运行时提示缺少 DLL
**A**: 检查 `thirdparty/` 目录是否包含所有必要的 DLL，或由 `windeployqt` 自动复制的 Qt 依赖是否完整

### Q3: HyperLPR 识别不到车牌
**A**:
1. 确保图片中车牌清晰可见
2. 尝试使用更大尺寸的图片（≥ 640x480）
3. 检查模型文件是否正确复制到构建目录

### Q4: 程序启动时崩溃（0xC000007B）
**A**: 检查所有依赖库的构建类型是否一致（全部 Debug 或全部 Release）

### Q5: QML 中操作枚举选不中
**A**: 枚举需通过类型级访问，例如 `ImageProcessor.BASED_ROTATE`，不要使用实例级 `processor.BASED_ROTATE`（会解析为 `undefined` → `0` → `EMPTY`）。

## 13. 许可证

本项目使用 MIT 许可证，详见 `LICENSE` 文件。

HyperLPR 项目使用 Apache 2.0 许可证，详见 `HyperLPR-master/LICENSE` 文件。

## 14. 参考与致谢

本项目在以下开源项目的基础上进行修改与扩展，在此表示感谢：

- **QtImageProcess**（前端界面与图像处理流程的参考实现）：<https://github.com/Ayuan2002/QtImageProcess>
- **MNN**（轻量级深度学习推理引擎）：<https://github.com/alibaba/MNN>
- **HyperLPR**（高性能开源中文车牌识别框架）：<https://github.com/szad670401/HyperLPR>

