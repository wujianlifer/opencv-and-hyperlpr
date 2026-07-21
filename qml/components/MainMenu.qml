import QtQuick
import QtQuick.Controls
import App 1.0

// 菜单栏：openDialog/saveDialog/aboutDialog 由主窗口注入
MenuBar {
    property var openDialog
    property var saveDialog
    property var aboutDialog

    Menu {
        title: "文件"
        Action { text: "打开"; icon.source: "qrc:/icons/folder.png"; onTriggered: openDialog.open() }
        Action { text: "保存"; icon.source: "qrc:/icons/diskette.png"; enabled: processor.hasResult; onTriggered: saveDialog.open() }
        Action { text: "另存为"; enabled: processor.hasResult; onTriggered: saveDialog.open() }
        Action { text: "退出"; icon.source: "qrc:/icons/close.png"; onTriggered: Qt.quit() }
    }
    Menu {
        title: "编辑"
        Action { text: "恢复"; icon.source: "qrc:/icons/restore.png"; onTriggered: processor.reset() }
        Action { text: "直方图"; icon.source: "qrc:/icons/bar-chart.png"; onTriggered: processor.showHistogram() }
        Action { text: "添加噪声"; icon.source: "qrc:/icons/noise.png"; onTriggered: processor.selectOperation(ImageProcessor.NOISE) }
    }
    Menu {
        title: "基本操作"
        Action { text: "旋转"; icon.source: "qrc:/icons/rotate.png"; onTriggered: processor.selectOperation(ImageProcessor.BASED_ROTATE) }
        Action { text: "水平翻转"; icon.source: "qrc:/icons/flip-h.png"; onTriggered: processor.selectOperation(ImageProcessor.BASED_HORIZONTAL_FLIP) }
        Action { text: "垂直翻转"; icon.source: "qrc:/icons/flip-v.png"; onTriggered: processor.selectOperation(ImageProcessor.BASED_VERTICAL_FLIP) }
        Action { text: "缩放"; icon.source: "qrc:/icons/resize.png"; onTriggered: processor.selectOperation(ImageProcessor.BASED_RESIZE) }
        Action { text: "灰度化"; onTriggered: processor.selectOperation(ImageProcessor.BASED_GRAY) }
    }
    Menu {
        title: "图像分割"
        Action { text: "边缘检测"; onTriggered: processor.selectOperation(ImageProcessor.SEGMENTATION_EDGE) }
        Action { text: "直线检测"; onTriggered: processor.selectOperation(ImageProcessor.SEGMENTATION_LINE_DETECTION) }
        Action { text: "阈值处理"; onTriggered: processor.selectOperation(ImageProcessor.SEGMENTATION_THRESHOLD) }
    }
    Menu {
        title: "频率域算法"
        Action { text: "傅里叶变换"; onTriggered: processor.selectOperation(ImageProcessor.FREQUENCY_FOURIER) }
        Action { text: "低通滤波器"; onTriggered: processor.selectOperation(ImageProcessor.FREQUENCY_LOW_PASS_FILTER) }
    }
    Menu {
        title: "空间域算法"
        Action { text: "直方图均衡"; onTriggered: processor.selectOperation(ImageProcessor.DIMENSION_EQUALIZATION_HISTOGRAM) }
        Menu {
            title: "平滑滤波器"
            Action { text: "中值滤波"; onTriggered: processor.selectOperation(ImageProcessor.DIMENSION_MEDIAN_FILTER) }
            Action { text: "高斯滤波"; onTriggered: processor.selectOperation(ImageProcessor.DIMENSION_GAUSSIAN_FILTER) }
        }
        Menu {
            title: "锐化滤波器"
            Action { text: "Laplacian滤波"; onTriggered: processor.selectOperation(ImageProcessor.DIMENSION_LAPLACIAN_FILTER) }
            Action { text: "Sobel滤波"; onTriggered: processor.selectOperation(ImageProcessor.DIMENSION_SOBEL_FILTER) }
        }
    }
    Menu {
        title: "形态学操作"
        Action { text: "膨胀"; onTriggered: processor.selectOperation(ImageProcessor.MORPHOLOGY_DILATION) }
        Action { text: "腐蚀"; onTriggered: processor.selectOperation(ImageProcessor.MORPHOLOGY_EROSION) }
        Action { text: "开运算"; onTriggered: processor.selectOperation(ImageProcessor.MORPHOLOGY_OPENING) }
        Action { text: "闭运算"; onTriggered: processor.selectOperation(ImageProcessor.MORPHOLOGY_CLOSING) }
    }
    Menu {
        title: "艺术效果"
        Action { text: "水彩画"; onTriggered: processor.selectOperation(ImageProcessor.ART_COLOR_PAINTING) }
        Action { text: "怀旧"; onTriggered: processor.selectOperation(ImageProcessor.ART_OLD_PHOTO) }
        Action { text: "素描"; onTriggered: processor.selectOperation(ImageProcessor.ART_SKETCH) }
    }
    Menu {
        title: "车牌识别"
        Action { text: "车牌识别"; onTriggered: processor.selectOperation(ImageProcessor.PLATE) }
    }
    Menu {
        title: "关于"
        Action { text: "关于"; onTriggered: aboutDialog.open() }
    }
}
