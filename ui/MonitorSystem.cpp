#include "MonitorSystem.h"
#include "ui_MonitorSystem.h"
#include "QIcon"

MonitorSystem::MonitorSystem(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MonitorSystem)
{
    ui->setupUi(this);
//    ui->originalImage->setGeometry(9,9,950,740);
    this->setWindowTitle("K-TIG Welding Monitor System");
    this->setWindowIcon(QIcon(QString::fromStdString(root_path + "kaggle.ico")));
    roiRect = QRect(0, 0, 0, 0);
    roirect = defaultroi;
    ui->ROI_x->setValue(roirect.x);
    ui->ROI_y->setValue(roirect.y);
    ui->ROI_w->setValue(roirect.width);
    ui->ROI_h->setValue(roirect.height);
    ui->ROI_x->setSingleStep(10);
    ui->ROI_y->setSingleStep(10);
    ui->ROI_w->setSingleStep(10);
    ui->ROI_h->setSingleStep(10);
    isROI = true;
    // 按钮与信号相连
    QObject::connect(ui->cameraOpen, &QPushButton::clicked, this, &MonitorSystem::cameraOpenEvent);
    QObject::connect(ui->cameraClose, &QPushButton::clicked, this, &MonitorSystem::cameraCloseEvent);
    QObject::connect(ui->imageSave, &QPushButton::clicked, this, &MonitorSystem::imageSaveEvent);
    QObject::connect(ui->saveEnd, &QPushButton::clicked, this, &MonitorSystem::imageEndsaveEvent);
//    QObject::connect(ui->setROIButton, &QPushButton::clicked, this, &MonitorSystem::setROIEvent);
//    QObject::connect(ui->FinSetROIButton, &QPushButton::clicked, this, &MonitorSystem::FinsetROIEvent);
    QObject::connect(ui->calibButton, &QPushButton::clicked, this, &MonitorSystem::calibbuttonEvent);
    QObject::connect(ui->calibCapture, &QPushButton::clicked, this, &MonitorSystem::calibcaptureonceEvent);
    QObject::connect(ui->keyholeMonitor, &QRadioButton::clicked, this, &MonitorSystem::keyholeMonitorEvent);
    QObject::connect(this, SIGNAL(imgshow_signal()), this, SLOT(imageShowevent()));
    QObject::connect(this, SIGNAL(dataprocess_signal()), this, SLOT(dataProcessandShow()));
    QObject::connect(ui->setKeyholePoint, &QPushButton::clicked, this, &MonitorSystem::setKeyholePointEvent);
    QObject::connect(ui->setSeamPoint, &QPushButton::clicked, this, &MonitorSystem::setSeamPointEvent);
    QObject::connect(ui->deletePoint, &QPushButton::clicked, this, &MonitorSystem::deletePointEvent);
    QObject::connect(ui->finishSetPoint, &QPushButton::clicked, this, &MonitorSystem::finishSetPointEvent);
    QObject::connect(ui->flipButton, &QRadioButton::clicked, this, &MonitorSystem::flipEvent);
    // 按钮使能初始化
    widgetInit();


    // 相关参数读取
    matRead(root_path + "cameraMatrix.txt", cameraMatrix);
    matRead(root_path + "distCoeffs.txt", distCoeffs);
    {
        Mat temp;
        matRead(root_path + "networkChessPoints.txt", temp);
        mat2vectorpoint(temp, networkPoints);
    }
    {
        Mat temp;
        matRead(root_path + "chessPoints.txt", temp);
        mat2vectorpoint(temp, img_points_buf);
    }
    homographyMatrix = findHomography(img_points_buf, networkPoints);
    this->seg_network.gethomoMatrix(homographyMatrix);
    matRead(root_path + "lengthPerPixel.txt", pixelLength);
    ui->fx->setText(QString::fromStdString("fx:"+formatDoubleValue(pixelLength.at<float>(0, 0), 6)));
    ui->fy->setText(QString::fromStdString("fy:"+formatDoubleValue(pixelLength.at<float>(0, 1), 6)));
    this->chessLength = 3;
    this->chessNumLength = 4;
    this->chessNumWidth = 3;
    ui->chessLength->setValue(this->chessLength);
    ui->chessNumLength->setValue(this->chessNumLength);
    ui->chessNumWidth->setValue(this->chessNumWidth);
    ui->chessLength->setMinimum(0);
    ui->chessNumLength->setMinimum(3);
    ui->chessNumWidth->setMinimum(3);
    this->principlePoint = networkPoints[int(networkPoints.size()/2)];
    string principlePoint = "(" + to_string(this->principlePoint.x);
    principlePoint += "," + to_string(this->principlePoint.y) + ")";
    ui->priciplePoint->setText(QString::fromStdString(principlePoint));

    // 状态栏信息
    vector<string> status_info;
    status_info.push_back("Reconnecting");
    status_info.push_back("Connected");
    welder_state = new status_Lable(status_info, "Welder:", true);
    ui->statusbar->addWidget(welder_state);
    status_info.push_back("NULL");
    status_info.push_back("Under penetration");
    status_info.push_back("Partial penetration");
    status_info.push_back("Good penetration");
    status_info.push_back("Over penetration");
    welding_state = new status_Lable(status_info, "Welding Quality:", false);
    ui->statusbar->addWidget(welding_state);
    status_info.push_back("Closed");
    status_info.push_back("Opened");
    status_info.push_back("Calibrting");
    status_info.push_back("Waiting");
    camera_state = new status_Lable(status_info, "Camera State:", false);
    ui->statusbar->addWidget(camera_state);
    status_info.push_back("       A");
    welding_current = new status_Lable(status_info, "Welding current:", false);
    ui->statusbar->addWidget(welding_current);
    status_info.push_back("       mm/min");
    welding_speed = new status_Lable(status_info, "Welding speed:", false);
    ui->statusbar->addWidget(welding_speed);
    status_info.push_back("     mm/min");
    wire_speed = new status_Lable(status_info, "Wire speed:", false);
    ui->statusbar->addWidget(wire_speed);
    // 定周期尝试连接焊机通信？需要吗？打开相机？需要吗？

    this->keyhole_feature = vector<float>(3, 0.0);

    if(!this->isInit){
        int ret = seg_network.init();
        if(ret<0){
            return;
        }
        this->isInit = true;
    }

    //modbus init
    mdThread = new ModbusThread;
    mdThread->start();
    this->mbTimer = new QTimer(this);
    QObject::connect(this->mbTimer, &QTimer::timeout, this, &MonitorSystem::timeoutEvent);
    this->mbTimer->start(50);
    this->showMaximized();

    // save path
    this->savePath=  imgs_path;
    ui->savePath->setText(this->savePath);
}

