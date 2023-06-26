#include "EEcamera.h"
#include <math.h>

using namespace cv;
int align_32(int value) {  // 不是要幂啊，是32的倍数啊
	int factor = (value + 32 - 1) / 32;
	return 32 * factor;
}

EEcamera::EEcamera(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
	QObject::connect(this, SIGNAL(image_show_signal()), this, SLOT(image_show_slot()));

    QObject::connect(ui.cameraOpenButton, &QPushButton::clicked, this, &EEcamera::cameraOpenEvent);
    QObject::connect(ui.camerCloseButton, &QPushButton::clicked, this, &EEcamera::cameraCloseEvent);
    QObject::connect(ui.saveButton, &QPushButton::clicked, this, &EEcamera::imageSaveEvent);
    QObject::connect(ui.endButton, &QPushButton::clicked, this, &EEcamera::imageEndsaveEvent);
    QObject::connect(ui.roiSetButton, &QPushButton::clicked, this, &EEcamera::setROIEvent);
    QObject::connect(ui.roiDelButton, &QPushButton::clicked, this, &EEcamera::delROIEvent);
    QObject::connect(ui.captureOnce, &QPushButton::clicked, this, &EEcamera::CaptureOnceEvent);

	QObject::connect(ui.calibraButton, &QPushButton::clicked, this, &EEcamera::calibrateButtonEvent);
	QObject::connect(ui.keyholeClickButton, &QPushButton::clicked, this, &EEcamera::keyholeEvent);
	QObject::connect(ui.modelLoadButton, &QPushButton::clicked, this, &EEcamera::modelLoadEvent);
	QObject::connect(ui.mRoiButton, &QPushButton::clicked, this, &EEcamera::netRoiEvent);
	QObject::connect(ui.addClickButton, &QPushButton::clicked, this, &EEcamera::addClickEvent);
	QObject::connect(ui.undoClickButton, &QPushButton::clicked, this, &EEcamera::undoClickEvent);
	QObject::connect(ui.clearClickButton, &QPushButton::clicked, this, &EEcamera::clearClickEvent);

	QObject::connect(ui.axisBox, &QComboBox::currentTextChanged, this, &EEcamera::axisChangeEvent);
	QObject::connect(ui.pTraceBox, &QComboBox::currentTextChanged, this, &EEcamera::pTraceChangeEvent);
	QObject::connect(ui.preMaskBox, &QComboBox::currentTextChanged, this, &EEcamera::preMaskTypeChangeEvent);
    isSaved = false;
    isRecord = false;
    is_camera_open_ = false;//控制相机线程的flag
    roi = false;
    isSequence = false;
    cx = 0; cy = 0; w = 0; h = 0; device_num = 0;

	modelWidgetState(false);
}

void EEcamera::cameraOpenEvent()
{
    bool flag = this->camera.open(this->device_num);
    if (!flag) {
        ShowErrorMsg("Failed to Open Camera", 0);
        return;
    }
    this->camera.set(3, 1920);
    this->camera.set(4, 1080);

    flag = this->camera.read(cam);
    if (!flag) {
        ShowErrorMsg("Failed to Read Image", 0);
        this->camera.release();
        return;
    }

    cam.copyTo(this->myImage);
    //this->myImage = cv::imread("1.jpg"); //测试
    this->cx = int(this->myImage.cols / 2); cy = int(this->myImage.rows / 2); w = cx; h = cy;
    this->cx = this->cx - int(w / 2);
    this->cy = this->cy - int(h / 2);
    ui.cxEdit->setText(QString::number(this->cx)); ui.cyEdit->setText(QString::number(this->cy));
    ui.wEdit->setText(QString::number(this->w)); ui.hEdit->setText(QString::number(this->h));

    this->is_camera_open_ = true;
	capture_worker_ = make_shared<std::thread>(&EEcamera::capture_t, this); //采集子线程
	show_worker_ = make_shared<std::thread>(&EEcamera::show_t, this);

    ui.cameraOpenButton->setEnabled(false);
    ui.camerCloseButton->setEnabled(true);
    return;
}

void EEcamera::save_one_image()
{
	if (savePath == NULL)
	{
		return;
	}
	std::string file_name = this->savePath.toStdString() + '/' + std::to_string(this->device_num) + '_';
	file_name += std::to_string(this->num_capture) + ".jpg";
	cv::imwrite(file_name, cam);
	cv::waitKey(5);
	this->num_capture++;
}

