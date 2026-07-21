//
// ImageProvider：通过 "image://processor/source"、"image://processor/result"、
// "image://processor/histogram" 向 QML 的 Image 元素提供源图像 / 处理结果 / 直方图。
// requestImage 在 Qt Quick 的渲染线程中被调用，因此本类自行持有图像副本，
// 由主线程在对应信号触发时通过 setSource/setResult/setHistogram 推入（加锁），
// 避免跨线程直接访问 ImageProcessor 的 QImage 成员造成数据竞争。
//

#ifndef QTIMAGEPROCESS_IMAGE_PROVIDER_HPP
#define QTIMAGEPROCESS_IMAGE_PROVIDER_HPP

#include <QQuickImageProvider>
#include <QImage>
#include <QMutex>

class ImageProvider : public QQuickImageProvider {
public:
    explicit ImageProvider();
    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;

    // 由主线程调用，把最新图像副本推入 provider（内部加锁拷贝）
    void setSource(const QImage& img);
    void setResult(const QImage& img);
    void setHistogram(const QImage& img);

private:
    QMutex m_mutex;
    QImage m_source;
    QImage m_result;
    QImage m_histogram;
};

#endif //QTIMAGEPROCESS_IMAGE_PROVIDER_HPP