bool MonitorSystem::matSave(string filename, Mat data){
    ofstream file(filename, ios::trunc);
    if(file){
        for (int i=0; i<data.rows; i++) {
            for (int j=0; j<data.cols-1; j++ ) {
                file << data.at<float>(i, j) << " ";
            }
            if(data.cols-1>= 0){
                file << data.at<float>(i, data.cols-1);
            }
            file << '\n';
        }
        file.close();
        return true;
    }
    else
        return false;
}

bool MonitorSystem::matRead(string filename, Mat& data){
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

void MonitorSystem::widgetInit(){
    ui->cameraClose->setEnabled(false);
    ui->imageSave->setEnabled(false);
    ui->saveEnd->setEnabled(false);
    ui->calibButton->setEnabled(false);
    ui->calibCapture->setEnabled(false);
    roisetEnable(false);
    pointsetEnable(false);
    calibrasetEnable(false);
}

//inline bool MonitorSystem::ifFileExists (const std::string& name){
//    struct stat buffer;
//      return (stat (name.c_str(), &buffer) == 0);
//}

void MonitorSystem::cameraOpenEvent()
{
    this->isCameraopen = true;
//    bool flag = this->Camera.open(root_path + "video.avi");
    bool flag = this->Camera.open(0);
    this->Camera.set(3, 1920);
    this->Camera.set(4, 1080);
//    qDebug() << this->Camera.get(3) << endl;
//    qDebug() << this->Camera.get(4) << endl;
    if (!flag)
    {
        if(this->mdThread->ctx != NULL){
//            this->mdThread->mb_mapping->tab_registers[10] = 1;
            this->mdThread->warning = 1;
        }
        return;
    }
//    this->mdThread->mb_mapping->tab_registers[10] = 0;
    this->mdThread->warning = 0;
//    cv::Mat cam;
//    this->Camera >> cam;
//    this->weldpoolImage = cv::imread("/home/firefly/Desktop/track_system for paper/yitong.jpg"); //测试
//    cam.copyTo(this->myImage);
//    this->myImage = cam;

    std::thread captureThread(&MonitorSystem::captureEvent, this); //相机1测试子线程
    captureThread.detach();
    ui->cameraOpen->setEnabled(false);
    ui->cameraClose->setEnabled(true);
    ui->calibButton->setEnabled(true);
    ui->imageSave->setEnabled(true);
    camera_state->updateUI(1);
    roisetEnable(true);
    ui->setKeyholePoint->setEnabled(true);
    ui->setSeamPoint->setEnabled(true);
    calibrasetEnable(true);
//    if(this->mdThread->isConnect && this->mdThread->mb_mapping->tab_registers[16]>0)
//        keyholeMonitorEvent();
//    else if(this->mdThread->ctx != NULL)
//        this->mdThread->mb_mapping->tab_registers[0] = 1;
//    this->mbTimer->start(50);
    return;
}

void MonitorSystem::captureEvent(){
//    cv::Mat cam;
//    cv::Mat seg;
    int count = 0;
    while (true)
    {
        while(this->isshowing){
            if (!this->isCameraopen)
            {
//                while(this->isshowing);
                this->Camera.release();
                if (this->isSave)
                    imageEndsaveEvent();
                break;
            }
        }
        this->Camera >> cam;
        if(this->isFlip)
            cv::flip(cam, cam, 0);
        while(this->isCalib){
            while(this->isshowing);
            if(this->isFlip)
                this->Camera >> cam;
            cv::flip(cam, cam, 0);
            if(this->isCalibonce){
                if(!cam.empty()){
                    Mat temp;
                    if(isROI){
                        cam(roirect).copyTo(temp);
                    }
                    else
                        cam.copyTo(temp);
                    camera_state->updateUI(3);
                    img_points_buf_tmp.emplace_back(vector<Point2f>());
                    Timeout_test t(std::bind(&(cv::findChessboardCorners), temp, board_size, ref(img_points_buf_tmp.back()), CALIB_CB_ADAPTIVE_THRESH + CALIB_CB_NORMALIZE_IMAGE), 10000);
                    bool ret = t.function_wait_for();
    //                bool ret = findChessboardCorners(temp, board_size, img_points_buf, CALIB_CB_ADAPTIVE_THRESH + CALIB_CB_NORMALIZE_IMAGE);
                    if(ret){
                        img_points_buf = img_points_buf_tmp.back();
                        Mat view_gray;
                        cvtColor(temp, view_gray, CV_BGR2GRAY);
                        find4QuadCornerSubpix(view_gray, img_points_buf, Size(5, 5));
                        this->img_points_seq.push_back(img_points_buf);
                        drawChessboardCorners(temp, board_size, img_points_buf, ret);
                        this->principlePoint = Point2i(int(img_points_buf[int(chessNumLength*chessNumWidth/2)].x), \
                                int(img_points_buf[int(chessNumLength*chessNumWidth/2)].y));
                        string principlePoint = "(" + to_string(this->principlePoint.x);
                        principlePoint += "," + to_string(this->principlePoint.y) + ")";
                        ui->priciplePoint->setText(QString::fromStdString(principlePoint));
                        this->isPrincipleget = true;
                        this->isCalib = false;
                        matSave(this->root_path + "chessPoints.txt", vectorpoint2mat(img_points_seq[0]));
                    //    circle(cam, Point2i(int(img_points_buf[0].x + this->roirect.x), int(img_points_buf[0].y + this->roirect.y)), 5, (255, 0, 255), -1);
                    //    imwrite(root_path + "calibate.jpg", cam);
                        bool ret2 = homomatrixCalc();
                        if(ret2)
                        {
                            camera_state->status_vec[2] = "Calibrting:done";
                            matSave(this->root_path + "homographyMatrix.txt", homographyMatrix);
                        }
                        else
                            camera_state->status_vec[2] = "Chess not match!";
                        if(isROI){
                            temp.copyTo(cam(roirect));
                        }
                        else
                            temp.copyTo(cam);
//                        imageshowEvent(cam);
                        camera_state->updateUI(2);
                        sleep(2);
                        camera_state->updateUI(1);
                    }
                    else{
                        camera_state->status_vec[2] = "Calibrting";
                        camera_state->updateUI(2);
                    }
                }
                this->isCalibonce = false;
            }
            if(!this->isCameraopen || cam.empty()){
                camera_state->updateUI(0);
                this->isCalib = false;
                ui->calibCapture->setEnabled(false);  // 子线程能操作吗？如果不能就交给定时器
                ui->calibButton->setEnabled(true);
                break;
            }
//            imageshowEvent(cam);
            this->isshowing = true;
            emit imgshow_signal();
            if (!this->isCameraopen)
            {
//                while(this->isshowing);
                this->Camera.release();
                if (this->isSave)
                    imageEndsaveEvent();
                break;
            }
        }
//        cv::rotate(cam, cam, cv::ROTATE_180);
        if(cam.empty()){
            if (!this->isCameraopen)
            {
                this->Camera.release();
                if (this->isSave)
                    imageEndsaveEvent();
                this->isCameraopen = true;
                break;
            }
            continue;
        }
        if(this->isSave){
            string filename = this->savePath.toStdString() +'/';
            filename += to_string(count) + ".jpg";
            imwrite(filename, cam);
//            count++;
//            waitKey(5);
        }
        if (!this->isCameraopen)
        {
//            while(this->isshowing);
            this->Camera.release();
            if (this->isSave)
                imageEndsaveEvent();
//            this->isCameraopen = true;
            if(this->isSeg)
                keyholeMonitorEvent();
            break;
        }
        if(this->isSeg){
            Mat seg;
            if(!isROI){
                int x,y,w,h;
//                x = principlePoint.x - 350;
//                x = x >0?  x : 0;
//                y = principlePoint.y - 150;
//                y = y>0? y: 0;
//                w = x + 700 < cam.cols? 700 : cam.cols - x -1;
//                h = x + 800 < cam.rows? 800 : cam.rows - y -1;
                x = 800; w = 700;
                y = 0; h = cam.rows;
//                Rect roi(x, y, w, h);
                roirect = Rect(x, y, w, h);
            }

//            scale_x = w / 320; scale_x /= this->pixelLength.at<float>(0, 0);
//            scale_y = h / 320; scale_y /= this->pixelLength.at<float>(0, 1);
            cam(roirect).copyTo(seg);
            if(this->isSeg)
                seg_network.infer(seg);
//            cv::resize(seg, seg, cv::Size(w, h), 0.0, 0.0, cv::INTER_LINEAR);
            seg.copyTo(cam(roirect));
//            this->dataProcessandShow();
            this->isdatashowing = true;
            emit dataprocess_signal();
            while(this->isdatashowing){
                if (!this->isCameraopen)
                {
                    this->Camera.release();
                    if (this->isSave)
                        imageEndsaveEvent();
                    break;
                }
            }
            this->mdsendFeature();
//            imageshowEvent(cam);
            this->isshowing = true;
            emit imgshow_signal();
            if(this->isSave){
                string filename = this->savePath.toStdString() +'/';
                filename += to_string(count) + "_seg.jpg";
                imwrite(filename, cam);
    //            count++;
    //            waitKey(5);
            }
//            imageshowEvent(this->seg_network.seam_seg);
        }
        else{
//            imageshowEvent(cam);
            this->isshowing = true;
            emit imgshow_signal();
            usleep(1000);
        }
        if(this->isSave){
            count++;
        }
    }
    return;
}

QImage MonitorSystem::cvMat2QImage(const cv::Mat & mat)
{
    using namespace cv;
    if (mat.type() == CV_8UC1)                  // 单通道
    {
        QImage image(mat.cols, mat.rows, QImage::Format_Indexed8);
        image.setColorCount(256);               // 灰度级数256
        for (int i = 0; i < 256; i++)
        {
            image.setColor(i, qRgb(i, i, i));
        }
        uchar *pSrc = mat.data;                 // 复制mat数据
        for (int row = 0; row < mat.rows; row++)
        {
            uchar *pDest = image.scanLine(row);
            memcpy(pDest, pSrc, mat.cols);
            pSrc += mat.step;
        }
        return image;
    }

    else if (mat.type() == CV_8UC3)             // 3通道
    {
        const uchar *pSrc = (const uchar*)mat.data;         // 复制像素
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);    // R, G, B 对应 0,1,2
        return image.rgbSwapped();              // rgbSwapped是为了显示效果色彩好一些。
    }
    else if (mat.type() == CV_8UC4)
    {
        const uchar *pSrc = (const uchar*)mat.data;         // 复制像素
                                                            // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);        // B,G,R,A 对应 0,1,2,3
        return image.copy();
    }
    else
    {
        return QImage();
    }
}