void EEcamera::capture_t()  // 这边肯定是生产者
{
    cv::Mat weldpoolroi;
    uint img_num = 0;
    bool flag = true;
    clock_t start, end;
    start = end = clock();
	show_pro_->set_value(true);
    while (is_camera_open_)
    {
		{
			unique_lock<std::mutex> l_cap(image_lock_);
			flag = this->camera.read(cam); // open which camera
			//cam = cv::imread("1.jpg"); //测试
			if (!flag) {  // 如果没采集到还可以再继续看能不能恢复
				if (!this->is_camera_open_) // close and end recording
				{
					return;
				}
				continue;
			}
			end = clock();
			time_int = float(end - start) / CLOCKS_PER_SEC;
			start = clock();
			
		}
		show_pro_->get_future().wait_for(std::chrono::seconds(1));
		show_pro_ = make_shared<std::promise<bool>>();
		show_cond_.notify_one();
		if (!(stack_worker_ == nullptr))
		{
			stack_pro_->get_future().wait_for(std::chrono::seconds(1));
			stack_pro_ = make_shared<std::promise<bool>>();
			stack_cond_.notify_one();
		}
        
        if(this->isCapture_once){
			save_one_image();
            this->isCapture_once = false;
        }
    }
    return;
}

bool EEcamera::get_show_mat() {

	unique_lock<mutex> l_show(image_lock_);
	show_cond_.wait(l_show);

	if (!is_camera_open_) return false;
	cam.copyTo(mat_for_show);
	return true;

}

void EEcamera::show_t()  // 消费者
{
	while (get_show_mat())
	{
		if (mat_for_show.empty()) continue;
		if (is_model_load_)
		{
			clock_t start, end;
			start = clock();
			model->infer_and_draw(mat_for_show);
			end = clock();
			time_int = 1.0 / (float(end - start) / CLOCKS_PER_SEC);
			cv::putText(mat_for_show, std::to_string(time_int), cv::Point(20, 50), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 255, 0));
		}
		if (this->roi) {
			rectangle(mat_for_show, imgROI, Scalar(255, 255, 255), 2, LINE_8, 0);
		}
		cv::resize(mat_for_show, mat_for_show, cv::Size(ui.imgShow->width(), ui.imgShow->height()));
		qmat = this->cvMat2QImage(mat_for_show);
		emit image_show_signal();
	}
}

void EEcamera::image_show_slot() {
	ui.imgShow->setPixmap(QPixmap::fromImage(qmat));  // TODO:信号槽的方式显示图像
	if (is_model_load_)
	{
		ui.gapWidthEdit->setText(QString::number(model->get_width()));
		ui.DeviationEdit->setText(QString::number(model->get_deviation()));
	}
	show_pro_->set_value(true);
}

bool EEcamera::get_stack_mat() {
	unique_lock<mutex> l_stack(image_lock_);
	stack_cond_.wait(l_stack);
	if (!(is_camera_open_ && isRecord)) return false;
	{
		lock_guard<mutex> l_seq_stack(image_seq_lock_);
		if (this->roi) {
			//Mat stack;
			//cam(this->imgROI).copyTo(stack);
			img_seq.emplace(cam(this->imgROI));
		}
		else
		{
			//Mat stack;
			//cam.copyTo(stack);
			img_seq.emplace(cam);  // TODO:回复制么？
		}
		//float stack_time = time_int;
		time_seq.emplace(time_int);
	}
	this->img_count++;
	return true;
}

void EEcamera::stack_t(){  // 生产者和消费者
	while (get_stack_mat())
		stack_pro_->set_value(true);
}

void EEcamera::time_save_event() {
	if (savePath == NULL) return;
	std::ofstream out_file;
	std::string txt_file_name = this->savePath.toStdString() + '/' + "time.txt";

	if (!time_seq.empty())
	{
		out_file.open(txt_file_name, std::ios::out | std::ios::trunc);

		for (size_t i = 0; !time_seq.empty(); i++)
		{
			out_file << time_seq.front() << endl;
			time_seq.pop();
		}
		out_file.close();
	}
	return;
}

void EEcamera::save_t() {  // 消费者
	std::queue<cv::Mat> temp_image;
	//std::queue<float> temp_time;
	int count = 10000;
	while (1) {
		{
			lock_guard<std::mutex> l_seq_save(image_seq_lock_);
			for (size_t i = 0; !img_seq.empty(); i++)
			{
				temp_image.emplace(std::move(img_seq.front()));
				img_seq.pop();
			}
			//for (size_t i = 0; !time_seq.empty(); i++)
			//{
			//	temp_time.emplace(std::move(time_seq.front()));
			//	time_seq.pop();
			//}
		}
		for (size_t i = 0; !temp_image.empty(); i++){
			std::string file_name = this->savePath.toStdString() + '/' + std::to_string(this->device_num) + '_';
			file_name += std::to_string(count) + ".jpg";
			cv::imwrite(file_name, temp_image.front());
			temp_image.pop();
			count++;
		}
		if (!(is_camera_open_ && isRecord)) {
			lock_guard<std::mutex> l(image_seq_lock_);
			if (img_seq.empty())
			{
				time_save_event();
				return;
			}			
		}
			
	}
}

