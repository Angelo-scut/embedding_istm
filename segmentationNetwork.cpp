#include "segmentationNetwork.h"

segmentation::segmentation(){
    homoMatrix = Mat::eye(3, 3, CV_32FC1);
    invert(homoMatrix, inverthomoMatrix);
    matRead(root_path + "horizontalH.txt", horizontalH);
    cv::invert(horizontalH, invertH);
}

void segmentation::gethomoMatrix(Mat& mat){
    homoMatrix = mat;
    invert(homoMatrix, inverthomoMatrix);
    return;
}

int segmentation::init(){
//    const char *model_path = "0921.rknn";//"segNextwtt.rknn";
//    const int img_width = 320;
//    const int img_height = 480;
    const int img_channels = 3;

    const int input_index = 0;      // node name "input"
//    const int output_index = 0;     // node name "MobilenetV1/Predictions/Reshape_1"
    // Load model
    fp = fopen(model_path, "rb");
    if(fp == NULL) {
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    int model_len = ftell(fp);
    model = malloc(model_len);
    fseek(fp, 0, SEEK_SET);
    if(model_len != fread(model, 1, model_len, fp)) {
        free(model);
        return -1;
    }

    int ret = 0;

    ret = rknn_init(&ctx, model, model_len, RKNN_FLAG_PRIOR_MEDIUM);  // RKNN_FLAG_COLLECT_PERF_MASK consume more inference time
    if(ret < 0) {
        return -1;
    }

    output0_attr.index = 0;
    ret = rknn_query(ctx, RKNN_QUERY_OUTPUT_ATTR, &output0_attr, sizeof(output0_attr)); // RKNN_QUERY_OUTPUT_ATTR query output tensor attribute
    if(ret < 0) {
        return -1;
    }
    inputs[0].index = input_index;
//    inputs[0].buf = img.data; // init when infer
    inputs[0].size = img_width * img_height * img_channels;
    inputs[0].pass_through = false;  // true->data directly deliver to knn; false->data convert to the .type and .fmt before deliver to knn
    inputs[0].type = RKNN_TENSOR_UINT8;
    inputs[0].fmt = RKNN_TENSOR_NHWC;
    return 0;
}

int segmentation::infer(Mat& input_img){
    int ret = 0;
    original_cols = input_img.cols;
    original_rows = input_img.rows;
    // Load image
    cv::Mat img;
    input_img.copyTo(img);
    Mat seg = Mat::zeros(img_height, img_width, CV_8UC3);
//    warpPerspective(input_img, img, this->homoMatrix, Size(original_cols, original_rows), cv::INTER_LINEAR);
    if(!img.data) {
        return -1;
    }
    cvtColor(img, img, CV_BGR2GRAY);
    equalizeHist(img, img);
    cvtColor(img, img, CV_GRAY2BGR);
    cvtColor(input_img, input_img, CV_BGR2GRAY);
    cvtColor(input_img, input_img, CV_GRAY2BGR);
    if(img.cols != img_width || img.rows != img_height)
        cv::resize(img, img, cv::Size(img_width, img_height), (0, 0), (0, 0), cv::INTER_LINEAR);
    inputs[0].buf = img.data;
//    cv::Mat segmask;

//    img.copyTo(seg);
    ret = rknn_inputs_set(ctx, 1, inputs);
    if(ret < 0) {
        return -1;
    }

    ret = rknn_run(ctx, nullptr);
    if(ret < 0) {
        return -1;
    }

    outputs[0].want_float = false;
    outputs[0].is_prealloc = false;
    ret = rknn_outputs_get(ctx, 1, outputs, nullptr);
    if(ret < 0) {
        return -1;
    }

    // Process output
//    output0_attr.qnt_type
    if(outputs[0].size == output0_attr.n_elems * sizeof(uint8_t))
    {
        uint8_t* prediction = (uint8_t*)outputs[0].buf;
        resultCal(prediction, seg);
        cv::resize(seg, seg, cv::Size(original_cols, original_rows), (0, 0), (0, 0), cv::INTER_NEAREST);
//        warpPerspective(seg, seg, inverthomoMatrix, Size(original_cols, original_rows), cv::INTER_NEAREST);
        addWeighted(input_img, 0.8, seg, 0.2, 0, input_img);
    }
    else
    {
        return -1;
    }
    return ret;
}

void segmentation::resultCal(uint8_t *pred, cv::Mat &segmask){
    const int rows = segmask.rows;
    const int cols = segmask.cols;
    keyhole_seg = Mat::zeros(rows, cols, CV_8UC1);
    seam_seg = Mat::zeros(rows, cols, CV_8UC1);
    const int chns = 3;
    for(uint i=0; i < rows; i++){
        for(uint j=0; j < cols; j++){
            uchar max_index = 0;
//            float max_value = pred[chns*cols*i+chns*j]; // unkonw pointer
            uint8_t max_value = pred[cols*i+j];
            for(uchar k=1; k<chns;k++){
//                if(pred[chns*cols*i+chns*j+k] > max_value){
//                    max_value = pred[chns*cols*i+chns*j+k];
                if(pred[k*cols*rows+cols*i+j] > max_value){
                    max_value = pred[k*cols*rows+cols*i+j];
                    max_index = k;
                }
            }
            switch (max_index) {
            case 3:
//                segmask.ptr<uchar>(i,j)[0] = 0.5*segmask.ptr<uchar>(i,j)[0] + 128;
//                keyhole_seg.ptr<uchar>(i)[j] = 255;
                break;
            case 1:
//                segmask.ptr<uchar>(i,j)[1] = 0.5*segmask.ptr<uchar>(i,j)[1] + 128;
                keyhole_seg.ptr<uchar>(i)[j] = 255;
                break;
            case 2:
//                segmask.ptr<uchar>(i,j)[2] = 0.5*segmask.ptr<uchar>(i,j)[2] + 128;
                seam_seg.ptr<uchar>(i)[j] = 255;
                break;
            default:
                break;
            }
        }
    }
    int ret = keyholeExtration2(keyhole_seg, segmask);
    if(ret>0){
        circle(segmask, center, 5, Scalar(0, 0, 255), -1);
        seamTracking2(seam_seg, segmask);
        resize(segmask, segmask, cv::Size(original_cols, original_rows), (0, 0), (0, 0), cv::INTER_NEAREST);
//        line(segmask, seam_point1, seam_point2, Scalar(255, 255, 255), 3);
    }
    else
        resize(segmask, segmask, cv::Size(original_cols, original_rows), (0, 0), (0, 0), cv::INTER_NEAREST);
}

int segmentation::keyholeExtration(Mat& keyhole_img){
    vector<vector<Point>> contours;
//    warpPerspective(keyhole_img, keyhole_img, horizontalH, Size(keyhole_img.cols, keyhole_img.rows));
    resize(keyhole_img, keyhole_img, cv::Size(original_cols, original_rows), (0, 0), (0, 0), cv::INTER_NEAREST);
    warpPerspective(keyhole_img, keyhole_img, this->horizontalH, Size(keyhole_img.cols, keyhole_img.rows), cv::INTER_NEAREST);
    findContours(keyhole_img, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
    if(keyhole_feature.size() != 3){
        keyhole_feature = vector<float>(3, 0.0);
    }
    if(!contours.empty()){
        float maxvalue = 0.0;
        uint index = 0;
        for(int i=0; i<contours.size(); i++){
            float temp = contourArea(contours[i]);
            if(temp > maxvalue){
                maxvalue = temp;
                index = i;
            }
        }
        std::vector<int> x_list, y_list;
        for(int i=0; i<contours[index].size(); i++){
            x_list.push_back(contours[index][i].x);
            y_list.push_back(contours[index][i].y);
        }
        keyhole_feature[2] = float(std::abs(*std::max_element(x_list.begin(), x_list.end()) - \
                                            *std::min_element(x_list.begin(), x_list.end())));
        keyhole_feature[1] = float(std::abs(*std::max_element(y_list.begin(), y_list.end()) - \
                                            *std::min_element(y_list.begin(), y_list.end())));
        keyhole_feature[0] = maxvalue;

        // 计算锁孔中心位置，长度方向不那么理想
        cv::Moments mu;
        mu = moments(contours[index], false);
        Mat temp = Mat(3, 1, CV_32FC1, Scalar::all(1));
        temp.at<float>(0, 0) = mu.m10 / mu.m00; temp.at<float>(1, 0) = mu.m01 / mu.m00;
        temp = this->invertH * temp;
        center = Point2f(temp.at<float>(0, 0) / temp.at<float>(2, 0), temp.at<float>(1, 0) / temp.at<float>(2, 0));
        return 1;
    }
    else
        return -1;
}

int segmentation::seamTracking(Mat& seam_img){
    vector<vector<Point>> contours;
    resize(seam_img, seam_img, cv::Size(original_cols, original_rows), (0, 0), (0, 0), cv::INTER_NEAREST);
    warpPerspective(seam_img, seam_img, horizontalH, Size(seam_img.cols, seam_img.rows));
    findContours(seam_img, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
    if(keyhole_feature.size() != 3){
        keyhole_feature = vector<float>(3, 0.0);
    }
    if(!contours.empty()){
        float maxvalue = 0.0;
        uint index = 0;
        for(int i=0; i<contours.size(); i++){
            float temp = contourArea(contours[i]);
            if(temp > maxvalue){
                maxvalue = temp;
                index = i;
            }
        }
        std::vector<int> x_list;
        for(int i=0; i<contours[index].size(); i++){
            x_list.push_back(contours[index][i].x);
        }
        int sum = std::accumulate(x_list.begin(), x_list.end(), 0.0);
        float mean = float(sum) / x_list.size();
        width = float(std::abs(*std::max_element(x_list.begin(), x_list.end()) - \
                                            *std::min_element(x_list.begin(), x_list.end())));
        Mat temp = Mat(3, 1, CV_32FC1, Scalar::all(1));
        temp.at<float>(0, 0) = mean; temp.at<float>(1, 0) = 0;
        temp = this->invertH * temp;
        seam_point1 = Point2i(temp.at<float>(0, 0) / temp.at<float>(2, 0), temp.at<float>(1, 0) / temp.at<float>(2, 0));
        temp.at<float>(0, 0) = mean; temp.at<float>(1, 0) = original_rows-1; temp.at<float>(2, 0) = 1;
        temp = this->invertH * temp;
        seam_point2 = Point2i(temp.at<float>(0, 0) / temp.at<float>(2, 0), temp.at<float>(1, 0) / temp.at<float>(2, 0));
        deviation = center.x - mean;
    }
    return 1;
}

int segmentation::keyholeExtration2(Mat& keyhole_img, cv::Mat &segmask){
    vector<vector<Point>> contours;
//    resize(keyhole_img, keyhole_img, cv::Size(original_cols, original_rows), (0, 0), (0, 0), cv::INTER_NEAREST);
    findContours(keyhole_img, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
    if(keyhole_feature.size() != 3){
        keyhole_feature = vector<float>(3, 0.0);
    }
    if(!contours.empty()){
        float maxvalue = 0.0;
        uint index = 0;
        for(int i=0; i<contours.size(); i++){
            float temp = contourArea(contours[i]);
            if(temp > maxvalue){
                maxvalue = temp;
                index = i;
            }
        }
        // 计算锁孔中心位置，长度方向不那么理想
        cv::Moments mu;
        mu = moments(contours[index], false);
        center.x = mu.m10 / mu.m00; center.y = mu.m01 / mu.m00;
        int x_dist=0, y_dist=0;
        if(!keyhole_intervative_point.empty()){
            x_dist = keyhole_intervative_point[0].x - center.x;
            y_dist = keyhole_intervative_point[0].y - center.y;
        }
        if(keyhole_intervative_point.size()>0 && (std::abs(x_dist) > 20 || abs(y_dist) > 20)){
            for(auto &p:contours[index]){
                p.x += x_dist;
                p.y += y_dist;
            }
            center.x = keyhole_intervative_point[0].x;
            center.y = keyhole_intervative_point[0].y;
        }
        vector<vector<Point>> temp_contours;
        temp_contours.push_back(contours[index]);
        drawContours(segmask, temp_contours, -1, Scalar(0, 128, 0), -1);
        std::vector<int> x_list, y_list;
        for(int i=0; i<contours[index].size(); i++){
            x_list.push_back(contours[index][i].x);
            y_list.push_back(contours[index][i].y);
        }
        keyhole_feature[2] = float(std::abs(*std::max_element(x_list.begin(), x_list.end()) - \
                                            *std::min_element(x_list.begin(), x_list.end()))) * \
                                            original_cols / keyhole_img.cols;
        keyhole_feature[1] = float(std::abs(*std::max_element(y_list.begin(), y_list.end()) - \
                                            *std::min_element(y_list.begin(), y_list.end()))) * \
                                                original_rows / keyhole_img.rows;
        keyhole_feature[0] = maxvalue * (original_cols * original_rows) /  (keyhole_img.rows * keyhole_img.cols);
        return 1;
    }
    else
        return -1;
}

int segmentation::seamTracking2(Mat& seam_img, cv::Mat &segmask){
    vector<vector<Point>> contours;
//    resize(seam_img, seam_img, cv::Size(original_cols, original_rows), (0, 0), (0, 0), cv::INTER_NEAREST);
    findContours(seam_img, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);
    if(keyhole_feature.size() != 3){
        keyhole_feature = vector<float>(3, 0.0);
    }
    if(!contours.empty()){
        float maxvalue = 0.0;
        uint index = 0;
        for(int i=0; i<contours.size(); i++){
            float temp = contourArea(contours[i]);
            if(temp > maxvalue){
                maxvalue = temp;
                index = i;
            }
        }
        cv::Moments mu;
        mu = moments(contours[index], false);
        Point2f centroid;
        centroid.x = mu.m10 / mu.m00; centroid.y = mu.m01 / mu.m00;
        vector<vector<Point>> squares(1, vector<Point>(4, Point()));
        RotatedRect ret;
//        float isInside = ;
        if(this->seam_intervative_point.size() > 3){
//        if(this->seam_intervative_point.size() > 3 &&  pointPolygonTest(seam_intervative_point, centroid, false) < 0){
            vector<Point> temp;
            interative_info_rand(seam_intervative_point, temp);
//            for(auto &p:seam_intervative_point){
//                circle(segmask, p, 5, Scalar(255, 0, 0), -1);
//            }
            ret = minAreaRect(temp);
        }
        else{
            ret = minAreaRect(contours[index]);
        }
        cv::Point2f vtx[4];
        ret.points(vtx);
        for(auto i=0; i<4; i++)
            squares[0][i] = Point(vtx[i].x, vtx[i].y);
        drawContours(segmask, squares, -1, Scalar(0, 0, 128), -1);
        int x=squares[0][0].x, y = squares[0][3].y, x_index = 0, y_index = 3;
        for(auto i=0; i<squares[0].size(); i++){
            Point p = squares[0][i];
            if(p.x < x){
                x = p.x;
                x_index = i;
            }
        }
        for(auto i=0; i<squares[0].size(); i++){
            Point p = squares[0][i];
            if(p.y > y && i != x_index){
                y = p.y;
                y_index = i;
            }
        }
        int x_index2 = -1, y_index2 = -1;
        for(auto i=0; i<squares[0].size(); i++){
            if(i != x_index && i != y_index){
                if(x_index2 > 0){
                    y_index2 = i;
                }
                else{
                    x_index2 = i;
                }
            }
        }
        seam_point1 = Point2i((squares[0][x_index].x + squares[0][y_index].x) / 2,
                (squares[0][x_index].y + squares[0][y_index].y) / 2);
        seam_point2 = Point2i((squares[0][x_index2].x + squares[0][y_index2].x) / 2,
                (squares[0][x_index2].y + squares[0][y_index2].y) / 2);
        cv::line(segmask, seam_point1, seam_point2, Scalar(255, 255, 255), 2);
        float k1;
        if(seam_point2.x != seam_point1.x)
            k1 = float(seam_point2.y - seam_point1.y) / (seam_point2.x - seam_point1.x);
        else
            k1 = float(seam_point2.y - seam_point1.y);
        float k2 = -1 / k1;
        float b1 = float(seam_point1.y) - k1*seam_point1.x,
                b2 = float(center.y) - k2*center.x;
        float x2 = (b2 - b1) / (k1 - k2);
        float y2 = k1*x2 +b1;
        cross_point = Point2i(int(x2), int(y2));
        cv::line(segmask, seam_point1, cross_point, Scalar(255, 255, 255), 2);
        cv::line(segmask, center, cross_point, Scalar(255, 255, 255), 2);
        width = std::sqrt(std::pow((squares[0][x_index].x - squares[0][y_index].x), 2) +
                pow((squares[0][x_index].y - squares[0][y_index].y), 2)) * \
                original_cols / seam_img.cols;
        deviation = std::sqrt(std::pow((center.x - cross_point.x), 2) +
                pow((center.y - cross_point.y), 2)) * \
                original_cols / seam_img.cols;
//        circle(segmask, Point(int(centroid.x), int(centroid.y)), 5, Scalar(255, 255, 255), -1);
    }
    return 1;
}

bool segmentation::matRead(string filename, Mat& data){
    ifstream file(filename);
    if(!file.fail()){
        string line;
        vector<float> contents;
        vector<vector<float>> data_temp;
        while (getline(file, line))
        {
            string content;
            contents.clear();
            istringstream readstr(line);
            while (getline(readstr, content, ' '))
            {
                contents.push_back(stof(content));
            }
            data_temp.push_back(contents);
        }
        data.create(data_temp.size(), data_temp[0].size(), CV_32FC1);
        for (auto i=0; i<data_temp.size(); i++) {
            for (auto j=0; j<data_temp[0].size(); j++) {
                data.at<float>(i, j) = data_temp[i][j];
            }
        }
        file.close();
        return true;
    }
    return false;
}

void segmentation::release(){
    if(ctx>0)    rknn_destroy(ctx);
    if(model)    free(model);
    if(fp)       fclose(fp);
    return;
}

segmentation::~segmentation(){
    release();
}

void segmentation::intervative_info_process(vector<Point>& InputPoints, vector<Point>& OutputPoints,
                                            Rect roi){
    OutputPoints.clear();
    OutputPoints.swap(InputPoints);
//    OutputPoints.assign(InputPoints.begin(), InputPoints.end());
    for(auto &i:OutputPoints){
        Point temp;
        temp.x = int((i.x - roi.x) * (float(img_width) / roi.width));
        temp.y = int((i.y - roi.y) * (float(img_height) / roi.height));
        i = temp;
    }
    return;
}

void segmentation::interative_info_rand(vector<Point>& InputPoints, vector<Point>& OutputPoints){
    srand((unsigned)time(NULL));
    OutputPoints.clear();
    for(auto &i:InputPoints){
        Point temp(i);
        int rand_num = rand() % rand_range;
        rand_num = rand_num - (rand_range / 2);
        temp.x = temp.x + rand_num;
        rand_num = rand() % rand_range;
        rand_num = rand_num - (rand_range / 2);
        temp.y = temp.y + rand_num;
        OutputPoints.emplace_back(temp);
    }
    return;
}

bool segmentation::isPointInside(Point& p, vector<Point>& contour){

}
