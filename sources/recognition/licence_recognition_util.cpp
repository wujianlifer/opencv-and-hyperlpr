#include <licence_recognition_util.hpp>
#include <QFile>
#include <QString>

// 从编译进 exe 的 Qt 资源（templates.qrc，前缀 /templates）中读取字符模板图片。
// 模板固化在二进制内，运行目录无 pictures/ 文件夹、用户也无法在外部篡改，
// 保证传统车牌识别结果稳定、可信。
static cv::Mat loadQrcImage(const QString& resourcePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return cv::Mat();
    }
    const QByteArray data = file.readAll();
    std::vector<uchar> buf(data.begin(), data.end());
    return cv::imdecode(buf, cv::IMREAD_GRAYSCALE);
}

std::string LicenceRecognitionUtil::getNumsPath()
{
    return ":/templates/pictures/nums/";
}

std::string LicenceRecognitionUtil::getAlphabetPath()
{
    return ":/templates/pictures/alphabet/";
}

std::string LicenceRecognitionUtil::getProvincePath()
{
    return ":/templates/pictures/province/";
}

LicenceRecognition::~LicenceRecognition()
{
    cv::destroyAllWindows();
    character_images.clear();
    province_images.clear();
    template_images.clear();
    alphabet_images.clear();
};

LicenceRecognition::LicenceRecognition() = default;

LicenceRecognition::LicenceRecognition(std::string& path)
{
    try
    {
        this->image = cv::imread(path, cv::IMREAD_UNCHANGED);
        this->gray_image = cv::Mat::zeros(image.size(), CV_8UC1);
    }
    catch (cv::Exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}

LicenceRecognition::LicenceRecognition(const cv::Mat& mat)
{
    try
    {
        if (mat.empty())
            throw cv::Exception(0, "image matrix is empty", __func__, __FILE__, __LINE__);
        this->image = mat.clone();
        this->gray_image = cv::Mat::zeros(image.size(), CV_8UC1);
    }
    catch (cv::Exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}

void LicenceRecognition::preprocessImage()
{
    cv::cvtColor(image, gray_image, cv::COLOR_BGR2GRAY);

    cv::Mat equalized;
    cv::equalizeHist(gray_image, equalized);

    cv::GaussianBlur(equalized, gaussian_blur_image, cv::Size(5, 5), 0);

    cv::Mat grad_x, abs_grad_x, grad_y, abs_grad_y;
    cv::Sobel(gaussian_blur_image, grad_x, CV_16S, 1, 0, 3);
    cv::Sobel(gaussian_blur_image, grad_y, CV_16S, 0, 1, 3);
    cv::convertScaleAbs(grad_x, abs_grad_x);
    cv::convertScaleAbs(grad_y, abs_grad_y);
    cv::addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, sobel_image);

    cv::threshold(sobel_image, binary_image, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(25, 9));
    cv::morphologyEx(binary_image, close_image, cv::MORPH_CLOSE, element);
}

cv::Mat LicenceRecognition::detectColorPlate()
{
    cv::Mat hsv_image;
    cv::cvtColor(image, hsv_image, cv::COLOR_BGR2HSV);

    cv::Mat blue_mask, yellow_mask, white_mask;
    cv::inRange(hsv_image, cv::Scalar(100, 43, 46), cv::Scalar(124, 255, 255), blue_mask);
    cv::inRange(hsv_image, cv::Scalar(26, 43, 46), cv::Scalar(34, 255, 255), yellow_mask);
    cv::inRange(hsv_image, cv::Scalar(0, 0, 221), cv::Scalar(180, 30, 255), white_mask);

    cv::Mat color_mask = blue_mask | yellow_mask | white_mask;

    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(color_mask, color_mask, cv::MORPH_OPEN, element);
    
    cv::Mat element_close = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(7, 3));
    cv::morphologyEx(color_mask, color_mask, cv::MORPH_CLOSE, element_close);

    return color_mask;
}