void EEcamera::release_capture()
{
	if (!(capture_worker_ == nullptr))
	{
		capture_worker_->join();
		capture_worker_.reset();
	}
}

void EEcamera::release_show()
{
	if (!(show_worker_ == nullptr))
	{
		show_cond_.notify_all();
		show_worker_->join();
		show_worker_.reset();
	}
}

void EEcamera::release_save()
{
	if (!(save_worker_ == nullptr)) {
		save_worker_->join();
		save_worker_.reset();
	}
}

void EEcamera::release_stack()
{
	if (!(stack_worker_ == nullptr)) {
		stack_cond_.notify_all();
		stack_worker_->join();
		stack_worker_.reset();
	}
}

QImage EEcamera::cvMat2QImage(const cv::Mat & mat)
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

void EEcamera::cameraCloseEvent()
{
    this->is_camera_open_ = false;
	release_capture();
	release_show();
	release_stack();
	release_save();
    this->camera.release();  // 如果重新激活呢？
    ui.camerCloseButton->setEnabled(false);
    ui.cameraOpenButton->setEnabled(true);
}

void EEcamera::imageSaveEvent()
{
    if (!this->isSaved) {
        this->savePath = QFileDialog::getExistingDirectory(this, "save");
        ui.pathEdit->setText(this->savePath);
        if (this->savePath != NULL)
        {
            ui.saveButton->setText("Start");
            this->isSaved = true;
            if (this->isMonitor)
            {
                ui.saveButton->setEnabled(false);
            }
        }
    }
    else
    {
        this->isRecord = true;
		stack_pro_ = make_shared <promise<bool>>();
		stack_pro_->set_value(true);
        ui.endButton->setEnabled(true);
        ui.saveButton->setEnabled(false);
        ui.saveButton->setText("Save");
		stack_worker_ = make_shared<std::thread>(&EEcamera::stack_t, this);
		save_worker_ = make_shared<std::thread>(&EEcamera::save_t, this);
    }
}

void EEcamera::imageEndsaveEvent()
{
    ui.endButton->setEnabled(false);
    this->isRecord = false;
	release_stack();
	release_save();
    ui.pathEdit->setText(" ");
    this->isSaved = false;
    //while(!this->endsave);
    //this->endsave = false;
    this->img_count = 10000; // 图片index归零
    ui.saveButton->setEnabled(true);
}

void EEcamera::setROIEvent()
{
    this->cx = ui.cxEdit->text().toInt();
    this->cy = ui.cyEdit->text().toInt();
    this->w = ui.wEdit->text().toInt();
    this->h = ui.hEdit->text().toInt();
    if (this->w > 0 && this->h > 0)
    {
        this->imgROI = cv::Rect(this->cx, this->cy, this->w, this->h);
        this->roi = true;
    }
    else
        this->roi = false;
}

void EEcamera::delROIEvent()
{
    this->cx = 0; cy = 0; w = 0; h = 0;
    ui.cxEdit->setText(" "); ui.cyEdit->setText(" ");
    ui.wEdit->setText(" "); ui.hEdit->setText(" ");
    this->roi = false;
}

void EEcamera::ShowErrorMsg(QString Message, int nErrorNum)
{
    QMessageBox box;
    box.setText(Message);
    box.exec();
}

void EEcamera::CaptureOnceEvent(){
    this->isCapture_once = true;
}

void EEcamera::modelWidgetState(bool state)
{
	ui.calibraButton->setEnabled(state);
	ui.keyholeClickButton->setEnabled(state);
	ui.mRoiButton->setEnabled(state);
	ui.addClickButton->setEnabled(state);
	ui.undoClickButton->setEnabled(state);
	ui.clearClickButton->setEnabled(state);
	ui.axisBox->setEnabled(state);
	ui.pTraceBox->setEnabled(state);
	ui.preMaskBox->setEnabled(state);
}

void EEcamera::keyholeEvent(){
    isKeyhole = true;
	isModelroi = false;
	isAddClick = false;
}