cv::Rect MonitorSystem::QRect2Rect(const QRect qrect){
    Rect temp(0, 0, 0, 0);
    temp.x = qrect.topLeft().x(); temp.y = qrect.topLeft().y();
    temp.width = qrect.width(); temp.height = qrect.height();
    return temp;
}

QRect MonitorSystem::Rect2QRect(const Rect rect){
    QRect temp(0, 0, 0, 0);
    temp.setX(rect.x); temp.setY(rect.y);
    temp.setWidth(rect.width); temp.setHeight(rect.height);
    return temp;
}

bool MonitorSystem::roiCheck(Rect& rect, Mat& img){
    if(rect.x <0) rect.x = 0;
    if(rect.y <0) rect.y = 0;
    if(rect.x >= img.cols) rect.x = img.cols - 1;
    if(rect.y >= img.rows) rect.y = img.rows - 1;
    if(rect.x+rect.width >= img.cols) rect.width = img.cols - 1 - rect.x;
    if(rect.y+rect.height >= img.rows) rect.height = img.rows - 1 - rect.y;
    if(rect.width > 0 && rect.height > 0) return true;
    else {
        rect = defaultroi;
        return true;
    }

}

void MonitorSystem::imageshowEvent(Mat& img){
    int width = img.cols, height = img.rows;
    QImage imgg;
    Mat temp;
    img.copyTo(temp);
//    std::cout << ui->originalImage->width() << ui->originalImage->height() << endl;
    if (this->isROI){
        roirect.x = ui->ROI_x->value(); roirect.y = ui->ROI_y->value();
        roirect.width = ui->ROI_w->value(); roirect.height = ui->ROI_h->value();
        roiCheck(roirect, img);
        cv::rectangle(temp, roirect, cv::Scalar(0,0,255));
//        cv::rectangle(temp, cv::Point(roiRect.topLeft().x(), roiRect.topLeft().y()), cv::Point(roiRect.topLeft().x() + roiRect.width(), roiRect.topLeft().y() + roiRect.height()), cv::Scalar(0,0,255));
    }
    cv::resize(temp, temp, cv::Size(ui->originalImage->width(), ui->originalImage->height()));
    imgg = this->cvMat2QImage(temp);
//    ui->originalImage->setScaledContents(true);
    ui->originalImage->setPixmap(QPixmap::fromImage(imgg));
    return;
}

