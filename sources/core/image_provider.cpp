//
// ImageProvider 实现
//
#include "image_provider.hpp"

ImageProvider::ImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image) {}

void ImageProvider::setSource(const QImage& img) {
    QMutexLocker lock(&m_mutex);
    m_source = img;
}
void ImageProvider::setResult(const QImage& img) {
    QMutexLocker lock(&m_mutex);
    m_result = img;
}
void ImageProvider::setHistogram(const QImage& img) {
    QMutexLocker lock(&m_mutex);
    m_histogram = img;
}

QImage ImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize) {
    QMutexLocker lock(&m_mutex);
    // 去掉 URL 中 "?" 后面的查询串（如 "source?v=1" -> "source"）
    QString key = id;
    int qpos = key.indexOf('?');
    if (qpos >= 0)
        key = key.left(qpos);
    QImage img;
    if (key == "source")
        img = m_source;
    else if (key == "result")
        img = m_result;
    else if (key == "histogram")
        img = m_histogram;
    // 按请求尺寸返回缩略图，避免超大图在渲染侧重复缩放带来的开销
    if (!img.isNull() && requestedSize.isValid() && !requestedSize.isEmpty()
        && (img.width() > requestedSize.width() || img.height() > requestedSize.height())) {
        img = img.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    if (size)
        *size = img.size();
    return img;
}
