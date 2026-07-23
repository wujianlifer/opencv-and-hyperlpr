//
// Created by Ayuan on 2024/3/4.
//

#ifndef QTIMAGEPROCESS_GLOBE_DEFINE_WORDS_H
#define QTIMAGEPROCESS_GLOBE_DEFINE_WORDS_H

#include <QString>
#include <QStandardPaths>
#include <QDir>

// 缓存目录：使用系统规范的 CacheLocation（不污染 exe 目录、不依赖工作目录）
inline std::string cacheDir() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (dir.isEmpty())
        dir = QDir::homePath();
    dir += "/opencvAndHyperlpr/cache";
    QDir().mkpath(dir);
    return (dir + "/").toStdString();
}
#define cache_file_path cacheDir()
enum ProcessType{
    EMPTY=0,
    BASED_ROTATE,
    BASED_HORIZONTAL_FLIP,
    BASED_VERTICAL_FLIP,
    BASED_RESIZE,
    BASED_GRAY,
    SEGMENTATION_EDGE,
    SEGMENTATION_THRESHOLD,
    SEGMENTATION_LINE_DETECTION,
    FREQUENCY_FOURIER,
    FREQUENCY_LOW_PASS_FILTER,
    DIMENSION_EQUALIZATION_HISTOGRAM,
    DIMENSION_MEDIAN_FILTER,
    DIMENSION_GAUSSIAN_FILTER,
    DIMENSION_LAPLACIAN_FILTER,
    DIMENSION_SOBEL_FILTER,
    MORPHOLOGY_DILATION,
    MORPHOLOGY_EROSION,
    MORPHOLOGY_OPENING,
    MORPHOLOGY_CLOSING,
    ART_COLOR_PAINTING,
    ART_OLD_PHOTO,
    ART_SKETCH,
};

#endif //QTIMAGEPROCESS_GLOBE_DEFINE_WORDS_H