void MonitorSystem::imageShowevent(){
    if(this->cam.empty())
        return;
//    isshowing = true;
    int width = cam.cols, height = cam.rows;
    QImage imgg;
    Mat temp;
    cam.copyTo(temp);
//    std::cout << ui->originalImage->width() << ui->originalImage->height() << endl;
    if (this->isROI){
        roirect.x = ui->ROI_x->value(); roirect.y = ui->ROI_y->value();
        roirect.width = ui->ROI_w->value(); roirect.height = ui->ROI_h->value();
        roiCheck(roirect, cam);
        cv::rectangle(temp, roirect, cv::Scalar(0,0,255));
//        cv::rectangle(temp, cv::Point(roiRect.topLeft().x(), roiRect.topLeft().y()), cv::Point(roiRect.topLeft().x() + roiRect.width(), roiRect.topLeft().y() + roiRect.height()), cv::Scalar(0,0,255));
    }
    if (!attentionPoint.empty()) {
        for (auto &i : attentionPoint) {
            cv::circle(temp, i, 5, (255,255,255), -1);
        }
    }
    cv::resize(temp, temp, cv::Size(ui->originalImage->width(), ui->originalImage->height()));
    imgg = this->cvMat2QImage(temp);
//    ui->originalImage->setScaledContents(true);
    ui->originalImage->setPixmap(QPixmap::fromImage(imgg));
    isshowing = false;
    return;
}