void EEcamera::calibrateEvent() {
	isCalibrate = false;
	Size board_size = Size(11, 8);	// 棋盘格内角点个数
	Size square_size = Size(3, 3);  // 棋盘格实际大小，单位mm
	std::vector<Point2f> corners;
	if (!findChessboardCorners(cam, board_size, corners)) return;

	Mat gray_image;
	cvtColor(cam, gray_image, COLOR_BGR2GRAY);
	find4QuadCornerSubpix(gray_image, corners, Size(5, 5));  // 亚像素

	std::vector<Point2f> objps;  // 实际坐标
	for (int i = 0; i < board_size.height; i++)
	{
		for (int j = 0; j < board_size.width; j++)
		{
			float x = i * square_size.width;
			float y = j * square_size.height;
			objps.emplace_back(Point2f(x, y));
		}
	}

	Mat homo = findHomography(corners, objps);
	if (is_model_load_)
	{
		model->set_homo_matrix(homo);
	}
}

void EEcamera::calibrateButtonEvent(){
	isCalibrate = true;
}

void EEcamera::netRoiEvent(){
    isModelroi = true;
	isKeyhole = false;
	isAddClick = false;
}

void EEcamera::modelLoadEvent(){
	QString file = QFileDialog::getOpenFileName(this, "model");
	if (file != NULL)
	{
		is_model_load_ = false;
		//model = make_shared<istm::Istm>(file.toStdString());
		if (!model->reset(file.toStdString())) 
		{
			std::string builded_file = istm::build_model(file.toStdString());
			if (builded_file.empty() && !model->reset(builded_file))  // 短路性质，如果前面是空，则不会进入第二个判断
			{
				modelWidgetState(false);
				ShowErrorMsg("Invalid model.");
				return;
			}
		}
		{
			modelWidgetState(true);
			is_model_load_ = true;
			model->homo_matrix_from_file("homo_matrix.txt");
		}
	}
}

void EEcamera::addClickEvent()
{
	isAddClick = true;
	isKeyhole = false;
	isModelroi = false;
}

void EEcamera::undoClickEvent()
{
	model->undo_click();
}

void EEcamera::clearClickEvent()
{
	model->clicks_clear();
}

void EEcamera::axisChangeEvent(QString)
{
	if (ui.axisBox->currentText() == 'y')
	{
		model->set_trace_axis(istm::trace_axis::y);
	}
	else
	{
		model->set_trace_axis(istm::trace_axis::x);
	}
}

void EEcamera::pTraceChangeEvent(QString)
{
	QString str = ui.pTraceBox->currentText();
	if (str == "None")
	{
		model->set_trace_type(istm::trace_type::none);
	}
	else if (str == "Center") {
		model->set_trace_type(istm::trace_type::center);
	}
	else
	{
		model->set_trace_type(istm::trace_type::side);
	}
}

void EEcamera::preMaskTypeChangeEvent(QString)
{
	QString str = ui.preMaskBox->currentText();
	if (str == "None")
	{
		model->set_mask_type(istm::mask_type::none);
	}
	else if (str == "Binary") {
		model->set_mask_type(istm::mask_type::binary);
	}
	else
	{
		model->set_mask_type(istm::mask_type::prob);
	}
}

bool EEcamera::checkPointOutofWindow(QPoint& pos) {
	pos = pos - ui.imgShow->pos();
	int x = pos.x(), y = pos.y();
	int width = ui.imgShow->width(), height = ui.imgShow->height();
	if (x < 0 || y < 0 || x > width || y > height)
	{
		return true;
	}
	x = int(float(x * cam.cols) / ui.imgShow->width());
	y = int(float(y * cam.rows) / ui.imgShow->height());
	pos.setX(x); pos.setY(y);
	return false;
}

void EEcamera::mousePressEvent(QMouseEvent *e){
	if (!(isAddClick || isModelroi || isKeyhole) || cam.empty())  // 短路性质，避免频繁访问cam
	{
		return;
	}
	QPoint pos = e->pos();
	if (checkPointOutofWindow(pos))
		return;
	if(e->button() == Qt::LeftButton){
		if (isAddClick)
		{
			istm::coord click(pos.x(), pos.y(), POSITIVE);
			model->add_click(click);
		}
		else if (isModelroi) {
			if (roi_click_cout == 0)
			{
				roi_click_cout++;
				firstClick = pos;
			}
			else
			{
				isModelroi = false;
				roi_click_cout = 0;
				int w = abs(pos.x() - firstClick.x());
				int h = abs(pos.y() - firstClick.y());
				w = align_32(w);
				h = align_32(h);
				model->set_roi(cv::Rect(firstClick.x(), firstClick.y(), w, h));
			}
		}
		else if (isKeyhole) {
			model->set_keyhole(cv::Point(pos.x(), pos.y()));
			isKeyhole = false;
		}
    }
    else if(e->button() == Qt::RightButton && isAddClick){
		istm::coord click(pos.x(), pos.y(), NEGATIVE);
		model->add_click(click);
    }
}
