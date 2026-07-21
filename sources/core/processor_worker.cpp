//
// ProcessorWorker 实现：所有耗时图像处理均在此后台线程中执行。
//

#include "processor_worker.hpp"

#include <vector>
#include "globe_define_words.h"
#include "based.hpp"
#include "segmentation_manipulator.hpp"
#include "frequency_manipulator.hpp"
#include "dimension_manipulator.hpp"
#include "morphology_manipulator.hpp"
#include "arts_manipulator.hpp"
#include <util.hpp>
#include "hyperlpr_wrapper.hpp"
#include "licence_recognition_util.hpp"

#include <QPainter>
#include <QCoreApplication>
#include <opencv2/opencv.hpp>

ProcessorWorker::ProcessorWorker(QObject* parent) : QObject(parent) {}

void ProcessorWorker::process(const ImageProcessor::ProcessRequest& req) {
    QImage result;
    QString plateText;
    QString status;
    bool success = true;

    if (req.type == ImageProcessor::NOISE) {
        doNoise(req, result, status, success);
    } else if (req.type == ImageProcessor::PLATE) {
        doPlate(req, result, plateText, status, success);
    } else {
        result = runManipulator(req);
        if (result.isNull()) {
            status = (req.type == ImageProcessor::BASED_GRAY)
                         ? "错误: 无法灰度化图片，请检查图片是否为有效图片"
                         : "错误: 操作失败，请检查图像或参数";
            success = false;
        }
    }

    emit finished(req.requestId, result, plateText, status, success);
}

void ProcessorWorker::doNoise(const ImageProcessor::ProcessRequest& req,
                              QImage& result, QString& status, bool& success) {
    // 选项顺序：0 斑点, 1 椒盐, 2 高斯
    int nt = (req.optionIndex == 1) ? ImageProcessor::SALT
             : (req.optionIndex == 2) ? ImageProcessor::GAUSSIAN
                                       : ImageProcessor::SPECKLE;
    cv::Mat src = ImageUtil::matFromQImage(req.sourceImage);
    if (src.empty()) {
        status = "错误: 无法加载图片";
        success = false;
        return;
    }
    switch (nt) {
    case ImageProcessor::SALT:
        result = ImageUtil::addSaltNoise(src).toImage();
        break;
    case ImageProcessor::GAUSSIAN:
        result = ImageUtil::addGaussianNoise(src).toImage();
        break;
    case ImageProcessor::SPECKLE:
        result = ImageUtil::addSpeckleNoise(src).toImage();
        break;
    default:
        break;
    }
    if (result.isNull()) {
        status = "错误: 添加噪声失败";
        success = false;
    } else {
        status = "噪声已添加";
    }
}

void ProcessorWorker::doPlate(const ImageProcessor::ProcessRequest& req,
                              QImage& result, QString& plateText,
                              QString& status, bool& success) {
    // 选项顺序：0 HyperLPR, 1 传统算法
    int pm = (req.optionIndex == 1) ? ImageProcessor::TRADITIONAL
                                    : ImageProcessor::HYPERLPR;
    if (pm == ImageProcessor::HYPERLPR) {
        if (!m_hyperlpr.isInitialized()) {
            QString model_path = QCoreApplication::applicationDirPath()
                                 + "/resource/models/r2_mobile";
            if (!m_hyperlpr.init(model_path.toStdString())) {
                status = "HyperLPR 模型初始化失败，请检查模型路径";
                success = false;
                return;
            }
        }
        cv::Mat image = ImageUtil::matFromQImage(req.sourceImage);
        if (image.empty()) {
            status = "无法加载图片";
            success = false;
            return;
        }
        std::vector<PlateResult> results;
        if (!m_hyperlpr.recognize(image, results)) {
            status = "HyperLPR 识别失败";
            success = false;
            return;
        }
        // 在图上绘制绿色边框与蓝色车牌文字
        for (const auto& r : results) {
            cv::rectangle(image, cv::Point(r.x1, r.y1),
                          cv::Point(r.x2, r.y2), cv::Scalar(0, 255, 0), 2);
        }
        QPixmap pix = ImageUtil::convertToQPixmap(image);
        if (!pix.isNull()) {
            QPainter painter(&pix);
            QFont font("Microsoft YaHei", 20, QFont::Bold);
            painter.setFont(font);
            painter.setPen(QColor(0, 0, 255));
            for (const auto& r : results) {
                QString text = QString("%1 (%2)")
                                   .arg(QString::fromStdString(r.code))
                                   .arg(r.confidence, 0, 'f', 3);
                painter.drawText(
                    QPoint(static_cast<int>(r.x1), static_cast<int>(r.y1) - 10),
                    text);
            }
        }
        result = pix.toImage();
        QString pt;
        for (const auto& r : results) {
            if (!pt.isEmpty()) pt += ", ";
            pt += QString("%1 (%2)")
                      .arg(QString::fromStdString(r.code))
                      .arg(r.confidence, 0, 'f', 3);
        }
        plateText = results.empty() ? "未识别到车牌" : pt;
        status = "HyperLPR 识别完成";
    } else {
        cv::Mat src = ImageUtil::matFromQImage(req.sourceImage);
        if (src.empty()) {
            status = "无法加载图片";
            success = false;
            return;
        }
        LicenceRecognition rec(src);
        int r = rec.recognizePlate();
        if (r < 0) {
            status = "传统算法识别失败";
            success = false;
            return;
        }
        cv::Mat plate = rec.getPlateImage();
        if (plate.empty()) {
            status = "传统算法未检测到车牌";
            success = false;
            return;
        }
        result = ImageUtil::convertToQPixmap(plate).toImage();
        plateText = QString::fromStdString(rec.getPlate());
        status = "传统算法识别完成";
    }
}

