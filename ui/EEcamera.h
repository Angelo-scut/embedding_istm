#ifndef EECAMERA_H
#define EECAMERA_H

#include <QtWidgets/QWidget>
#include <qfiledialog.h>
#include <qeventloop.h>
#include <qfile.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qmessagebox.h>
#include <QtNetwork/qtcpsocket.h>
#include "QMouseEvent"
#include <string>
#include <cstring>
#include <qstring.h>
#include <qtimer.h>
#include <qmessagebox.h>
#include "ui_EEcamera.h"
#include <opencv.hpp>
#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <fstream>
#include <iomanip>
#include <memory>
#include <future>
#include <condition_variable>

#include <app-istm.hpp>

#define POSITIVE true
#define NEGATIVE false
using namespace std;
int align_32(int value);
class EEcamera : public QWidget
{
	Q_OBJECT

public:
	//function
    EEcamera(QWidget *parent = Q_NULLPTR);
    void cameraOpenEvent();
    QImage cvMat2QImage(const cv::Mat& mat);
    void cameraCloseEvent();
    void imageSaveEvent();
    void imageEndsaveEvent();
    void setROIEvent();
    void delROIEvent();
    void ShowErrorMsg(QString Message, int nErrorNum=0);
    void CaptureOnceEvent();

	void modelWidgetState(bool state);
    void keyholeEvent();
	void calibrateEvent();
    void calibrateButtonEvent();
    void netRoiEvent();
    void modelLoadEvent();
	void addClickEvent();
	void undoClickEvent();
	void clearClickEvent();
	void axisChangeEvent(QString);
	void pTraceChangeEvent(QString);
	void preMaskTypeChangeEvent(QString);

	bool get_camera_state() const { return is_camera_open_; }

    //variable
    QString camera_name;
    QString savePath;
    cv::Mat myImage;
    cv::VideoCapture camera;
    cv::Mat cam;
    //QString savePath;
    bool isSaved;
    bool isRecord = false;
    
    bool autoonce = false; //控制序列采集时，第一次先进行一次自动曝光
    bool roi;
    
    bool isMonitor = false; //是否监控机器人的标志位
    bool isCamera = false;
    std::vector<float> timeSeqvector;
    cv::Rect imgROI;
    bool isSequence;
    int device_num;
    
    uint img_count = 10000;
    bool endsave = false;
    bool isShow = false;
    bool isCapture = false;
    bool isCopy = false;
    bool isCapture_once = false;
    int num_capture = 0;
    
signals:
	void image_show_signal();

public slots:
	void image_show_slot();

protected:
    void mousePressEvent(QMouseEvent *e);//鼠标点击事件
private:
    Ui::EEcamera ui;

	int cx, cy, w, h;
	QImage qmat;
	cv::Mat mat_for_show;
	shared_ptr<std::thread> capture_worker_;
	shared_ptr<std::thread> stack_worker_;
	shared_ptr<std::thread> save_worker_;
	shared_ptr<std::thread> show_worker_;
	atomic<bool> is_camera_open_;//控制相机线程的flag
	void capture_t();
	void save_t();
	void stack_t();
	void show_t();
	void time_save_event();
	std::mutex image_lock_;
	std::mutex image_seq_lock_;
	std::queue<cv::Mat> img_seq;
	float time_int = 0.0f;
	std::queue<float> time_seq;
	bool get_show_mat();
	bool get_stack_mat();
	//bool get_save_seq();
	condition_variable show_cond_;
	condition_variable stack_cond_;
	condition_variable save_cond_;
	shared_ptr<promise<bool>> show_pro_ = make_shared<promise<bool>>();
	shared_ptr<promise<bool>> stack_pro_ = make_shared<promise<bool>>();
	shared_ptr<promise<bool>> save_pro_ = make_shared<promise<bool>>();
	void release_capture();
	void release_show();
	void release_save();
	void release_stack();
	void save_one_image();

	bool isKeyhole = false;
	bool isModelroi = false;
	bool isCalibrate = false;
	QPoint firstClick;
	int roi_click_cout = 0;
	bool isAddClick = false;
	std::shared_ptr<istm::Istm> model = make_shared< istm::Istm>();
    bool is_model_load_ = false;
	bool checkPointOutofWindow(QPoint& pos);
};


#endif // EECAMERA_H
