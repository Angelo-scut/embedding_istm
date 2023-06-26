#ifndef MONITORSYSTEM_H
#define MONITORSYSTEM_H

// Qt
#include <QMainWindow>
#include <QWidget>
#include "ui_MonitorSystem.h"
#include <qmessagebox.h>
#include <qfiledialog.h>
#include <qtimer.h>

// OpenCV
#include "opencv.hpp"

// STL
#include "thread"
#include <sstream>
#include <time.h>
#include <mutex>

// Self define
#include "status_Label.h"
#include "segmentationNetwork.h"
#include "modbusthread.h"

using namespace cv;
using namespace std;

enum Direction
{
    DIR_INNER = 0,
    DIR_OUTER,
    DIR_NONE
};

class MonitorSystem : public QMainWindow
{
    Q_OBJECT

public:
    MonitorSystem(QWidget *parent = nullptr);
    ~MonitorSystem();
//    void closeEvent(QCloseEvent *event);
    typedef chrono::milliseconds MS;
    typedef chrono::high_resolution_clock Clock;
    void widgetInit();
    void cameraOpenEvent();
    void captureEvent();
    void imageshowEvent(Mat& img);
    Mat cam;
    bool isshowing = false;
    bool isdatashowing = false;
    QImage cvMat2QImage(const cv::Mat& mat);
    cv::Rect QRect2Rect(const QRect qrect);
    QRect Rect2QRect(const Rect rect);
    bool roiCheck(Rect& rect, Mat& img);
    void cameraCloseEvent();
    void imageSaveEvent();
    void imageEndsaveEvent();
    void calibbuttonEvent();

    void calibcaptureonceEvent();
    void calibEvent();
    bool homomatrixCalc();
    int distance(Point2f a, Point2f b);
    bool matRead_Save(string filename, Mat data, int mode);
    string formatDoubleValue(double val, int fixed);
    Mat vectorpoint2mat(vector<Point2f>& points);
    void mat2vectorpoint(Mat& data, vector<Point2f>& points);
//    inline bool ifFileExists (const std::string& name);
    bool matSave(string filename, Mat data);
    bool matRead(string filename, Mat& data);
    void keyholeMonitorEvent();


    void setROIEvent();
    void FinsetROIEvent();
    QRect roiRect;
    cv::Rect roirect;
    cv::Rect const defaultroi = cv::Rect(300, 0, 700, 1080);
    void roisetEnable(bool flag);
    void pointsetEnable(bool flag);
    bool isCameraopen=false;
    cv::VideoCapture Camera;
    bool isSave = false;
    QString savePath;
    String root_path = "/home/firefly/Desktop/K-TIG_Welding_Monitor_System/config/";
    QString imgs_path = "/home/firefly/Desktop/K-TIG_Welding_Monitor_System/config/imgs";
    int chessLength;  // mm
    int chessNumWidth;
    int chessNumLength;
    bool isCalib=false;
    bool isCalibonce = false;
    vector<Point2f> img_points_buf;
    vector<vector<Point2f>> img_points_seq;
    vector<vector<Point2f>> img_points_buf_tmp;
    Size board_size;
    void calibrasetEnable(bool flag);
    Point2i principlePoint;
    bool isPrincipleget = false;
    Mat cameraMatrix = Mat(3, 3, CV_32FC1, Scalar::all(0));
    Mat distCoeffs = Mat(1, 5, CV_32FC1, Scalar::all(0));
    Mat homographyMatrix;
    Mat pixelLength = Mat(1, 2, CV_32FC1, Scalar::all(0));
    vector<Point2f> networkPoints;

    status_Lable* welder_state;
    status_Lable* welding_state;
    status_Lable* camera_state;
    status_Lable* welding_current;
    status_Lable* welding_speed;
    status_Lable* wire_speed;

//    segmentation seg_network=segmentation();
    bool isSeg = false;
    bool isInit = false;
    bool isROIsetting = false;
    bool isROI = false;
    float scale_x;
    float scale_y;
    vector<float> keyhole_feature;
    float seam_width = 0.0;
    float deviation = 0.0;
//    Mat myImg;
    ModbusThread *mdThread;
    void mdsendFeature();
    QTimer *mbTimer;
    void timeoutEvent();
    bool isWelderOpen = false;

    void mbclosecameraEvent();
    void mbstartdetectEvent();
    void mbstopdetectEvent();

    bool isSettingKeyholePoint = false, isSettingSeamPoint = false, deletePoint = false;
    vector<Point> attentionPoint;
    void setKeyholePointEvent();
    void setSeamPointEvent();
    void deletePointEvent();
    void finishSetPointEvent();
    Direction region(const QPoint & point);

    bool isFlip = false;
    void flipEvent();
protected:
    void mousePressEvent(QMouseEvent *ev);
//    void mouseMoveEvent(QMouseEvent *ev);
//    void mouseReleaseEvent(QMouseEvent *ev);

signals:
    void imgshow_signal();
    void dataprocess_signal();
public slots:
    void imageShowevent();
    void dataProcessandShow();
private:
    void modelWidetState(bool state=false);
    void keyholeEvent();
    void netRoiEvent();
    void modelLoadEvent();
    void addClickEvent();
    void clearClickEvent();
    void axisChangeEvent(QString);
    void pTraceChangeEvent(QString);
    void preMaskTypeChangeEvent(QString);
    void checkPointOutofWindow(QPoint& pos);

    Ui::MonitorSystem *ui;
};

class Timeout_test{
public:
    typedef chrono::milliseconds MS;
    typedef chrono::high_resolution_clock Clock;

    Timeout_test(function<bool()> _func, int _timeout): _ptr(make_shared<task_key>()) {
        _ptr->isClose_worker = false;
        _ptr->isClose_tester = false;
        _ptr->func = _func;
        _ptr->ret = false;
        _ptr->timeout = _timeout;
    }

    bool function_wait_for() {
        std::thread worker([this](){
            auto ptr = this->_ptr;
            auto begin = Clock::now();
            bool res = ptr->func();
            lock_guard<mutex> lock(ptr->mtx);
            if (!ptr->isClose_tester) {
                ptr->ret = res;
//                printf("success detect! time: %ld ms \n", chrono::duration_cast<MS>(Clock::now() - begin).count());
            }
            ptr->isClose_worker = true;
        });
        std::thread tester([this](){
            auto ptr = this->_ptr;
            auto begin = Clock::now();
            while(!ptr->isClose_worker){
                if(chrono::duration_cast<MS>(Clock::now() - begin).count() < ptr->timeout) continue;
                lock_guard<mutex> lock(ptr->mtx);
                ptr->ret = false;
//                printf("fail detect! time: %ld ms \n", chrono::duration_cast<MS>(Clock::now() - begin).count());
                ptr->isClose_tester = true;
                break;
            }
        });
        tester.detach();
        worker.detach();
        while(!_ptr->isClose_worker && !_ptr->isClose_tester){
            this_thread::yield();
        }
        return _ptr->ret;
    }

    ~Timeout_test(){}
private:
    struct task_key{
        atomic_bool isClose_worker;
        atomic_bool isClose_tester;
        function<bool()> func;
        bool ret;
        int timeout;
        mutex mtx;
    };
    shared_ptr<task_key> _ptr;

    Timeout_test(const Timeout_test &t) = delete;
    Timeout_test(Timeout_test &&t) = delete;
    Timeout_test& operator= (const Timeout_test &t) = delete;
};

#endif // MONITORSYSTEM_H
