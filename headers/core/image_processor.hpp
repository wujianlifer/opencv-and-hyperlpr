//
// ImageProcessor：主窗口的 C++ 后端/数据模型。
// 负责持有源图像与处理结果、调度各 Manipulator 完成图像处理，
// 并向 QML 暴露操作类型、参数配置与图像。
//
#ifndef QTIMAGEPROCESS_IMAGE_PROCESSOR_HPP
#define QTIMAGEPROCESS_IMAGE_PROCESSOR_HPP

#include <QObject>
#include <QImage>
#include <QString>
#include <QStringList>
#include <QPixmap>
#include <QSize>
#include <QThread>
#include <QTimer>
#include <QUrl>

#include "globe_define_words.h"
#include "based.hpp"
#include "segmentation_manipulator.hpp"
#include "frequency_manipulator.hpp"
#include "dimension_manipulator.hpp"
#include "morphology_manipulator.hpp"
#include "arts_manipulator.hpp"
#include <util.hpp>
#include "image_provider.hpp"

class ProcessorWorker;   // 前向声明，避免头文件循环依赖

class ImageProcessor : public QObject {
    Q_OBJECT
public:
    // 操作类型枚举（与 globe_define_words.h 中 ProcessType 一一对应）
    enum Operation {
        EMPTY = ::EMPTY,
        BASED_ROTATE = ::BASED_ROTATE,
        BASED_HORIZONTAL_FLIP = ::BASED_HORIZONTAL_FLIP,
        BASED_VERTICAL_FLIP = ::BASED_VERTICAL_FLIP,
        BASED_RESIZE = ::BASED_RESIZE,
        BASED_GRAY = ::BASED_GRAY,
        SEGMENTATION_EDGE = ::SEGMENTATION_EDGE,
        SEGMENTATION_THRESHOLD = ::SEGMENTATION_THRESHOLD,
        SEGMENTATION_LINE_DETECTION = ::SEGMENTATION_LINE_DETECTION,
        FREQUENCY_FOURIER = ::FREQUENCY_FOURIER,
        FREQUENCY_LOW_PASS_FILTER = ::FREQUENCY_LOW_PASS_FILTER,
        DIMENSION_EQUALIZATION_HISTOGRAM = ::DIMENSION_EQUALIZATION_HISTOGRAM,
        DIMENSION_MEDIAN_FILTER = ::DIMENSION_MEDIAN_FILTER,
        DIMENSION_GAUSSIAN_FILTER = ::DIMENSION_GAUSSIAN_FILTER,
        DIMENSION_LAPLACIAN_FILTER = ::DIMENSION_LAPLACIAN_FILTER,
        DIMENSION_SOBEL_FILTER = ::DIMENSION_SOBEL_FILTER,
        MORPHOLOGY_DILATION = ::MORPHOLOGY_DILATION,
        MORPHOLOGY_EROSION = ::MORPHOLOGY_EROSION,
        MORPHOLOGY_OPENING = ::MORPHOLOGY_OPENING,
        MORPHOLOGY_CLOSING = ::MORPHOLOGY_CLOSING,
        ART_COLOR_PAINTING = ::ART_COLOR_PAINTING,
        ART_OLD_PHOTO = ::ART_OLD_PHOTO,
        ART_SKETCH = ::ART_SKETCH,
        // 以下为不属于 ProcessType 的扩展操作，用独立大值避免冲突
        NOISE = 1000,
        PLATE = 1001,
    };
    Q_ENUM(Operation)

    // 噪声类型（供 QML 通过 ImageProcessor.SALT 等访问）
    enum NoiseType {
        EMPTY_NOISE = 0,
        SALT,
        GAUSSIAN,
        SPECKLE
    };
    Q_ENUM(NoiseType)

    // 车牌识别模式（供 QML 通过 ImageProcessor.HYPERLPR 等访问）
    enum PlateMode {
        TRADITIONAL = 0,
        HYPERLPR
    };
    Q_ENUM(PlateMode)

    // 消息级别（供 QML 判断是否需要弹窗提示，取代脆弱的字符串判断）
    enum MessageLevel {
        Info = 0,
        Warning = 1,
        Error = 2
    };
    Q_ENUM(MessageLevel)

    // 提交给工作线程的处理请求（值类型，跨线程复制安全）
    struct ProcessRequest {
        Operation type = EMPTY;
        int param1 = 0;
        double param2 = 0.0;
        int optionIndex = 0;
        int requestId = 0;
        QImage sourceImage;   // 内存中已加载的源图，避免后台线程重复读盘
    };

    explicit ImageProcessor(QObject* parent = nullptr);
    ~ImageProcessor();