void MonitorSystem::cameraCloseEvent()
{
    this->isCameraopen = false;
//    while(!this->isCameraopen); // wait for the capture thread quit
//    this->isCameraopen = false;
    ui->cameraClose->setEnabled(false);
    ui->cameraOpen->setEnabled(true);
    camera_state->updateUI(0);
    roisetEnable(false);
    calibrasetEnable(false);
}

void MonitorSystem::imageSaveEvent(){
    if (this->savePath!=NULL){
        this->isSave=true;
        ui->imageSave->setEnabled(false);
        ui->saveEnd->setEnabled(true);
    }
    else{
        this->savePath = QFileDialog::getExistingDirectory(this, "save");;
        ui->savePath->setText(this->savePath);
        if (this->savePath!=NULL)
        {
    //        this->isSave = true;
    //        ui->imageSave->setEnabled(false);
//            ui->saveEnd->setEnabled(true);
        }
    }
    return;
}

void MonitorSystem::imageEndsaveEvent(){
    this->isSave = false;
    ui->imageSave->setEnabled(true);
    ui->saveEnd->setEnabled(false);
    ui->savePath->clear();
}

void MonitorSystem::roisetEnable(bool flag){
    ui->ROI_x->setEnabled(flag);
    ui->ROI_y->setEnabled(flag);
    ui->ROI_w->setEnabled(flag);
    ui->ROI_h->setEnabled(flag);
    return;
}

void MonitorSystem::pointsetEnable(bool flag){
    ui->setKeyholePoint->setEnabled(flag);
    ui->setSeamPoint->setEnabled(flag);
    ui->deletePoint->setEnabled(flag);
    ui->finishSetPoint->setEnabled(flag);
    return;
}

void MonitorSystem::calibbuttonEvent(){
    if(!this->isCameraopen){
        return;
    }
    QString temp = ui->chessLength->text();
    if(temp == NULL){
        return;
    }
    this->chessLength = temp.toInt();
    temp = ui->chessNumLength->text();
    if(temp == NULL){
        return;
    }
    this->chessNumLength = temp.toInt();
    temp = ui->chessNumWidth->text();
    if(temp == NULL){
        return;
    }
    this->chessNumWidth = temp.toInt();
//    std::thread calibThread(&MonitorSystem::calibEvent, this); //相机1测试子线程
//    calibThread.detach();
//    ui->calibButton->setEnabled(false);
    ui->calibCapture->setEnabled(true);
//    ui->chessLength->setEnabled(false);
//    ui->chessNumLength->setEnabled(false);
//    ui->chessNumWidth->setEnabled(false);
    camera_state->status_vec[2] = "Calibrting";
    camera_state->updateUI(2);
    this->img_points_buf_tmp.clear();
    this->board_size = Size(chessNumLength, chessNumWidth);
    this->isCalib = true;
//    this->mbTimer->stop();
}

void MonitorSystem::calibcaptureonceEvent(){
    this->isCalibonce = true;
}

void MonitorSystem::calibrasetEnable(bool flag){
    ui->calibButton->setEnabled(flag);
    ui->calibCapture->setEnabled(flag);
    ui->chessLength->setEnabled(flag);
    ui->chessNumLength->setEnabled(flag);
    ui->chessNumWidth->setEnabled(flag);
    return;
}

int MonitorSystem::distance(Point2f a, Point2f b){
    return abs(a.x-b.x) + abs(a.y-b.y);
}

bool MonitorSystem::matRead_Save(string filename, Mat data, int mode){
    if(mode==0){
        FileStorage fs(filename, FileStorage::WRITE);
        fs<< "data" << data;
        fs.release();
        return true;
    }
    else if (mode==1) {
        FileStorage fs(filename, FileStorage::READ);
        fs["data"] >> data;
        fs.release();
        if(data.empty()){
            return false;
        }
        return true;
    }
    return false;
}

string MonitorSystem::formatDoubleValue(double val, int fixed){
    ostringstream oss;
    oss << setprecision(fixed) << val;
    return oss.str();
}

Mat MonitorSystem::vectorpoint2mat(vector<Point2f>& points){
    Mat data;
    data.create(points.size(), 2, CV_32FC1);
    for(int i=0; i<points.size(); i++){
        data.at<float>(i, 0) = points[i].x;
        data.at<float>(i, 1) = points[i].y;
    }
    return data;
}

void MonitorSystem::mat2vectorpoint(Mat& data, vector<Point2f>& points){
    if(data.empty()){
        return;
    }
    for(int i=0; i<data.rows; i++){
        Point2f p(data.at<float>(i, 0), data.at<float>(i, 1));
        points.push_back(p);
    }
    return;
}

