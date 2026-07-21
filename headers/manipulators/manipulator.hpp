
#ifndef QTIMAGEPROCESS_MANIPULATOR_HPP
#define QTIMAGEPROCESS_MANIPULATOR_HPP
#include <iostream>
#include <opencv2/opencv.hpp>
#include "globe_define_words.h"
class Manipulator {
protected:
    cv::Mat image;
public:
    Manipulator(){};
    explicit Manipulator(std::string &image_path);
    explicit Manipulator(const cv::Mat &mat);   // 复用内存中的图像，避免重复读盘
    virtual ~Manipulator() {};
};

#endif //QTIMAGEPROCESS_MANIPULATOR_HPP