void LicenceRecognition::extractPlateRegion()
{
    marked_image = image.clone();

    cv::Mat color_mask = detectColorPlate();

    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(color_mask, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    

    cv::Rect best_plate;
    float idealAspectRatio = 4.4f;
    float aspectRatioRange = 2.0f;
    int maxArea = 0;
    int imgArea = image.cols * image.rows;
    int minPlateArea = static_cast<int>(imgArea * 0.01);
    int maxPlateArea = static_cast<int>(imgArea * 0.6);
    cv::Rect fallback_candidate;
    int fallback_width = 0;
    std::vector<cv::Point> best_contour;

    

    for (const auto& contour : contours)
    {
        cv::Rect bounding_rect = cv::boundingRect(contour);
        int area = bounding_rect.area();

        cv::RotatedRect rotated_rect = cv::minAreaRect(contour);
        float w = rotated_rect.size.width;
        float h = rotated_rect.size.height;
        float aspect_ratio = std::max(w, h) / std::min(w, h);

        if (aspect_ratio > 1.0f && bounding_rect.width > fallback_width &&
                bounding_rect.width >= image.cols * 0.2 && bounding_rect.height >= image.rows * 0.08)
        {
            fallback_candidate = bounding_rect;
            fallback_width = bounding_rect.width;
        }

        if (area < minPlateArea || area > maxPlateArea ||
                aspect_ratio < (idealAspectRatio - aspectRatioRange) ||
                aspect_ratio > (idealAspectRatio + aspectRatioRange) ||
                bounding_rect.width < 80 || bounding_rect.height < 20)
        {
            continue;
        }

        float aspectScore = 1.0f - std::abs(aspect_ratio - idealAspectRatio) / idealAspectRatio;
        float areaScore = static_cast<float>(area) / maxPlateArea;
        float totalScore = aspectScore * 0.7f + areaScore * 0.3f;

        

        if (totalScore > 0.2f && area > maxArea)
        {
            maxArea = area;
            best_plate = bounding_rect;
            best_contour = contour;
        }
    }

    if (best_plate.area() == 0)
    {
        if (fallback_candidate.area() > 0)
        {
            
            best_plate = fallback_candidate;
        }
        else
        {
            
            best_plate = cv::Rect(0, 0, image.cols, image.rows);
        }
    }

    int padding_x = best_plate.width * 0.2;
    int padding_y = best_plate.height * 0.3;
    int new_x = std::max(0, best_plate.x - padding_x);
    int new_y = std::max(0, best_plate.y - padding_y);
    int new_width = std::min(image.cols - new_x, best_plate.width + padding_x * 2);
    int new_height = std::min(image.rows - new_y, best_plate.height + padding_y * 2);

    cv::Rect expanded_rect(new_x, new_y, new_width, new_height);
    best_rect = expanded_rect;

    plate_corners.clear();
    
    if (!best_contour.empty())
    {
        cv::RotatedRect rotated_rect = cv::minAreaRect(best_contour);
        cv::Point2f rect_points[4];
        rotated_rect.points(rect_points);

        cv::Point2f tl, tr, bl, br;
        tl = tr = bl = br = rect_points[0];
        for (int i = 0; i < 4; i++)
        {
            if (rect_points[i].x + rect_points[i].y < tl.x + tl.y) tl = rect_points[i];
            if (rect_points[i].x - rect_points[i].y > tr.x - tr.y) tr = rect_points[i];
            if (rect_points[i].x - rect_points[i].y < bl.x - bl.y) bl = rect_points[i];
            if (rect_points[i].x + rect_points[i].y > br.x + br.y) br = rect_points[i];
        }

        plate_corners.push_back(tl);
        plate_corners.push_back(tr);
        plate_corners.push_back(bl);
        plate_corners.push_back(br);
        
        
    }
    else
    {
        plate_corners.push_back(cv::Point2f(expanded_rect.x, expanded_rect.y));
        plate_corners.push_back(cv::Point2f(expanded_rect.x + expanded_rect.width, expanded_rect.y));
        plate_corners.push_back(cv::Point2f(expanded_rect.x, expanded_rect.y + expanded_rect.height));
        plate_corners.push_back(cv::Point2f(expanded_rect.x + expanded_rect.width, expanded_rect.y + expanded_rect.height));
        
        
    }

    for (int i = 0; i < plate_corners.size(); i++)
    {
        cv::circle(marked_image, plate_corners[i], 5, cv::Scalar(0, 255, 0), -1);
    }
    for (int i = 0; i < 4; i++)
    {
        cv::line(marked_image, plate_corners[i], plate_corners[(i+1)%4], cv::Scalar(0, 0, 255), 2);
    }

    
    
    for (size_t i = 0; i < plate_corners.size(); i++)
    {
        
    }
}

void LicenceRecognition::correctPlateRegion()
{
    cv::Mat plate_roi = image(best_rect).clone();
    cv::Mat gray;
    cv::cvtColor(plate_roi, gray, cv::COLOR_BGR2GRAY);

    

    if (plate_corners.size() == 4)
    {
        cv::Point2f tl = plate_corners[0];
        cv::Point2f tr = plate_corners[1];
        cv::Point2f bl = plate_corners[2];
        cv::Point2f br = plate_corners[3];

        

        double width_top = cv::norm(tr - tl);
        double width_bottom = cv::norm(br - bl);
        double height_left = cv::norm(bl - tl);
        double height_right = cv::norm(br - tr);

        

        const float ideal_plate_ratio = 4.4f;
        int output_width = static_cast<int>(std::max(width_top, width_bottom) * 1.3);
        int output_height = static_cast<int>(output_width / ideal_plate_ratio);

        if (output_height < 60)
        {
            output_height = 60;
            output_width = static_cast<int>(output_height * ideal_plate_ratio);
        }

        

        std::vector<cv::Point2f> dst_points = {
            cv::Point2f(0, 0),
            cv::Point2f(output_width - 1, 0),
            cv::Point2f(0, output_height - 1),
            cv::Point2f(output_width - 1, output_height - 1)
        };

        cv::Mat perspective_matrix = cv::getPerspectiveTransform(plate_corners, dst_points);

        cv::Mat corrected;
        cv::warpPerspective(image, corrected, perspective_matrix, cv::Size(output_width, output_height));

        if (output_width < output_height)
        {
            cv::rotate(corrected, corrected, cv::ROTATE_90_CLOCKWISE);
            std::swap(output_width, output_height);
        }

        cv::Mat hsv_corrected;
        cv::cvtColor(corrected, hsv_corrected, cv::COLOR_BGR2HSV);
        
        cv::Mat white_mask;
        cv::inRange(hsv_corrected, cv::Scalar(0, 0, 180), cv::Scalar(180, 80, 255), white_mask);
        
        cv::Mat horizontal_proj = cv::Mat::zeros(1, corrected.cols, CV_32S);
        for (int x = 0; x < corrected.cols; x++)
        {
            horizontal_proj.at<int>(0, x) = cv::countNonZero(white_mask.col(x));
        }
        
        int right_boundary = corrected.cols;
        int consecutive_empty = 0;
        for (int x = corrected.cols - 1; x >= corrected.cols * 0.5; x--)
        {
            if (horizontal_proj.at<int>(0, x) < corrected.rows * 0.05)
            {
                consecutive_empty++;
                if (consecutive_empty > 5)
                {
                    right_boundary = x + consecutive_empty;
                    break;
                }
            }
            else
            {
                consecutive_empty = 0;
            }
        }
        
        if (right_boundary < corrected.cols * 0.7)
        {
            right_boundary = corrected.cols;
        }
        
        cv::Rect crop_rect(0, 0, right_boundary, corrected.rows);
        corrected = corrected(crop_rect);

        

        corrected_plate_image = corrected;
        cv::cvtColor(corrected_plate_image, plate_image, cv::COLOR_BGR2GRAY);
        return;
    }

    cv::Mat edges;
    cv::Canny(gray, edges, 50, 150);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(edges, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    

    double max_area = 0;
    std::vector<cv::Point> best_contour;

    for (const auto& contour : contours)
    {
        double area = cv::contourArea(contour);
        if (area > max_area)
        {
            max_area = area;
            best_contour = contour;
        }
    }

    

    cv::RotatedRect rotated_rect = cv::minAreaRect(best_contour);
    float angle = rotated_rect.angle;
    float w = rotated_rect.size.width;
    float h = rotated_rect.size.height;

    if (w < h)
    {
        angle = 90 + angle;
    }

    if (angle > 45) angle -= 90;
    if (angle < -45) angle += 90;

    const float max_rotation_angle = 15.0f;
    if (angle > max_rotation_angle) angle = max_rotation_angle;
    if (angle < -max_rotation_angle) angle = -max_rotation_angle;

    

    if (std::abs(angle) > 0.5f)
    {
        cv::Point2f center(gray.cols / 2.0, gray.rows / 2.0);
        cv::Mat rotation_matrix = cv::getRotationMatrix2D(center, angle, 1.0);

        cv::Mat rotated_plate;
        cv::warpAffine(plate_roi, rotated_plate, rotation_matrix, cv::Size(gray.cols, gray.rows));

        corrected_plate_image = rotated_plate;
        cv::cvtColor(corrected_plate_image, plate_image, cv::COLOR_BGR2GRAY);
    }
    else
    {
        corrected_plate_image = plate_roi;
        cv::cvtColor(corrected_plate_image, plate_image, cv::COLOR_BGR2GRAY);
    }
}

int LicenceRecognition::recognizePlate()
{
    try
    {
        preprocessImage();
        extractPlateRegion();
        correctPlateRegion();
        extractCharacterRegion();
        if (!loadTemplates())
        {
            std::cerr << "Failed to load templates!" << std::endl;
            return -2;
        }

        

        if (character_images.size() < 2)
        {
            throw std::runtime_error("Not enough characters for recognition");
        }

        std::string s, alpha;
        recognizeProvince(s);
        plate += province_map.at(s);

        if (character_images.size() >= 2)
        {
            recognizeAlphabet(alpha);
            plate += alpha;
        }

        for (size_t i = 2; i < character_images.size(); ++i)
        {
            if (i == 2) continue;
            
            std::string str;
            cv::Mat character_image = character_images[i];
            recognizeCharacters(character_image, str);
            plate += str;
        }

        
        printResult();
    }
    catch (cv::Exception& e)
    {
        std::cerr << "recognizePlate error: " << e.what() << std::endl;
        return -1;
    }
    catch (std::exception& e)
    {
        std::cerr << "recognizePlate error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}

void LicenceRecognition::extractCharacterRegion()
{
    cv::Mat color_plate = corrected_plate_image.clone();
    debug_plate_image = color_plate.clone();

    cv::Mat equalized;
    cv::equalizeHist(plate_image, equalized);

    cv::Mat gaussian;
    cv::GaussianBlur(equalized, gaussian, cv::Size(3, 3), 0);

    cv::Mat binary;
    double thresh = cv::threshold(gaussian, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, element);

    element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
    cv::morphologyEx(binary, binary, cv::MORPH_OPEN, element);

    int white_pixels = cv::countNonZero(binary);
    if (white_pixels < binary.total() * 0.05)
    {
        cv::bitwise_not(binary, binary);
    }

    int plate_width = plate_image.cols;
    int plate_height = plate_image.rows;

    cv::Mat vertical_proj = cv::Mat::zeros(1, plate_width, CV_32S);
    for (int x = 0; x < plate_width; x++)
    {
        vertical_proj.at<int>(0, x) = cv::countNonZero(binary.col(x));
    }

    std::vector<int> gaps;
    int gap_start = -1;
    int min_gap_width = 2;
    int gap_threshold = plate_height * 0.1;
    
    for (int x = 0; x < plate_width; x++)
    {
        if (vertical_proj.at<int>(0, x) < gap_threshold)
        {
            if (gap_start == -1) gap_start = x;
        }
        else
        {
            if (gap_start != -1 && x - gap_start >= min_gap_width)
            {
                gaps.push_back((gap_start + x) / 2);
            }
            gap_start = -1;
        }
    }
    
    std::sort(gaps.begin(), gaps.end());

    std::vector<int> split_positions;
    split_positions.push_back(0);
    
    for (size_t i = 0; i < gaps.size(); i++)
    {
        if (gaps[i] > 5 && gaps[i] < plate_width - 5)
        {
            bool too_close = false;
            int min_distance = plate_width / 16;
            for (int pos : split_positions)
            {
                if (std::abs(gaps[i] - pos) < min_distance)
                {
                    too_close = true;
                    break;
                }
            }
            if (!too_close)
            {
                split_positions.push_back(gaps[i]);
            }
        }
    }
    
    split_positions.push_back(plate_width);
    std::sort(split_positions.begin(), split_positions.end());

    std::vector<CharacterInfo> temp_char_list;
    
    for (size_t i = 0; i < split_positions.size() - 1; i++)
    {
        int start_x = split_positions[i];
        int end_x = split_positions[i + 1];
        int w = end_x - start_x;
        
        if (w > 5 && w < plate_width / 2)
        {
            cv::Rect char_rect(start_x, 0, w, plate_height);
            temp_char_list.emplace_back(binary(char_rect), start_x);
            cv::rectangle(debug_plate_image, char_rect, cv::Scalar(255, 0, 0), 1);
        }
    }
    
    if (temp_char_list.size() < 2)
    {
        const int expected_chars = 8;
        double province_ratio = 1.2;
        int province_width = static_cast<int>(plate_width * province_ratio / (province_ratio + expected_chars - 1));
        int char_width = (plate_width - province_width) / (expected_chars - 1);
        
        temp_char_list.clear();
        int x = 0;
        cv::Rect char_rect(x, 0, province_width, plate_height);
        temp_char_list.emplace_back(binary(char_rect), x);
        cv::rectangle(debug_plate_image, char_rect, cv::Scalar(255, 0, 0), 1);
        
        x += province_width;
        for (int i = 1; i < expected_chars; ++i)
        {
            int w = char_width;
            if (i == expected_chars - 1) w = plate_width - x;
            if (w > 2)
            {
                cv::Rect char_rect(x, 0, w, plate_height);
                temp_char_list.emplace_back(binary(char_rect), x);
                cv::rectangle(debug_plate_image, char_rect, cv::Scalar(255, 0, 0), 1);
            }
            x += w;
        }
    }

    if (temp_char_list.size() < 2)
    {
        throw std::runtime_error("can not recognize plate characters, didn't find enough characters");
    }

    std::sort(temp_char_list.begin(), temp_char_list.end(),
              [](const CharacterInfo & a, const CharacterInfo & b)
    {
        return a.x < b.x;
    });

    character_info_list = temp_char_list;
    character_images.clear();
    for (auto& ci : character_info_list)
    {
        character_images.push_back(ci.image);
    }

    const int TEMPLATE_WIDTH = 20;
    const int TEMPLATE_HEIGHT = 32;

    for (size_t i = 0; i < character_images.size(); ++i)
    {
        auto& img = character_images[i];

        cv::Mat horizontal_proj = cv::Mat::zeros(img.rows, 1, CV_32S);
        for (int y = 0; y < img.rows; y++)
        {
            horizontal_proj.at<int>(y, 0) = cv::countNonZero(img.row(y));
        }
        
        int top = 0, bottom = img.rows - 1;
        while (top < img.rows && horizontal_proj.at<int>(top, 0) == 0) top++;
        while (bottom >= 0 && horizontal_proj.at<int>(bottom, 0) == 0) bottom--;
        
        if (top <= bottom && bottom - top >= 5)
        {
            img = img(cv::Rect(0, top, img.cols, bottom - top + 1));
        }

        cv::Mat vertical_proj = cv::Mat::zeros(1, img.cols, CV_32S);
        for (int x = 0; x < img.cols; x++)
        {
            vertical_proj.at<int>(0, x) = cv::countNonZero(img.col(x));
        }
        
        int left = 0, right = img.cols - 1;
        while (left < img.cols && vertical_proj.at<int>(0, left) == 0) left++;
        while (right >= 0 && vertical_proj.at<int>(0, right) == 0) right--;
        
        if (left <= right && right - left >= 2)
        {
            img = img(cv::Rect(left, 0, right - left + 1, img.rows));
        }

        cv::Mat resized;
        cv::resize(img, resized, cv::Size(TEMPLATE_WIDTH, TEMPLATE_HEIGHT), 0, 0, cv::INTER_CUBIC);

        cv::Mat binary;
        cv::threshold(resized, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

        int white_pixels = cv::countNonZero(binary);
        if (white_pixels > binary.total() * 0.5)
        {
            cv::bitwise_not(binary, binary);
        }

        cv::Mat cleaned;
        cv::Mat small_element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
        cv::morphologyEx(binary, cleaned, cv::MORPH_OPEN, small_element);

        img = cleaned;
        character_info_list[i].image = img;
    }
}

bool LicenceRecognition::loadTemplates()
{
    bool success = true;

    const int TEMPLATE_WIDTH = 20;
    const int TEMPLATE_HEIGHT = 32;

    auto preprocessTemplate = [&](cv::Mat & img)
    {
        cv::Mat resized;
        cv::resize(img, resized, cv::Size(TEMPLATE_WIDTH, TEMPLATE_HEIGHT), 0, 0, cv::INTER_CUBIC);

        cv::Mat binary;
        cv::threshold(resized, binary, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

        int white_pixels = cv::countNonZero(binary);
        if (white_pixels > binary.total() * 0.5)
        {
            cv::bitwise_not(binary, binary);
        }

        cv::Mat cleaned;
        cv::Mat small_element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(1, 1));
        cv::morphologyEx(binary, cleaned, cv::MORPH_OPEN, small_element);

        return cleaned;
    };

    for (int i = 0; i < 10; ++i)
    {
        std::string filename = LicenceRecognitionUtil::getNumsPath() + "140_" + std::to_string(i) + ".jpg";
        cv::Mat img = loadQrcImage(QString::fromStdString(filename));
        if (!img.empty())
        {
            img = preprocessTemplate(img);
            template_images[std::to_string(i)] = img;
        }
        else
        {
            std::cerr << "Failed to load template: " << filename << std::endl;
            success = false;
        }
    }

    for (char c = 'A'; c <= 'Z'; ++c)
    {
        if (c == 'I' || c == 'O') continue;
        std::string filename = LicenceRecognitionUtil::getAlphabetPath() + "140_" + std::string(1, c) + ".jpg";
        cv::Mat img = loadQrcImage(QString::fromStdString(filename));
        if (!img.empty())
        {
            img = preprocessTemplate(img);
            template_images[std::string(1, c)] = img;
            alphabet_images[std::string(1, c)] = img;
        }
        else
        {
            std::cerr << "Failed to load template: " << filename << std::endl;
            success = false;
        }
    }

    for (const auto& p : province_map)
    {
        std::string pinyin = p.first;
        std::string short_name = p.second;
        QString q_filename = QString::fromStdString(LicenceRecognitionUtil::getProvincePath()) 
                            + "140_" + QString::fromStdString(short_name) + ".jpg";
        cv::Mat img = loadQrcImage(q_filename);
        if (!img.empty())
        {
            img = preprocessTemplate(img);
            province_images[pinyin] = img;
        }
        else
        {
            std::cerr << "Failed to load template: " << q_filename.toStdString() << std::endl;
            success = false;
        }
    }

    int number_count = 0;
    for (int i = 0; i < 10; ++i)
    {
        if (template_images.find(std::to_string(i)) != template_images.end())
            number_count++;
    }

    if (number_count < 10 || alphabet_images.size() < 24 || province_images.size() < 31)
    {
        std::cerr << "Not all templates loaded properly!" << std::endl;
        std::cerr << "Numbers: " << number_count << "/10" << std::endl;
        std::cerr << "Alphabets: " << alphabet_images.size() << "/24" << std::endl;
        std::cerr << "Provinces: " << province_images.size() << "/31" << std::endl;
        return false;
    }

    return success;
}

double LicenceRecognition::calculateShapeSimilarity(const cv::Mat& img1, const cv::Mat& img2)
{
    cv::Mat canny1, canny2;
    cv::Canny(img1, canny1, 50, 150);
    cv::Canny(img2, canny2, 50, 150);

    std::vector<std::vector<cv::Point>> contours1, contours2;
    cv::findContours(canny1, contours1, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    cv::findContours(canny2, contours2, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    cv::Mat hu1 = cv::Mat::zeros(7, 1, CV_64F);
    cv::Mat hu2 = cv::Mat::zeros(7, 1, CV_64F);

    if (!contours1.empty() && contours1[0].size() >= 4)
    {
        cv::Moments m1 = cv::moments(contours1[0]);
        if (m1.m00 > 0)
        {
            cv::HuMoments(m1, hu1);
        }
    }

    if (!contours2.empty() && contours2[0].size() >= 4)
    {
        cv::Moments m2 = cv::moments(contours2[0]);
        if (m2.m00 > 0)
        {
            cv::HuMoments(m2, hu2);
        }
    }

    for (int i = 0; i < 7; i++)
    {
        double val1 = hu1.at<double>(i);
        double val2 = hu2.at<double>(i);
        if (fabs(val1) > 1e-20)
        {
            hu1.at<double>(i) = -1 * copysign(1.0, val1) * log10(fabs(val1));
        }
        if (fabs(val2) > 1e-20)
        {
            hu2.at<double>(i) = -1 * copysign(1.0, val2) * log10(fabs(val2));
        }
    }

    double dist = cv::norm(hu1, hu2);
    if (std::isnan(dist) || std::isinf(dist))
    {
        return 0.0;
    }
    return 1.0 / (1.0 + dist);
}

void LicenceRecognition::recognizeProvince(std::string& input)
{
    cv::Mat first_char_image = character_images[0];

    
    

    double max_score = -1;
    std::string matched_province;

    for (const auto& province_img : province_images)
    {
        cv::Mat result;

        cv::matchTemplate(first_char_image, province_img.second, result, cv::TM_CCOEFF_NORMED);
        double minVal1, maxVal1;
        cv::minMaxLoc(result, &minVal1, &maxVal1);

        cv::matchTemplate(first_char_image, province_img.second, result, cv::TM_SQDIFF_NORMED);
        double minVal2;
        cv::minMaxLoc(result, &minVal2, nullptr);
        double diff_score = 1.0 - minVal2;

        cv::matchTemplate(first_char_image, province_img.second, result, cv::TM_CCORR_NORMED);
        double maxVal3;
        cv::minMaxLoc(result, nullptr, &maxVal3);

        double shape_score = calculateShapeSimilarity(first_char_image, province_img.second);

        double combined_score = maxVal1 * 0.35 + diff_score * 0.25 + maxVal3 * 0.2 + shape_score * 0.2;

        

        if (combined_score > max_score)
        {
            max_score = combined_score;
            matched_province = province_img.first;
        }
    }

    
    

    input = matched_province;
    province_images.clear();
}

void LicenceRecognition::recognizeAlphabet(std::string& input)
{
    cv::Mat second_char_image = character_images[1];

    

    double max_score = -1;
    std::string matched_alphabet;
    for (const auto& alphabet_img : alphabet_images)
    {
        cv::Mat result;

        cv::matchTemplate(second_char_image, alphabet_img.second, result, cv::TM_CCOEFF_NORMED);
        double minVal1, maxVal1;
        cv::minMaxLoc(result, &minVal1, &maxVal1);

        cv::matchTemplate(second_char_image, alphabet_img.second, result, cv::TM_SQDIFF_NORMED);
        double minVal2;
        cv::minMaxLoc(result, &minVal2, nullptr);
        double diff_score = 1.0 - minVal2;

        cv::matchTemplate(second_char_image, alphabet_img.second, result, cv::TM_CCORR_NORMED);
        double maxVal3;
        cv::minMaxLoc(result, nullptr, &maxVal3);

        double shape_score = calculateShapeSimilarity(second_char_image, alphabet_img.second);

        double combined_score = maxVal1 * 0.35 + diff_score * 0.25 + maxVal3 * 0.2 + shape_score * 0.2;

        if (combined_score > max_score)
        {
            max_score = combined_score;
            matched_alphabet = alphabet_img.first;
        }
    }

    

    input = matched_alphabet;
    alphabet_images.clear();
}

void LicenceRecognition::recognizeCharacters(cv::Mat& input, std::string& output)
{
    double max_score = -1;
    std::string matched_char;

    for (const auto& template_img : template_images)
    {
        cv::Mat result;

        cv::matchTemplate(input, template_img.second, result, cv::TM_CCOEFF_NORMED);
        double minVal1, maxVal1;
        cv::minMaxLoc(result, &minVal1, &maxVal1);

        cv::matchTemplate(input, template_img.second, result, cv::TM_SQDIFF_NORMED);
        double minVal2, maxVal2;
        cv::minMaxLoc(result, &minVal2, &maxVal2);
        double diff_score = 1.0 - minVal2;

        cv::matchTemplate(input, template_img.second, result, cv::TM_CCORR_NORMED);
        double minVal3, maxVal3;
        cv::minMaxLoc(result, &minVal3, &maxVal3);

        double shape_score = calculateShapeSimilarity(input, template_img.second);

        double combined_score = maxVal1 * 0.35 + diff_score * 0.25 + maxVal3 * 0.2 + shape_score * 0.2;

        if (combined_score > max_score)
        {
            max_score = combined_score;
            matched_char = template_img.first;
        }
    }
    
    output = matched_char;
}

void LicenceRecognition::recognizeCharactersByFeature(cv::Mat& input, std::string& output)
{
    cv::Ptr<cv::SIFT> detector = cv::SIFT::create();

    std::vector<cv::KeyPoint> keypoints_input;
    cv::Mat descriptors_input;
    detector->detectAndCompute(input, cv::noArray(), keypoints_input, descriptors_input);

    double max_score = 0.0;
    std::string matched_char;

    for (const auto& template_img : template_images)
    {
        std::vector<cv::KeyPoint> keypoints_template;
        cv::Mat descriptors_template;
        detector->detectAndCompute(template_img.second, cv::noArray(), keypoints_template, descriptors_template);

        cv::FlannBasedMatcher matcher;
        std::vector<std::vector<cv::DMatch>> knn_matches;
        matcher.knnMatch(descriptors_template, descriptors_input, knn_matches, 2);

        double score = 0.0;
        for (size_t i = 0; i < knn_matches.size(); ++i)
        {
            if (knn_matches[i][0].distance < 0.8 * knn_matches[i][1].distance)
            {
                score += 1.0;
            }
        }

        if (score > max_score)
        {
            max_score = score;
            matched_char = template_img.first;
        }
    }
    output = matched_char;
}

void LicenceRecognition::printResult()
{
    
}

const cv::Mat& LicenceRecognition::getCorrectedPlateImage() const
{
    return corrected_plate_image;
}

const cv::Mat& LicenceRecognition::getDebugPlateImage() const
{
    return debug_plate_image;
}

const std::vector<cv::Mat>& LicenceRecognition::getCharacterImages() const
{
    return character_images;
}

const cv::Mat& LicenceRecognition::getGrayImage() const
{
    return gray_image;
}

const cv::Mat& LicenceRecognition::getGaussianBlurImage() const
{
    return gaussian_blur_image;
}

const cv::Mat& LicenceRecognition::getSobelImage() const
{
    return sobel_image;
}

const cv::Mat& LicenceRecognition::getBinaryImage() const
{
    return binary_image;
}

const cv::Mat& LicenceRecognition::getCloseImage() const
{
    return close_image;
}

const cv::Mat& LicenceRecognition::getMarkedImage() const
{
    return marked_image;
}

const cv::Mat& LicenceRecognition::getPlateImage() const
{
    return plate_image;
}

const std::string& LicenceRecognition::getPlate() const
{
    return plate;
}