void MonitorSystem::calibEvent(){
//    while(!this->isPrincipleget);
//    cout << "enter calib" << endl;
    int img_num = 1;
    int count = 0;
    Mat cam;
    this->Camera >> cam;
    Size img_size = Size(cam.cols, cam.rows);
    Size board_size = Size(chessNumLength, chessNumWidth);
    vector<vector<Point2f>> img_points_buf_tmp;
    while(count<img_num){
        this->Camera >> cam;
//        auto t_start = Clock::now();
        if(this->isCalibonce){
            if(!cam.empty()){
                Mat temp;
                if(isROI){
                    cam(roirect).copyTo(temp);
                }
                else
                    cam.copyTo(temp);
//                Size board_size = Size(chessNumLength, chessNumWidth);
//                auto begin = Clock::now();
                camera_state->updateUI(3);
                img_points_buf_tmp.emplace_back(vector<Point2f>());
                Timeout_test t(std::bind(&(cv::findChessboardCorners), temp, board_size, ref(img_points_buf_tmp.back()), CALIB_CB_ADAPTIVE_THRESH + CALIB_CB_NORMALIZE_IMAGE), 10000);
                bool ret = t.function_wait_for();
//                bool ret = findChessboardCorners(temp, board_size, img_points_buf, CALIB_CB_ADAPTIVE_THRESH + CALIB_CB_NORMALIZE_IMAGE);
                if(ret){
                    img_points_buf = img_points_buf_tmp.back();
                    Mat view_gray;
                    cvtColor(temp, view_gray, CV_BGR2GRAY);
                    find4QuadCornerSubpix(view_gray, img_points_buf, Size(5, 5));
                    this->img_points_seq.push_back(img_points_buf);
                    drawChessboardCorners(temp, board_size, img_points_buf, ret);
                    imageshowEvent(cam);
                    this->principlePoint = Point2i(int(img_points_buf[int(chessNumLength*chessNumWidth/2)].x), \
                            int(img_points_buf[int(chessNumLength*chessNumWidth/2)].y));
                    string principlePoint = "(" + to_string(this->principlePoint.x);
                    principlePoint += "," + to_string(this->principlePoint.y) + ")";
                    ui->priciplePoint->setText(QString::fromStdString(principlePoint));
                    this->isPrincipleget = true;
                    count ++;
                    if(isROI){
                        temp.copyTo(cam(roirect));
                    }
                    else
                        temp.copyTo(cam);
                }
                else
                    camera_state->updateUI(2);
            }
            this->isCalibonce = false;
        }
        if(!this->isCameraopen || cam.empty()){

            camera_state->updateUI(0);
            this->isCalib = false;
            ui->calibCapture->setEnabled(false);  // 子线程能操作吗？如果不能就交给定时器
            ui->calibButton->setEnabled(true);

            return;
        }
        imageshowEvent(cam);
        sleep(1);
    }
    matSave(this->root_path + "chessPoints.txt", vectorpoint2mat(img_points_seq[0]));
//    circle(cam, Point2i(int(img_points_buf[0].x + this->roirect.x), int(img_points_buf[0].y + this->roirect.y)), 5, (255, 0, 255), -1);
//    imwrite(root_path + "calibate.jpg", cam);
    bool ret = homomatrixCalc();
    if(ret)
        camera_state->status_vec[2] = "Calibrting:done";
    else
        camera_state->status_vec[2] = "Chess not match!";
    camera_state->updateUI(2);
    this->isCalib = false;
    ui->calibCapture->setEnabled(false);  // 子线程能操作吗？如果不能就交给定时器
    ui->calibButton->setEnabled(true);
    ui->chessLength->setEnabled(true);
    ui->chessNumLength->setEnabled(true);
    ui->chessNumWidth->setEnabled(true);
    sleep(2);
    camera_state->updateUI(1);
//    waitKey(1000);
}

bool MonitorSystem::homomatrixCalc(){
    vector<Point2f> chessboard = img_points_seq[0];
    if(chessboard[0].x > chessboard.back().x){
        for(int i=0; i<chessNumWidth; i++){
            for(int j=0; j<int(chessNumLength / 2); j++){
                std::swap(chessboard[i * chessNumLength + j], \
                        chessboard[(i+1) * chessNumLength -j -1]);
            }
        }
    }
    if(chessboard[0].y > chessboard.back().y){
        for(int i=0; i<chessNumLength; i++){
            for(int j=0; j<int(chessNumWidth / 2); j++){
                std::swap(chessboard[j * chessNumLength + i], \
                        chessboard[(chessNumWidth -j -1) * chessNumLength +i]);
            }
        }
    }
    vector<Point2f> object_points;
    for(auto j=0; j<chessNumWidth; j++){
        for(auto i=0; i<chessNumLength; i++){
            Point2f temp_point;
            temp_point.x = i * chessLength;
            temp_point.y = j * chessLength;
            object_points.push_back(temp_point);
        }
    }
    homographyMatrix = findHomography(chessboard, object_points);
    homographyMatrix.convertTo(homographyMatrix, CV_32FC1);
    Mat temp = Mat(3, 1, CV_32FC1, Scalar::all(1));
    temp.at<float>(1, 0) = 0;
    temp = homographyMatrix * temp;
    pixelLength.at<float>(0, 0) = std::abs(temp.at<float>(0, 0) / temp.at<float>(2, 0));
    temp = Mat(3, 1, CV_32FC1, Scalar::all(1));
    temp.at<float>(0, 0) = 0;
    temp = homographyMatrix * temp;
    pixelLength.at<float>(0, 1) = std::abs(temp.at<float>(1, 0) / temp.at<float>(2, 0));
    ui->fx->setText(QString::fromStdString("fx:"+formatDoubleValue(pixelLength.at<float>(0, 0), 6)));
    ui->fy->setText(QString::fromStdString("fy:"+formatDoubleValue(pixelLength.at<float>(0, 1), 6)));
    matSave(this->root_path + "lengthPerPixel.txt", pixelLength);
//    this->seg_network.gethomoMatrix(homographyMatrix);
    return true;
}