    // ---- 供 QML 读取的属性 ----
    Q_PROPERTY(QImage sourceImage READ sourceImage NOTIFY sourceImageChanged)
    Q_PROPERTY(QImage resultImage READ resultImage NOTIFY resultImageChanged)
    Q_PROPERTY(int sourceVersion READ sourceVersion NOTIFY sourceImageChanged)
    Q_PROPERTY(int resultVersion READ resultVersion NOTIFY resultImageChanged)
    Q_PROPERTY(QImage histogramImage READ histogramImage NOTIFY histogramImageChanged)
    Q_PROPERTY(int histogramVersion READ histogramVersion NOTIFY histogramImageChanged)
    Q_PROPERTY(bool hasHistogram READ hasHistogram NOTIFY histogramImageChanged)
    Q_PROPERTY(bool hasImage READ hasImage NOTIFY sourceImageChanged)
    Q_PROPERTY(bool hasResult READ hasResult NOTIFY resultImageChanged)
    Q_PROPERTY(QString windowTitle READ windowTitle NOTIFY windowTitleChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString plateText READ plateText NOTIFY plateTextChanged)
    // 是否正在后台线程处理（用于禁用“生成”按钮、显示处理中状态）
    Q_PROPERTY(bool processing READ processing NOTIFY processingChanged)
    // 上次使用的目录（用于打开/保存对话框记忆位置）
    Q_PROPERTY(QUrl currentFolder READ currentFolder NOTIFY currentFolderChanged)
    // 是否开启参数实时预览（勾选后改参数即自动重算，无需点“生成”）
    Q_PROPERTY(bool livePreview READ livePreview WRITE setLivePreview NOTIFY livePreviewChanged)
    // 当前操作是否有可调整参数（无参数时实时预览无意义，勾选框应禁用）
    Q_PROPERTY(bool hasParameters READ hasParameters NOTIFY operationConfigChanged)

    // ---- 参数面板配置 ----
    Q_PROPERTY(QString operationTitle READ operationTitle NOTIFY operationConfigChanged)
    Q_PROPERTY(QString param1Label READ param1Label NOTIFY operationConfigChanged)
    Q_PROPERTY(int param1Minimum READ param1Minimum NOTIFY operationConfigChanged)
    Q_PROPERTY(int param1Maximum READ param1Maximum NOTIFY operationConfigChanged)
    Q_PROPERTY(int param1Value READ param1Value WRITE setParam1Value NOTIFY param1ValueChanged)
    Q_PROPERTY(bool needsParam2 READ needsParam2 NOTIFY operationConfigChanged)
    Q_PROPERTY(QString param2Label READ param2Label NOTIFY operationConfigChanged)
    Q_PROPERTY(double param2Minimum READ param2Minimum NOTIFY operationConfigChanged)
    Q_PROPERTY(double param2Maximum READ param2Maximum NOTIFY operationConfigChanged)
    Q_PROPERTY(double param2Value READ param2Value WRITE setParam2Value NOTIFY param2ValueChanged)
    // 选项参数（下拉选择，供噪声/车牌识别等使用）
    Q_PROPERTY(bool needsOption READ needsOption NOTIFY operationConfigChanged)
    Q_PROPERTY(QString optionLabel READ optionLabel NOTIFY operationConfigChanged)
    Q_PROPERTY(QStringList optionList READ optionList NOTIFY operationConfigChanged)
    Q_PROPERTY(int optionIndex READ optionIndex WRITE setOptionIndex NOTIFY optionIndexChanged)
    // 当前操作是否显示车牌识别文本
    Q_PROPERTY(bool showPlateText READ showPlateText NOTIFY operationConfigChanged)

    QImage sourceImage() const { return m_sourceImage; }
    QImage resultImage() const { return m_resultImage; }
    int sourceVersion() const { return m_sourceVersion; }
    int resultVersion() const { return m_resultVersion; }
    QImage histogramImage() const { return m_histogramImage; }
    int histogramVersion() const { return m_histogramVersion; }
    bool hasHistogram() const { return !m_histogramImage.isNull(); }
    bool hasImage() const { return !m_sourceImage.isNull(); }
    bool hasResult() const { return !m_resultImage.isNull(); }
    QString windowTitle() const { return m_windowTitle; }
    QString statusMessage() const { return m_statusMessage; }
    QString plateText() const { return m_plateText; }
    QUrl currentFolder() const { return m_currentFolder; }

