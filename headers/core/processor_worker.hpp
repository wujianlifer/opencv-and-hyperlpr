//
// ProcessorWorker：后台工作线程中的处理单元。
// 承载所有耗时的图像处理（基础操作、噪声、车牌识别），运行在独立 QThread 中，
// 处理完成后通过 finished 信号把结果回传给主线程的 ImageProcessor。
// 本类不直接访问任何 UI / QML 相关成员，所有结果通过信号传出。
//

#ifndef QTIMAGEPROCESS_PROCESSOR_WORKER_HPP
#define QTIMAGEPROCESS_PROCESSOR_WORKER_HPP

#include <QObject>
#include <QImage>
#include <QString>
#include "image_processor.hpp"
#include "hyperlpr_wrapper.hpp"

class ProcessorWorker : public QObject {
    Q_OBJECT
public:
    explicit ProcessorWorker(QObject* parent = nullptr);

public slots:
    // 收到主线程提交的处理请求后，在后台线程中执行
    void process(const ImageProcessor::ProcessRequest& req);

signals:
    // 处理完成（运行在主线程的槽中接收）
    void finished(int requestId, const QImage& result,
                  const QString& plateText, const QString& status, bool success);

private:
    // 基础/分割/频率域/空间域/形态学/艺术效果等，返回结果图（空表示失败）
    QImage runManipulator(const ImageProcessor::ProcessRequest& req);
    void doNoise(const ImageProcessor::ProcessRequest& req,
                 QImage& result, QString& status, bool& success);
    void doPlate(const ImageProcessor::ProcessRequest& req,
                 QImage& result, QString& plateText, QString& status, bool& success);

    HyperLPRWrapper m_hyperlpr;   // 仅在后台线程访问，避免主线程阻塞
};

#endif //QTIMAGEPROCESS_PROCESSOR_WORKER_HPP
