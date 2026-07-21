#include "hyperlpr_wrapper.hpp"
#include <QDebug>

HyperLPRWrapper::HyperLPRWrapper() {}

HyperLPRWrapper::~HyperLPRWrapper() {
    release();
}

bool HyperLPRWrapper::init(const std::string& model_path) {
    qDebug() << "[HyperLPR-Wrapper] init() called, model_path:" << QString::fromStdString(model_path);
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
    
    qDebug() << "[HyperLPR-Wrapper] HLPR_CreateContext() called";
    ctx = HLPR_CreateContext(&config);
    if (!ctx) {
        qDebug() << "[HyperLPR-Wrapper] HLPR_CreateContext() failed, ctx is nullptr";
        return false;
    }
    qDebug() << "[HyperLPR-Wrapper] HLPR_CreateContext() succeeded";
    
    HREESULT status = HLPR_ContextQueryStatus(ctx);
    qDebug() << "[HyperLPR-Wrapper] HLPR_ContextQueryStatus() returned:" << status;
    if (status != Ok) {
        qDebug() << "[HyperLPR-Wrapper] Status is not Ok, expected Ok(" << Ok << ")";
        HLPR_ReleaseContext(ctx);
        ctx = nullptr;
        return false;
    }
    qDebug() << "[HyperLPR-Wrapper] Context status is Ok";
    
    qDebug() << "[HyperLPR-Wrapper] HLPR_CreateDataBufferEmpty() called";
    buffer = HLPR_CreateDataBufferEmpty();
    if (!buffer) {
        qDebug() << "[HyperLPR-Wrapper] HLPR_CreateDataBufferEmpty() failed";
        HLPR_ReleaseContext(ctx);
        ctx = nullptr;
        return false;
    }
    qDebug() << "[HyperLPR-Wrapper] HLPR_CreateDataBufferEmpty() succeeded";
    
    qDebug() << "[HyperLPR-Wrapper] init() succeeded";
    return true;
}

bool HyperLPRWrapper::recognize(const cv::Mat& image, std::vector<PlateResult>& output_results) {
    qDebug() << "[HyperLPR-Wrapper] recognize() called";
    
    if (!ctx) {
        qDebug() << "[HyperLPR-Wrapper] recognize() failed: ctx is null";
        return false;
    }
    
    if (image.empty()) {
        qDebug() << "[HyperLPR-Wrapper] recognize() failed: image is empty";
        return false;
    }
    
    qDebug() << "[HyperLPR-Wrapper] image size:" << image.cols << "x" << image.rows << ", channels:" << image.channels();
    
    cv::Mat rgb_image;
    cv::cvtColor(image, rgb_image, cv::COLOR_BGR2RGB);
    
    HLPR_ImageData data = {0};
    data.data = const_cast<uint8_t*>(rgb_image.ptr<uint8_t>(0));
    data.width = rgb_image.cols;
    data.height = rgb_image.rows;
    data.format = STREAM_RGB;
    data.rotation = CAMERA_ROTATION_0;
    
    qDebug() << "[HyperLPR-Wrapper] HLPR_CreateDataBuffer() called";
    P_HLPR_DataBuffer buffer = HLPR_CreateDataBuffer(&data);
    if (!buffer) {
        qDebug() << "[HyperLPR-Wrapper] HLPR_CreateDataBuffer() failed";
        return false;
    }
    qDebug() << "[HyperLPR-Wrapper] HLPR_CreateDataBuffer() succeeded";
    
    HLPR_PlateResultList raw_results{};
    HREESULT ret = HLPR_ContextUpdateStream(ctx, buffer, &raw_results);
    qDebug() << "[HyperLPR-Wrapper] HLPR_ContextUpdateStream() returned:" << ret;
    
    if (ret != Ok) {
        qDebug() << "[HyperLPR-Wrapper] recognize() failed: HLPR_ContextUpdateStream returned not Ok";
        HLPR_ReleaseDataBuffer(buffer);
        return false;
    }
    
    qDebug() << "[HyperLPR-Wrapper] HLPR_ContextUpdateStream() succeeded, plate_size:" << raw_results.plate_size;
    
    output_results.clear();
    float confidence_threshold = 0.5f;
    for (unsigned long i = 0; i < raw_results.plate_size; ++i) {
        float conf = raw_results.plates[i].text_confidence;
        if (conf < confidence_threshold) {
            qDebug() << "[HyperLPR-Wrapper] plate[" << i << "] skipped: confidence=" << conf << " < threshold=" << confidence_threshold;
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
        qDebug() << "[HyperLPR-Wrapper] plate[" << i << "]: code=" << QString::fromStdString(pr.code) 
                 << ", confidence=" << pr.confidence;
    }
    
    HLPR_ReleaseDataBuffer(buffer);
    
    qDebug() << "[HyperLPR-Wrapper] recognize() succeeded, output_results.size():" << output_results.size();
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