void MonitorSystem::keyholeMonitorEvent(){
    if(this->isSeg){
        this->isSeg = false;
        ui->keyholeMonitor->setAutoExclusive(false);
        ui->keyholeMonitor->setChecked(false);
        ui->keyholeMonitor->setAutoExclusive(true);
        ui->cameraClose->setEnabled(true);
        roisetEnable(true);
        calibrasetEnable(true);
//        this->isInit = false;
//        seg_network.release();
    }
    else{
        if(!this->isInit){
            int ret = seg_network.init();
            if(ret<0){
                if(this->mdThread->ctx != NULL){
//                    this->mdThread->mb_mapping->tab_registers[10] = 1;
                    this->mdThread->warning = 1;
                }
                return;
            }
            this->isInit = true;
        }
        this->isSeg = true;
        ui->cameraClose->setEnabled(false);
//        if(this->mdThread->ctx != NULL){  // if the welder has connected
//            this->isWelderOpen = true;
//            this->mdThread->mb_mapping->tab_registers[0] = 1;
            ui->keyholeMonitor->setAutoExclusive(false);
            ui->keyholeMonitor->setChecked(true);
            ui->keyholeMonitor->setAutoExclusive(true);
//        }
        roisetEnable(false);
        calibrasetEnable(false);
    }
    return;
}

void MonitorSystem::dataProcessandShow(){
    this->keyhole_feature[0] = this->seg_network.keyhole_feature[0] / (this->pixelLength.at<float>(0, 0) * this->pixelLength.at<float>(0, 1));
    this->keyhole_feature[1] = this->seg_network.keyhole_feature[1] / this->pixelLength.at<float>(0, 0);
    this->keyhole_feature[2] = this->seg_network.keyhole_feature[2] / this->pixelLength.at<float>(0, 1);
    this->seam_width = this->seg_network.width / this->pixelLength.at<float>(0, 0);
    this->deviation = this->seg_network.deviation / this->pixelLength.at<float>(0, 0);
    ui->keyholeArea->setText(QString::fromStdString(formatDoubleValue(this->keyhole_feature[0], 6)));
    ui->keyholeLength->setText(QString::fromStdString(formatDoubleValue(this->keyhole_feature[1], 6)));
    ui->keyholeWidth->setText(QString::fromStdString(formatDoubleValue(this->keyhole_feature[2], 6)));
    ui->seamWidth->setText(QString::fromStdString(formatDoubleValue(this->seam_width, 6)));
    ui->seamDev->setText(QString::fromStdString(formatDoubleValue(this->deviation, 6)));
    isdatashowing = false;
    return;
}

MonitorSystem::~MonitorSystem()
{
    delete ui;
    ui = nullptr;
}

/*void MonitorSystem::closeEvent(QCloseEvent *event){
//    if(this->isInit){
//        this->seg_network.release();
//    }
//}*/

void MonitorSystem::mdsendFeature(){
    if(this->mdThread->ctx != NULL){
        this->mdThread->mb_mapping->tab_registers[13] = uint16_t(this->keyhole_feature[0]*100);
        this->mdThread->mb_mapping->tab_registers[14] = uint16_t(this->keyhole_feature[1]*100);
        this->mdThread->mb_mapping->tab_registers[15] = uint16_t(this->keyhole_feature[2]*100);
        this->mdThread->mb_mapping->tab_registers[12] = uint16_t(this->seam_width*100);
        this->mdThread->mb_mapping->tab_registers[11] = int16_t((this->deviation)*100);
//        qDebug() << this->mdThread->mb_mapping->tab_registers[11] << endl;
    }
}

void MonitorSystem::timeoutEvent(){
    this->mbTimer->stop();
    if(this->mdThread->ctx != NULL && this->mdThread->isConnect){
        if(this->mdThread->mb_mapping->tab_registers[0] == 1){  // if welder order to open Camera
//            this->isWelderOpen = true;
//            if(!this->isCameraopen)  // if the camera not open, open the camera
//                cameraOpenEvent();
//            else if(!this->isSeg)  // if the camera is opened, then open the keyhole segmentation
//                keyholeMonitorEvent();
            this->mbstartdetectEvent();
        }
        else if(this->mdThread->mb_mapping->tab_registers[0] == 2){  // if welder order to close Camera
//            this->isWelderOpen = false;
//            if(this->isSeg){
//                keyholeMonitorEvent();
//            }
//            if(this->isCameraopen)
//                cameraCloseEvent();
            this->mbclosecameraEvent();
        }
        else if(this->mdThread->mb_mapping->tab_registers[0] == 4){
            this->mbstopdetectEvent();
        }
        welding_current->status_vec[0] = to_string(this->mdThread->mb_mapping->tab_registers[1]) + "A";
        welding_speed->status_vec[0] = to_string(this->mdThread->mb_mapping->tab_registers[3]) + "mm/min";
        wire_speed->status_vec[0] = to_string(this->mdThread->mb_mapping->tab_registers[4]) + "mm/min";
        welding_current->updateUI(0);
        welding_speed->updateUI(0);
        wire_speed->updateUI(0);
        welder_state->updateUI(1);
    }
    else
        welder_state->updateUI(0);
    this->mbTimer->start(50);
    return;
}

void MonitorSystem::mbclosecameraEvent(){
    if(this->isSeg){
        this->keyholeMonitorEvent();
    }
    if(this->isCameraopen){
        this->cameraCloseEvent();
    }
}

