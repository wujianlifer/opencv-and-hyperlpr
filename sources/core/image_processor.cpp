//
// ImageProcessor 实现
//
#include "image_processor.hpp"
#include "processor_worker.hpp"
#include <QImageReader>
#include <QFileInfo>
#include <QDateTime>
#include <QCoreApplication>
#include <QPainter>
#include <QSettings>
#include <QDir>
#include <windows.h>

ImageProcessor::ImageProcessor(QObject* parent) : QObject(parent) {
    m_windowTitle = "图像处理实验平台";

    // 记忆上次使用的目录（跨运行持久化）
    QSettings settings;
    m_currentFolder = settings.value("lastDir", QUrl::fromLocalFile(QDir::homePath())).toUrl();

    // 注册跨线程传递的请求类型
    qRegisterMetaType<ImageProcessor::ProcessRequest>();

    // 创建后台工作线程
    m_worker = new ProcessorWorker;
    m_worker->moveToThread(&m_workerThread);
    // 提交请求 -> 后台线程处理（跨线程自动排队）
    connect(this, &ImageProcessor::processRequested,
            m_worker, &ProcessorWorker::process);
    // 后台处理完成 -> 回到主线程更新 UI
    connect(m_worker, &ProcessorWorker::finished,
            this, &ImageProcessor::onProcessFinished);
    m_workerThread.start();
}

ImageProcessor::~ImageProcessor() {
    // 正确退出并回收后台线程；线程停止后再删除 worker 是安全的
    m_workerThread.quit();
    m_workerThread.wait();
    delete m_worker;
    m_worker = nullptr;
}

void ImageProcessor::setProcessing(bool v) {
    if (m_processing != v) {
        m_processing = v;
        emit processingChanged();
    }
}

// ----------------------- 属性写入辅助 -----------------------
void ImageProcessor::setSourceImage(const QImage& img) {
    m_sourceImage = img;
    if (m_provider) m_provider->setSource(img);
    ++m_sourceVersion;
    emit sourceImageChanged();
}
void ImageProcessor::setResultImage(const QImage& img) {
    m_resultImage = img;
    if (m_provider) m_provider->setResult(img);
    ++m_resultVersion;
    emit resultImageChanged();
}
void ImageProcessor::setHistogramImage(const QImage& img) {
    m_histogramImage = img;
    if (m_provider) m_provider->setHistogram(img);
    ++m_histogramVersion;
    emit histogramImageChanged();
}
void ImageProcessor::setWindowTitle(const QString& title) {
    if (m_windowTitle != title) {
        m_windowTitle = title;
        emit windowTitleChanged();
    }
}
void ImageProcessor::setPlateText(const QString& text) {
    if (m_plateText != text) {
        m_plateText = text;
        emit plateTextChanged();
    }
}
void ImageProcessor::clearResult() {
    setResultImage(QImage());
    setPlateText(QString());
}
void ImageProcessor::setStatusMessage(const QString& msg) {
    if (m_statusMessage != msg) {
        m_statusMessage = msg;
        emit statusMessageChanged();
    }
}
void ImageProcessor::showWarning(const QString& msg) {
    setStatusMessage(msg);
    emit message(msg, Warning);   // 规范化消息信号，供 QML 决定弹窗
    PlaySound(TEXT("SystemDefault"), nullptr, SND_ALIAS | SND_ASYNC);
}

void ImageProcessor::setCurrentFolder(const QUrl& folder) {
    if (m_currentFolder != folder) {
        m_currentFolder = folder;
        QSettings settings;
        settings.setValue("lastDir", folder);
        emit currentFolderChanged();
    }
}

void ImageProcessor::setParam1Value(int v) {
    if (m_param1Value != v) {
        m_param1Value = v;
        emit param1ValueChanged();
    }
}
void ImageProcessor::setParam2Value(double v) {
    if (m_param2Value != v) {
        m_param2Value = v;
        emit param2ValueChanged();
    }
}
void ImageProcessor::setOptionIndex(int i) {
    if (m_optionIndex != i) {
        m_optionIndex = i;
        emit optionIndexChanged();
    }
}

