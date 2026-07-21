//
// Created by Ayuan on 2024/4/9.
//
#include "manipulator.hpp"

Manipulator::Manipulator(std::string &image_path) {
    if (image_path.empty()){
        throw std::runtime_error("image path is empty");
    }
    image = cv::imread(image_path,cv::IMREAD_UNCHANGED);
}

Manipulator::Manipulator(const cv::Mat &mat) {
    if (mat.empty()) {
        throw std::runtime_error("image matrix is empty");
    }
    image = mat.clone();   // 拷贝一份，避免影响调用方的原图
}