void MonitorSystem::mbstartdetectEvent(){
    if(!this->isCameraopen){
        this->cameraOpenEvent();
    }
    if(!this->isSeg){
        this->keyholeMonitorEvent();
    }
}

void MonitorSystem::mbstopdetectEvent(){
    if(this->isSeg){
        this->keyholeMonitorEvent();
    }
    if(this->isCameraopen){
        this->cameraCloseEvent();
    }
}

Direction MonitorSystem::region(const QPoint & point)
{
    int mouseX = point.x();
    int mouseY = point.y();

    QPoint roiTopLeft(ui->originalImage->x(), ui->originalImage->y());
    QPoint roiBottomRight(roiTopLeft.x() + ui->originalImage->width(), roiTopLeft.y() + ui->originalImage->height());

    Direction dir = DIR_NONE;

    if (mouseX < roiTopLeft.x() || mouseY < roiTopLeft.y() || mouseX > roiBottomRight.x() || mouseY > roiBottomRight.y())
    {
        dir = DIR_OUTER;
    }
    else
    {
        dir = DIR_INNER;
    }

    return dir;
}

void MonitorSystem::mousePressEvent(QMouseEvent * ev)
{
    if (!isSettingSeamPoint && !isSettingKeyholePoint && !deletePoint) return ;
    if (ev->buttons() & Qt::LeftButton)
    {
        Direction dir = region(ev->pos());     //获取鼠标当前的位置
        if (dir == DIR_OUTER) return ;
        QPoint roiTopLeft(ui->originalImage->x(), ui->originalImage->y());
        if (!deletePoint){
            Point t(ev->pos().x() - roiTopLeft.x(), ev->pos().y() - roiTopLeft.y());
            t.x = t.x * cam.cols / ui->originalImage->width();
            t.y = t.y * cam.rows / ui->originalImage->height();
            attentionPoint.emplace_back(t);
        }else{
            for (auto it = attentionPoint.begin(); it != attentionPoint.end(); ++it) {
                if (abs(it->x * ui->originalImage->width() / cam.cols - ev->pos().x() + roiTopLeft.x()) <= 5
                        && abs(it->y * ui->originalImage->height() / cam.rows - ev->pos().y() + roiTopLeft.y()) <= 5) {
                    it = attentionPoint.erase(it);
                    break;
                }
            }
        }

    }
    return ;
}

void MonitorSystem::setKeyholePointEvent(){
    isSettingKeyholePoint = true;
    isSettingSeamPoint = false;
    deletePoint = false;
    ui->setKeyholePoint->setEnabled(false);
    ui->setSeamPoint->setEnabled(false);
    ui->deletePoint->setEnabled(true);
    ui->finishSetPoint->setEnabled(true);


}

void MonitorSystem::setSeamPointEvent(){

    isSettingKeyholePoint = false;
    isSettingSeamPoint = true;
    deletePoint = false;

    ui->setKeyholePoint->setEnabled(false);
    ui->setSeamPoint->setEnabled(false);
    ui->deletePoint->setEnabled(true);
    ui->finishSetPoint->setEnabled(true);

}

void MonitorSystem::deletePointEvent(){
    if (!deletePoint) deletePoint = true;
    ui->setKeyholePoint->setEnabled(true);
    ui->setSeamPoint->setEnabled(true);
    ui->deletePoint->setEnabled(false);
    return;
}

void MonitorSystem::finishSetPointEvent(){
    if (!isSettingKeyholePoint && !isSettingSeamPoint) return ;

//    int width = cam.cols, height = cam.rows;
//    Mat savePic = Mat::zeros(height, width, CV_8UC1);

//    for (auto &i : attentionPoint) {
//        Point t;
//        t.x = (double)i.x() / ui->originalImage->width() * width;
//        t.y = (double)i.y() / ui->originalImage->height() * height;
//        cv::circle(savePic, t, 10, (255,255,255), -1);
//    }
    if (isSettingKeyholePoint) {
//        cv::imwrite("keyholeAttention.jpg", savePic);
//        this->seg_network.keyhole_intervative_point.swap(attentionPoint);
        if(this->isROI)
            this->seg_network.intervative_info_process(attentionPoint,
                                                       seg_network.keyhole_intervative_point,
                                                       this->roirect);
        else
            this->seg_network.intervative_info_process(attentionPoint,
                                                       seg_network.keyhole_intervative_point,
                                                       Rect(0, 0, 0, 0));
        isSettingKeyholePoint = false;
    }
    else {
//        cv::imwrite("seamAttention.jpg", savePic);
        if(this->isROI)
            this->seg_network.intervative_info_process(attentionPoint,
                                                       seg_network.seam_intervative_point,
                                                       this->roirect);
        else
            this->seg_network.intervative_info_process(attentionPoint,
                                                       seg_network.seam_intervative_point,
                                                       Rect(0, 0, 0, 0));
        isSettingSeamPoint = false;
    }
    attentionPoint.clear();
    ui->setKeyholePoint->setEnabled(true);
    ui->setSeamPoint->setEnabled(true);
    ui->deletePoint->setEnabled(false);
    ui->finishSetPoint->setEnabled(false);
}

void MonitorSystem::flipEvent(){
    this->isFlip = !this->isFlip;
}