// ----------------------- 供 QML 调用的槽 -----------------------
bool ImageProcessor::openImage(const QUrl& urlOrPath) {
    QString filePath = urlOrPath.toLocalFile();
    if (filePath.isEmpty()) {
        showWarning("错误: 未选择有效的图像文件");
        return false;
    }
    if (!QFile::exists(filePath)) {
        showWarning("错误: 文件不存在：" + filePath);
        return false;
    }
    // 统一入口：优先用 QImageReader 解码（支持更多格式、保留色彩），
    // 失败再回退到 OpenCV 路径
    QImage img;
    QImageReader reader(filePath);
    reader.setDecideFormatFromContent(true);
    if (!reader.read(&img) || img.isNull()) {
        QPixmap pix = ImageUtil::getPixmapFromFile(filePath);
        if (pix.isNull()) {
            showWarning("错误: 无法加载图像文件，请检查文件格式：" + filePath);
            return false;
        }
        img = pix.toImage();
    }
    m_sourceFileName = filePath;       // 仅保留用于窗口标题等
    setSourceImage(img);
    setResultImage(QImage());          // 清空结果
    setHistogramImage(QImage());       // 清空直方图
    m_type = EMPTY;
    configureParameters();
    setWindowTitle(QFileInfo(filePath).fileName());
    setCurrentFolder(QUrl::fromLocalFile(QFileInfo(filePath).absolutePath()));
    setStatusMessage(QString());
    return true;
}

void ImageProcessor::selectOperation(int type) {
    m_type = static_cast<Operation>(type);
    clearResult();               // 切换操作前清空上一次的处理结果
    ++m_jobId;                   // 作废可能仍在后台运行的旧任务
    setProcessing(false);        // 重新允许提交
    configureParameters();
    QString opName = m_operationTitle;
    // 去除标题中的参数提示，仅保留操作名用于窗口标题
    opName.replace(QRegularExpression("[-—].*$"), QString());
    setWindowTitle("图像处理实验平台-" + opName + m_sourceFileName);
}

void ImageProcessor::apply() {
    if (m_sourceFileName.isEmpty()) {
        showWarning("请先打开一个图像文件");
        return;
    }
    if (m_type == EMPTY) {
        showWarning("请选择您要进行的操作");
        return;
    }

    // ---- 参数校验（与原 Widgets 版本保持一致，校验在主线程即时反馈）----
    if (m_type == SEGMENTATION_THRESHOLD) {
        if (m_param1Value == 0 || m_param1Value == 255) {
            showWarning("错误: 无法进行阈值处理，阈值不能为0或者255");
            return;
        }
    }
    if (m_type == FREQUENCY_LOW_PASS_FILTER) {
        if (m_param1Value % 2 == 0) { showWarning("错误: 无法进行低通滤波，滤波半径大小必须为奇数"); return; }
        if (m_param1Value >= 15)    { showWarning("错误: 无法进行低通滤波，您输入的滤波半径过大"); return; }
        if (m_param1Value == 0)     { showWarning("您还未输入任何滤波半径，请输入滤波半径"); return; }
    }
    if (m_type == DIMENSION_MEDIAN_FILTER || m_type == DIMENSION_GAUSSIAN_FILTER ||
        m_type == DIMENSION_LAPLACIAN_FILTER || m_type == DIMENSION_SOBEL_FILTER) {
        if (m_param1Value % 2 == 0) { showWarning("错误: 无法进行滤波，滤波半径大小必须为奇数"); return; }
        if (m_param1Value >= 15)    { showWarning("错误: 无法进行滤波，您输入的滤波半径过大"); return; }
        if (m_param1Value == 0)     { showWarning("您还未输入任何滤波半径，请输入滤波半径"); return; }
    }
    if (m_type == MORPHOLOGY_DILATION || m_type == MORPHOLOGY_EROSION ||
        m_type == MORPHOLOGY_OPENING || m_type == MORPHOLOGY_CLOSING) {
        if (m_param1Value == 0) { showWarning("您还没有输入核大小"); return; }
    }
    if (m_type == BASED_RESIZE) {
        if (m_param1Value <= 0 || m_param1Value > 100) {
            showWarning("错误: 无法进行缩放，请输入 1~100 的缩放比例(%)");
            return;
        }
    }
    if (m_type == SEGMENTATION_EDGE || m_type == SEGMENTATION_LINE_DETECTION) {
        if (m_param1Value >= m_param2Value) {
            showWarning("错误: 低阈值必须小于高阈值，请重新输入");
            return;
        }
    }

    // 防止并发提交：已有任务在后台处理时直接返回
    if (m_processing)
        return;

    // 组装请求并提交到后台工作线程
    ImageProcessor::ProcessRequest req;
    req.type = m_type;
    req.param1 = m_param1Value;
    req.param2 = m_param2Value;
    req.optionIndex = m_optionIndex;
    req.sourceImage = m_sourceImage;   // 直接提交内存中已加载的源图
    req.requestId = ++m_jobId;

    setProcessing(true);
    setStatusMessage("处理中…");
    emit processRequested(req);
}

