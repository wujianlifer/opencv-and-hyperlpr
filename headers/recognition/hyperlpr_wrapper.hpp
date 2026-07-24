#ifndef QTIMAGEPROCESS_HYPERLPR_WRAPPER_HPP
#define QTIMAGEPROCESS_HYPERLPR_WRAPPER_HPP

#include "hyper_lpr_sdk.h"
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

struct PlateResult {
    std::string code;
    float confidence;
    int type;
    float x1, y1, x2, y2;
};

class HyperLPRWrapper {
private:
    P_HLPR_Context ctx = nullptr;
    std::string models_path;
    
public:
    HyperLPRWrapper();
    ~HyperLPRWrapper();
    
    bool init(const std::string& model_path);
    bool recognize(const cv::Mat& image, std::vector<PlateResult>& results);
    void release();
    bool isInitialized() const;
};

#endif //QTIMAGEPROCESS_HYPERLPR_WRAPPER_HPP