QImage ProcessorWorker::runManipulator(const ImageProcessor::ProcessRequest& req) {
    cv::Mat src = ImageUtil::matFromQImage(req.sourceImage);
    if (src.empty())
        return {};
    cv::Mat mat;
    switch (req.type) {
    case ImageProcessor::BASED_ROTATE: {
        ImageBasedManipulator m(src);
        mat = m.rotate(req.param1); break; }
    case ImageProcessor::BASED_HORIZONTAL_FLIP: {
        ImageBasedManipulator m(src);
        mat = m.flip_horizontal(); break; }
    case ImageProcessor::BASED_VERTICAL_FLIP: {
        ImageBasedManipulator m(src);
        mat = m.flip_vertical(); break; }
    case ImageProcessor::BASED_RESIZE: {
        ImageBasedManipulator m(src);
        mat = m.resize(req.param1); break; }
    case ImageProcessor::BASED_GRAY: {
        ImageBasedManipulator m(src);
        mat = m.gray(); break; }
    case ImageProcessor::SEGMENTATION_EDGE: {
        SegmentationManipulator m(src);
        mat = m.detect_edges(req.param1, static_cast<int>(req.param2)); break; }
    case ImageProcessor::SEGMENTATION_LINE_DETECTION: {
        SegmentationManipulator m(src);
        mat = m.detect_lines(req.param1, static_cast<int>(req.param2)); break; }
    case ImageProcessor::SEGMENTATION_THRESHOLD: {
        SegmentationManipulator m(src);
        mat = m.apply_threshold(req.param1); break; }
    case ImageProcessor::FREQUENCY_FOURIER: {
        FrequencyManipulator m(src);
        mat = m.fourier_transform(); break; }
    case ImageProcessor::FREQUENCY_LOW_PASS_FILTER: {
        FrequencyManipulator m(src);
        mat = m.low_pass_filter(req.param1); break; }
    case ImageProcessor::DIMENSION_EQUALIZATION_HISTOGRAM: {
        DimensionManipulator m(src);
        mat = m.histogram_equalization(); break; }
    case ImageProcessor::DIMENSION_MEDIAN_FILTER: {
        DimensionManipulator m(src);
        mat = m.median_filter(req.param1); break; }
    case ImageProcessor::DIMENSION_GAUSSIAN_FILTER: {
        DimensionManipulator m(src);
        mat = m.Gauss_filter(req.param1, req.param2); break; }
    case ImageProcessor::DIMENSION_LAPLACIAN_FILTER: {
        DimensionManipulator m(src);
        mat = m.laplacian_filter(req.param1); break; }
    case ImageProcessor::DIMENSION_SOBEL_FILTER: {
        DimensionManipulator m(src);
        mat = m.sobel_filter(req.param1); break; }
    case ImageProcessor::MORPHOLOGY_DILATION: {
        MorphologyManipulator m(src);
        mat = m.dilation(req.param1); break; }
    case ImageProcessor::MORPHOLOGY_EROSION: {
        MorphologyManipulator m(src);
        mat = m.erosion(req.param1); break; }
    case ImageProcessor::MORPHOLOGY_OPENING: {
        MorphologyManipulator m(src);
        mat = m.opening(req.param1); break; }
    case ImageProcessor::MORPHOLOGY_CLOSING: {
        MorphologyManipulator m(src);
        mat = m.closing(req.param1); break; }
    case ImageProcessor::ART_COLOR_PAINTING: {
        ArtsManipulator m(src);
        mat = m.ColorPainting(); break; }
    case ImageProcessor::ART_OLD_PHOTO: {
        ArtsManipulator m(src);
        mat = m.OldPhoto(); break; }
    case ImageProcessor::ART_SKETCH: {
        ArtsManipulator m(src);
        mat = m.Sketch(); break; }
    case ImageProcessor::EMPTY:
    default:
        return {};
    }
    if (req.type == ImageProcessor::FREQUENCY_FOURIER
        || req.type == ImageProcessor::DIMENSION_LAPLACIAN_FILTER)
        return ImageUtil::convertToQPixmap(mat, 5).toImage();
    return ImageUtil::convertToQPixmap(mat).toImage();
}