    QString operationTitle() const { return m_operationTitle; }
    QString param1Label() const { return m_param1Label; }
    int param1Minimum() const { return m_param1Minimum; }
    int param1Maximum() const { return m_param1Maximum; }
    int param1Value() const { return m_param1Value; }
    bool needsParam2() const { return m_needsParam2; }
    QString param2Label() const { return m_param2Label; }
    double param2Minimum() const { return m_param2Minimum; }
    double param2Maximum() const { return m_param2Maximum; }
    double param2Value() const { return m_param2Value; }
    bool needsOption() const { return m_needsOption; }
    QString optionLabel() const { return m_optionLabel; }
    QStringList optionList() const { return m_optionList; }
    int optionIndex() const { return m_optionIndex; }
    bool showPlateText() const { return m_type == PLATE; }
    bool processing() const { return m_processing; }
    bool livePreview() const { return m_livePreview; }
    bool hasParameters() const { return m_hasParameters; }

    void setParam1Value(int v);
    void setParam2Value(double v);
    void setOptionIndex(int i);
    void setLivePreview(bool v);

    // 设置图像 provider（在主线程同步推送图像副本，避免 QML 渲染线程跨线程访问）
    void setImageProvider(ImageProvider* provider) { m_provider = provider; }

signals:
    void sourceImageChanged();
    void resultImageChanged();
    void histogramImageChanged();
    void windowTitleChanged();
    void statusMessageChanged();
    void plateTextChanged();
    void operationConfigChanged();
    void param1ValueChanged();
    void param2ValueChanged();
    void optionIndexChanged();
    void processingChanged();
    void currentFolderChanged();
    void livePreviewChanged();
    // 规范化消息信号（level: Info/Warning/Error），供 QML 决定弹窗
    void message(const QString& text, MessageLevel level);
    // 提交处理请求给后台工作线程（跨线程，自动排队）
    void processRequested(const ProcessRequest& req);

public slots:
    // ---- 供 QML 调用的槽 ----
    Q_INVOKABLE bool openImage(const QUrl& path);
    Q_INVOKABLE void selectOperation(int type);
    Q_INVOKABLE void apply();
    Q_INVOKABLE bool saveResult(const QUrl& path);
    Q_INVOKABLE void reset();
    Q_INVOKABLE void showHistogram();
    Q_INVOKABLE void clearResult();
    // 后台线程处理完成后的回调（运行在主线程）
    void onProcessFinished(int requestId, const QImage& result,
                           const QString& plateText, const QString& status, bool success);

private:
    void setStatusMessage(const QString& msg);
    void setSourceImage(const QImage& img);
    void setResultImage(const QImage& img);
    void setHistogramImage(const QImage& img);
    void setWindowTitle(const QString& title);
    void setPlateText(const QString& text);
    void showWarning(const QString& msg);
    void setCurrentFolder(const QUrl& folder);

    // 根据当前操作类型更新参数面板配置
    void configureParameters();
    void setProcessing(bool v);
    // 参数实时预览：防抖调度与执行、参数校验复用
    void scheduleLivePreview();
    void runLivePreview();
    bool validateParams(QString& outMsg);

    QImage m_sourceImage;
    QImage m_resultImage;
    QImage m_histogramImage;
    ImageProvider* m_provider = nullptr;
    int m_sourceVersion = 0;
    int m_resultVersion = 0;
    int m_histogramVersion = 0;
    QString m_windowTitle;
    QString m_statusMessage;
    QString m_plateText;

    Operation m_type = EMPTY;
    QString m_sourceFileName;
    QUrl m_currentFolder;   // 记忆上次打开/保存目录

    QString m_operationTitle;
    QString m_param1Label;
    int m_param1Minimum = 0;
    int m_param1Maximum = 100;
    int m_param1Value = 0;
    bool m_needsParam2 = false;
    QString m_param2Label;
    double m_param2Minimum = 0;
    double m_param2Maximum = 500;
    double m_param2Value = 0;
    bool m_needsOption = false;
    QString m_optionLabel;
    QStringList m_optionList;
    int m_optionIndex = 0;

    // 后台工作线程及其管理
    ProcessorWorker* m_worker = nullptr;
    QThread m_workerThread;
    bool m_processing = false;   // 是否有任务在后台处理
    int m_jobId = 0;             // 任务序号，用于丢弃过期结果
    QTimer* m_previewTimer = nullptr;  // 实时预览防抖计时器（单次触发）
    bool m_livePreview = false;        // 是否开启参数实时预览
    bool m_pendingLivePreview = false; // 处理中收到新参数，结束后再补跑一次
    bool m_hasParameters = false;      // 当前操作是否有可调整参数
};

Q_DECLARE_METATYPE(ImageProcessor::ProcessRequest)

#endif //QTIMAGEPROCESS_IMAGE_PROCESSOR_HPP
