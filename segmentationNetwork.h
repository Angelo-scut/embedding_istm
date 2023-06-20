#ifndef SEGMENTATIONNETWORK_H
#define SEGMENTATIONNETWORK_H

// cv
#include <opencv.hpp>

// rknn
#include "rknn_api.h"

//STL
#include "thread"
#include "numeric"
#include "math.h"
#include "cstdlib"
#include "time.h"

using namespace cv;
using namespace std;
class segmentation{
public:
    segmentation();
    ~segmentation();
    int init();
    int infer(Mat& input_img);
    void release();
    void resultCal(uint8_t *pred, cv::Mat &segmask);
    int keyholeExtration(Mat& keyhole_img);
    int seamTracking(Mat& seam_img);
    int keyholeExtration2(Mat& keyhole_img, cv::Mat &segmask);
    int seamTracking2(Mat& seam_img, cv::Mat &segmask);
    bool matRead(string filename, Mat& data);
    void gethomoMatrix(Mat& mat);
    Mat keyhole_seg;
    Mat seam_seg;
    rknn_input inputs[1];
    rknn_output outputs[1];
    rknn_tensor_attr output0_attr;
    const char *model_path = "/home/firefly/Desktop/K-TIG_Welding_Monitor_System/config/model.rknn";
    String root_path = "/home/firefly/Desktop/K-TIG_Welding_Monitor_System/config/";
    rknn_context ctx;
    void *model;
    int model_len;
    FILE *fp;
    float width;
    float deviation;
    Point2f center;
    Point2i seam_point1;
    Point2i seam_point2;
    Point2i cross_point;
    vector<float> keyhole_feature;
    Mat horizontalH;
    Mat invertH;
    Mat homoMatrix;
    Mat inverthomoMatrix;
    int original_rows = 0;
    int original_cols = 0;
    const int img_width = 320;
    const int img_height = 480;
    vector<Point> keyhole_intervative_point;
    vector<Point> seam_intervative_point;
    uint rand_range = 7;
    void intervative_info_process(vector<Point>& InputPoints, vector<Point>& OutputPoints,
                                  Rect roi);
    void interative_info_rand(vector<Point>& InputPoints, vector<Point>& OutputPoints);
    bool isPointInside(Point& p, vector<Point>& contour);
};



#endif // SEGMENTATIONNETWORK_H