void ImageProcessor::onProcessFinished(int requestId, const QImage& result,
                                       const QString& plateText,
                                       const QString& status, bool success) {
    // 丢弃过期任务的结果（用户已切换到其他操作并提交了新任务）
    if (requestId != m_jobId) {
        setProcessing(false);
        return;
    }
    setProcessing(false);
    if (success) {
        setResultImage(result);
        setPlateText(plateText);
        setStatusMessage(status);
    } else {
        if (!plateText.isEmpty())
            setPlateText(plateText);
        if (!status.isEmpty())
            showWarning(status);
        else
            setStatusMessage(QString());
    }
}

bool ImageProcessor::saveResult(const QUrl& path) {
    QString filePath = path.toLocalFile();
    if (filePath.isEmpty())
        return false;
    if (m_resultImage.isNull()) {
        showWarning("错误: 没有可保存的处理结果！");
        return false;
    }
    // 保存格式兜底：无扩展名或扩展名无法识别时，默认保存为 PNG（jpg 需去掉 alpha）
    QFileInfo fi(filePath);
    QString suffix = fi.suffix().toLower();
    QString outPath = filePath;
    QString format;
    if (suffix == "png" || suffix == "jpg" || suffix == "jpeg" || suffix == "bmp") {
        format = (suffix == "jpg") ? "JPG" : suffix.toUpper();
    } else {
        outPath = fi.absolutePath() + "/" + fi.baseName() + ".png";
        format = "PNG";
    }
    QImage toSave = m_resultImage;
    if (format == "JPG" && toSave.hasAlphaChannel())
        toSave = toSave.convertToFormat(QImage::Format_RGB32);
    if (!toSave.save(outPath, format.toLatin1().constData())) {
        showWarning("错误: 保存失败：" + outPath);
        return false;
    }
    setCurrentFolder(QUrl::fromLocalFile(fi.absolutePath()));
    setStatusMessage("保存成功：" + outPath);
    return true;
}

void ImageProcessor::reset() {
    m_type = EMPTY;
    m_sourceFileName.clear();
    setSourceImage(QImage());
    setResultImage(QImage());
    setHistogramImage(QImage());
    configureParameters();
    setWindowTitle("图像处理实验平台");
    setStatusMessage(QString());
    setPlateText(QString());
}

void ImageProcessor::showHistogram() {
    if (m_sourceImage.isNull()) {
        showWarning("错误: 您还没有打开一个图像文件，无法生成直方图");
        return;
    }
    QImage hist = HistogramUtil::calculateGrayscaleHistogram(m_sourceImage).toImage();
    if (hist.isNull()) {
        showWarning("错误: 生成灰度直方图失败");
        return;
    }
    setHistogramImage(hist);
    setStatusMessage("灰度直方图已生成");
}

