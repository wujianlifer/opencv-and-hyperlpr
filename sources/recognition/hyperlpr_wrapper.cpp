#include "hyperlpr_wrapper.hpp"

HyperLPRWrapper::HyperLPRWrapper() {}

HyperLPRWrapper::~HyperLPRWrapper() {
    release();
}

bool HyperLPRWrapper::init(const std::string& model_path) {
    
    models_path = model_path;
    
    HLPR_ContextConfiguration config{};
    config.models_path = const_cast<char*>(models_path.c_str());
    config.max_num = 5;
    config.threads = 1;
    config.use_half = false;
    config.box_conf_threshold = 0.30f;
    config.nms_threshold = 0.5f;
    config.rec_confidence_threshold = 0.5f;
    config.det_level = DETECT_LEVEL_LOW;
    
    
    ctx = HLPR_CreateContext(&config);
    if (!ctx) {
        
        return false;
    }
    
    
    HREESULT status = HLPR_ContextQueryStatus(ctx);
    
    if (status != Ok) {
        
        HLPR_ReleaseContext(ctx);
        ctx = nullptr;
        return false;
    }
    
    
    
    buffer = HLPR_CreateDataBufferEmpty();
    if (!buffer) {
        
        HLPR_ReleaseContext(ctx);
        ctx = nullptr;
        return false;
    }
    
    
    
    return true;
}

bool HyperLPRWrapper::recognize(const cv::Mat& image, std::vector<PlateResult>& output_results) {
    
    
    if (!ctx) {
        
        return false;
    }
    
    if (image.empty()) {
        
        return false;
    }
    
    
    
    cv::Mat rgb_image;
    cv::cvtColor(image, rgb_image, cv::COLOR_BGR2RGB);
    
    HLPR_ImageData data = {0};
    data.data = const_cast<uint8_t*>(rgb_image.ptr<uint8_t>(0));
    data.width = rgb_image.cols;
    data.height = rgb_image.rows;
    data.format = STREAM_RGB;
    data.rotation = CAMERA_ROTATION_0;
    
    
    P_HLPR_DataBuffer buffer = HLPR_CreateDataBuffer(&data);
    if (!buffer) {
        
        return false;
    }
    
    
    HLPR_PlateResultList raw_results{};
    HREESULT ret = HLPR_ContextUpdateStream(ctx, buffer, &raw_results);
    
    
    if (ret != Ok) {
        
        HLPR_ReleaseDataBuffer(buffer);
        return false;
    }
    
    
    
    output_results.clear();
    float confidence_threshold = 0.5f;
    for (unsigned long i = 0; i < raw_results.plate_size; ++i) {
        float conf = raw_results.plates[i].text_confidence;
        if (conf < confidence_threshold) {
            
            continue;
        }
        PlateResult pr;
        pr.code = raw_results.plates[i].code;
        pr.confidence = conf;
        pr.type = raw_results.plates[i].type;
        pr.x1 = raw_results.plates[i].x1;
        pr.y1 = raw_results.plates[i].y1;
        pr.x2 = raw_results.plates[i].x2;
        pr.y2 = raw_results.plates[i].y2;
        output_results.push_back(pr);
        
    }
    
    HLPR_ReleaseDataBuffer(buffer);
    
    
    return true;
}

void HyperLPRWrapper::release() {
    if (buffer) {
        HLPR_ReleaseDataBuffer(buffer);
        buffer = nullptr;
    }
    if (ctx) {
        HLPR_ReleaseContext(ctx);
        ctx = nullptr;
    }
}

bool HyperLPRWrapper::isInitialized() const {
    return ctx != nullptr && buffer != nullptr;
}
