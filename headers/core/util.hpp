
#ifndef QTIMAGEPROCESS_UTIL_HPP
#define QTIMAGEPROCESS_UTIL_HPP
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <opencv2/core/mat.hpp>
class HistogramUtil {
public:
static QPixmap calculateGrayscaleHistogram(const QImage &image);
};
class ImageUtil {
public:
    static QPixmap convertToQPixmap(const cv::Mat &image);
    // 把 QImage 转换为 OpenCV 可直接使用的 cv::Mat（RGB32/ARGB32/RGB888）
    static cv::Mat matFromQImage(const QImage &img);
    // 噪声（基于 cv::Mat，供后台线程复用内存中的图像）
    static QPixmap addSaltNoise(const cv::Mat &image);
    static QPixmap addGaussianNoise(const cv::Mat &image);
    static QPixmap addSpeckleNoise(const cv::Mat &image);
    static QPixmap addSimulationNoise(const cv::Mat &image);
    // 噪声（兼容旧接口：按路径读取后再处理）
    static QPixmap addSaltNoise(const QString& input);
    static void addSaltNoiseGray(cv::Mat &image);
    static void addSaltNoiseColor(cv::Mat &image);
    static void addSaltNoiseWithAlpha(cv::Mat &image);
    static QPixmap addGaussianNoise(const QString& input);
    static QPixmap addSpeckleNoise(const QString& input);
    static QPixmap addSimulationNoise(const QString& input);
    static QPixmap getPixmapFromFile(const QString& filePath);
};

#endif //QTIMAGEPROCESS_UTIL_HPP