// ----------------------- 参数面板配置 -----------------------
void ImageProcessor::configureParameters() {
    // 默认值
    m_param1Label.clear();
    m_param1Minimum = 0;
    m_param1Maximum = 100;
    m_param1Value = 0;
    m_needsParam2 = false;
    m_param2Label.clear();
    m_param2Minimum = 0;
    m_param2Maximum = 500;
    m_param2Value = 0;
    m_needsOption = false;
    m_optionLabel.clear();
    m_optionList.clear();
    m_optionIndex = 0;

    switch (m_type) {
    case BASED_ROTATE:
        m_operationTitle = "图像旋转-请输入角度（0-360）";
        m_param1Label = "角度(度)"; m_param1Maximum = 360; break;
    case BASED_RESIZE:
        m_operationTitle = "图像缩放-请输入缩放比例（1-100）";
        m_param1Label = "缩放比例(%)"; m_param1Minimum = 1; m_param1Maximum = 100; m_param1Value = 50; break;
    case BASED_HORIZONTAL_FLIP:
        m_operationTitle = "图像水平翻转"; break;
    case BASED_VERTICAL_FLIP:
        m_operationTitle = "图像垂直翻转"; break;
    case BASED_GRAY:
        m_operationTitle = "图像灰度化"; break;
    case SEGMENTATION_EDGE:
        m_operationTitle = "canny边缘检测";
        m_needsParam2 = true;
        m_param1Label = "低阈值"; m_param1Maximum = 500; m_param1Value = 50;
        m_param2Label = "高阈值"; m_param2Maximum = 500; m_param2Value = 150; break;
    case SEGMENTATION_LINE_DETECTION:
        m_operationTitle = "直线检测";
        m_needsParam2 = true;
        m_param1Label = "低阈值"; m_param1Maximum = 500; m_param1Value = 50;
        m_param2Label = "高阈值"; m_param2Maximum = 500; m_param2Value = 150; break;
    case SEGMENTATION_THRESHOLD:
        m_operationTitle = "阈值处理——请输入阈值（0-255）";
        m_param1Label = "阈值"; m_param1Maximum = 255; m_param1Value = 127; break;
    case FREQUENCY_FOURIER:
        m_operationTitle = "傅里叶变换"; break;
    case FREQUENCY_LOW_PASS_FILTER:
        m_operationTitle = "低通滤波-请输入低通滤波核大小（0-15）";
        m_param1Label = "滤波半径"; m_param1Maximum = 15; m_param1Value = 5; break;
    case DIMENSION_EQUALIZATION_HISTOGRAM:
        m_operationTitle = "直方图均衡化"; break;
    case DIMENSION_MEDIAN_FILTER:
        m_operationTitle = "中值滤波-请输入中值滤波半径（0-15）";
        m_param1Label = "滤波半径"; m_param1Maximum = 15; m_param1Value = 3; break;
    case DIMENSION_GAUSSIAN_FILTER:
        m_operationTitle = "高斯滤波参数";
        m_needsParam2 = true;
        m_param1Label = "高斯滤波核"; m_param1Maximum = 15; m_param1Value = 3;
        m_param2Label = "高斯核标准差"; m_param2Value = 1; break;
    case DIMENSION_LAPLACIAN_FILTER:
        m_operationTitle = "拉普拉斯滤波-请输入拉普拉斯滤波半径（0-15）";
        m_param1Label = "滤波半径"; m_param1Maximum = 15; m_param1Value = 3; break;
    case DIMENSION_SOBEL_FILTER:
        m_operationTitle = "索贝尔滤波-请输入索贝尔滤波半径（0-15）";
        m_param1Label = "滤波半径"; m_param1Maximum = 15; m_param1Value = 3; break;
    case MORPHOLOGY_DILATION:
        m_operationTitle = "膨胀-请输入膨胀核大小（0-50）";
        m_param1Label = "核大小"; m_param1Maximum = 50; m_param1Value = 3; break;
    case MORPHOLOGY_EROSION:
        m_operationTitle = "腐蚀-请输入腐蚀核大小（0-50）";
        m_param1Label = "核大小"; m_param1Maximum = 50; m_param1Value = 3; break;
    case MORPHOLOGY_OPENING:
        m_operationTitle = "开运算-请输入开运算核大小（0-50）";
        m_param1Label = "核大小"; m_param1Maximum = 50; m_param1Value = 3; break;
    case MORPHOLOGY_CLOSING:
        m_operationTitle = "闭运算-请输入闭运算核大小（0-50）";
        m_param1Label = "核大小"; m_param1Maximum = 50; m_param1Value = 3; break;
    case ART_COLOR_PAINTING:
        m_operationTitle = "水彩艺术画"; break;
    case ART_OLD_PHOTO:
        m_operationTitle = "怀旧照片"; break;
    case ART_SKETCH:
        m_operationTitle = "素描"; break;
    case NOISE:
        m_operationTitle = "添加噪声-请选择噪声类型";
        m_needsOption = true;
        m_optionLabel = "噪声类型";
        m_optionList = QStringList{"斑点噪声", "椒盐噪声", "高斯噪声"};
        break;
    case PLATE:
        m_operationTitle = "车牌识别-请选择识别模式";
        m_needsOption = true;
        m_optionLabel = "识别模式";
        m_optionList = QStringList{"HyperLPR 深度学习", "传统算法"};
        break;
    case EMPTY:
    default:
        m_operationTitle = "操作参数"; break;
    }
    // 切换到非车牌识别操作时清空车牌文本
    if (m_type != PLATE)
        setPlateText(QString());
    emit operationConfigChanged();
    emit param1ValueChanged();
    emit param2ValueChanged();
    emit optionIndexChanged();
}

// ----------------------- 调起 Manipulator 执行（已迁移至 ProcessorWorker） -----------------------

