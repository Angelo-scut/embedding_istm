#include <atomic>
#include <mutex>
#include <queue>
#include <fstream>
#include <condition_variable>
#include <numeric>
#include "app-istm.hpp"
#include "producer.hpp"
#include "infer-rknn.hpp"
#include "logger.hpp"

namespace istm{
    using namespace cv;
    using namespace std;

    static float desigmoid(float x){
        return -log(1.0f / x - 1.0f);
    }

    static float sigmoid(float x){
        return 1.0f / (1.0f + expf(-x));
    }

    using ControllerImpl = kiwi::Producer
    <
        istm_input,     // input
        cv::Mat,        // output
        string,         // start param
        cv::Mat         // input_coord
    >;
    class InferImpl : public Infer, public ControllerImpl{
    public:

        /** 要求在InferImpl里面执行stop，而不是在基类执行stop **/
        virtual ~InferImpl(){
            stop();
        }

        virtual bool startup(
            const std::string& engine_file,
            float confidence_threshold=0.25f
        ){
            confidence_threshold_ = confidence_threshold;
            return ControllerImpl::startup(engine_file);
        }

        virtual void worker(promise<bool>& result) override{

            auto engine = rknn::load_infer(start_param_);
            if(engine == nullptr){
                result.set_value(false);
                return;
            }

            // clean memory
            start_param_.clear();
            engine->print();

            auto input_tensor               = engine->input(0);  // 两个input怎么办？
            auto input_coord_tensor         = engine->input(1);
            auto output                     = engine->output();
//            float deconfidence_threshold    = desigmoid(confidence_threshold_);  // 哇，好聪明，直接把threshold反向sigmoid，这样就避免了其他的大量sigmoid了
            
            input_width_       = input_tensor->size(3);
            input_height_      = input_tensor->size(2);
            
            result.set_value(true);

            Job fetch_job;
            cv::Size target_size(input_width_, input_height_);
            cv::Mat input_image(input_height_, input_width_, CV_8UC3, input_tensor->cpu());
            cv::Mat input_coord(input_height_, input_width_, CV_8UC3, input_coord_tensor->cpu());
            float* optr = output->cpu<float>();

            while(get_job_and_wait(fetch_job)){                
                fetch_job.input.image.copyTo(input_image);
                fetch_job.additional.copyTo(input_coord);
                if(!engine->forward()){
                    INFOE("Forward failed");
                    fetch_job.pro->set_value({});
                    continue;
                }

                fetch_job.pro->set_value(Mat(input_height_, input_width_, CV_32FC1, output->cpu<float>()));
            }
            INFO("Engine destroy.");
        }

        virtual bool preprocess(Job& job, const istm_input& input) override{
            int image_width = input.image.cols;
            int image_height = input.image.rows;
            cv::Size input_size = cv::Size(input_width_, input_height_);

            cv::resize(input.image, job.input.image, input_size, 0, 0, cv::InterpolationFlags::INTER_LINEAR);
            
            cv::Mat pre_mask;
            // cv::resize(input.pre_mask, pre_mask, input_size, 0, 0, cv::InterpolationFlags::INTER_LINEAR);
            if (input.pre_mask.empty())
            {
                pre_mask = cv::Mat::zeros(input_size, CV_8UC1);
            }
            else{
                input.pre_mask.convertTo(pre_mask, CV_8UC1, 255.0f);
            }
            
            cv::Mat pos_click_map = cv::Mat::zeros(input_size, CV_8UC1);
            cv::Mat neg_click_map = cv::Mat::zeros(input_size, CV_8UC1);
            int num_clicks = input.clicks.size();
            const std::vector<istm::coord>& clicks = input.clicks;
            for (size_t i = 0; i < num_clicks; ++i)
            {
                const istm::coord& click = clicks[i];
                int x = click.x;
                int y = click.y;
                x = int(x * input_width_ / image_width);
                y = int(y * input_height_ / image_height);
                if (click.is_positive)
                {
                    cv::circle(pos_click_map, cv::Point(x, y), 5, cv::Scalar(255), -1);
                }
                else{
                    cv::circle(neg_click_map, cv::Point(x, y), 5, cv::Scalar(255), -1);
                }
                
            }

            cv::merge(std::vector<Mat>({pre_mask, pos_click_map, neg_click_map}), job.additional);
            
            return !input.image.empty();
        }

        virtual std::shared_future<cv::Mat> commit(const istm_input& input) override{
            return ControllerImpl::commit(input);
        }

    private:
        int input_width_            = 0;
        int input_height_           = 0;
        int gpu_                    = 0;
        float confidence_threshold_ = 0;
        float nms_threshold_        = 0;
    };

    shared_ptr<Infer> create_infer(
        const std::string& engine_file,
        float confidence_threshold
    ){
        shared_ptr<InferImpl> instance(new InferImpl());
        if(!instance->startup(
            engine_file, confidence_threshold)
        ){
            instance.reset();
        }
        return instance;
    }

    bool Istm::reset(const std::string& engine_file, float threshold){
		infer_ = create_infer(engine_file, threshold);  // 这边是赋值，那边是构造
        if (infer_ == nullptr)
        {
            return false;
        }
        keyhole_ = cv::Point();
        roi_ = cv::Rect();
        threshold_ = threshold;
        init_filter();
        return true;
    }

    /* forward出来的mask尺寸应该和输出图像一致才对 */
    cv::Mat Istm::forward(const cv::Mat& image, const cv::Mat& init_mask){
        if(image.empty())
            return Mat();
        if (!init_mask.empty())  // 如果初始mask非空
        {
            if (init_mask.type() == CV_8UC1)
                init_mask.convertTo(init_mask, CV_32FC1, 1.0 / 255.0f);

            init_mask.copyTo(pre_mask_);
        }
		int width = image.cols;
		int height = image.rows;

        const istm_input input(image, pre_mask_, clicks_);
        pre_mask_ = infer_->commit(input).get();
        
        cv::Mat result;
        cv::threshold(pre_mask_, result, threshold_, 1, THRESH_BINARY);  // TODO:reisize提取中线后再resize吧，不然cpu处理好难受
        return result;
        
    }

    void Istm::undo_click(){
        lock_guard<mutex> l(clicks_lock_);
        if (clicks_.size() == 0)
        {
            return;
        }
        clicks_.pop_back();
        return;   
    }

	void Istm::add_click(coord click) {
		lock_guard<mutex> l(clicks_lock_);
//		if (!roi_.empty())
        if (roi_.x != 0)
		{
			click.x = click.x - roi_.x;
			click.y = click.y - roi_.y;
		}
		clicks_.emplace_back(click);
	}

	void Istm::clicks_clear() {
		{
			lock_guard<mutex> l_click(clicks_lock_);
			clicks_.clear();
		}
		{
			lock_guard<mutex> l_infer(infer_lock_);
			pre_mask_ = Mat();
		}
	}

    void Istm::set_homo_matrix(const cv::Mat& homo_matrix){
        if (homo_matrix.type() != CV_32FC1)
        {
            INFOE("Homography matrix's type should be CV_32FC1!");
            return;
        }
        
        homo_matrix.copyTo(homo_matrix_);
    }

    void Istm::homo_matrix_from_file(const std::string file){
        std::ifstream txt_file(file);
        if (txt_file.fail())
        {
            INFOE("File fails to open!");
            return;
        }
        for (size_t i = 0; i < 3; i++)
        {
            for (size_t j = 0; j < 3; j++)
            {
                float a;
                txt_file >> a;
                homo_matrix_.at<float>(i, j) = a;
            }
            
        }
        txt_file.close();
        return;
        
    }

    void Istm::plattle(cv::Mat& image, cv::Mat& result){
        cv::Size image_size = cv::Size(image.cols, image.rows);
        cv::Size result_size = cv::Size(result.cols, result.rows);
        result.convertTo(result, CV_8UC1);
        if (image_size != result_size)
        {
            cv::resize(result, result, image_size, 0, 0, cv::InterpolationFlags::INTER_NEAREST);
        }
        
        Mat color_mask(image_size, CV_8UC3, cv::Scalar(0, 0, 255));
//        color_mask.mul(result);
//        cv::imwrite("result_map.jpg", result * 255);
        Mat mask;
        cv::add(color_mask, 0, mask, result);

        cv::addWeighted(image, 0.6, mask, 0.4, 0, image);
        return;
    }

    cv::Vec4f Istm::least_squares_fit(int* center_line, int end, bool is_x){
        std::vector<cv::Point> points;
        if (is_x){
            for (size_t i = 0; i < end; i++)
            {
                if (center_line[i] == 0)  continue;
                cv::Point p(i, center_line[i]);
                points.emplace_back(p);
            }
        }
        else{
            for (size_t i = 0; i < end; i++)
            {
                if (center_line[i] == 0)  continue;
                cv::Point p(center_line[i], i);
                points.emplace_back(p);
            }
        }
        if (points.empty()) return cv::Vec4f();
        
        cv::Vec4f line;
        cv::fitLine(points, line, cv::DIST_L2, 0, 0.01, 0.01);
        return line;
    }

	void Istm::point_trace_center(const cv::Vec4f& line) {
		float vx = line[0], vy = line[1], x0 = line[2], y0 = line[3];
		for (size_t i = 0; i < clicks_.size(); i++)
		{
			coord& click = clicks_[i];
			if (!click.is_positive)	continue;  // 仅做了正向点的跟踪，不想做负向点的跟踪了，累
			switch (trace_axis_)
			{
			case trace_axis::x: {  // x不变，y变
				click.y = vy * (click.x - x0) / (vx + 1e-12) + y0;
				break;
			}
			case trace_axis::y: {  // y不变，x变
				click.x = vx * (click.y - y0) / (vy + 1e-12) + x0;
			}
			default:
				break;
			}
		}
	}

	void Istm::point_trace_side(const Mat& points) {
		for (size_t i = 0; i < clicks_.size(); i++)
		{
			coord& click = clicks_[i];
			int x = click.x;
			int y = click.y;
            ushort* point_ptr = (ushort*)points.ptr();
			switch (trace_axis_)
			{
			case trace_axis::x: {  // x不变，y变
				if (point_ptr[x * 2] == 0)	break;
				auto y_up = point_ptr[x * 2];  // 这个值应该是更小的
				auto y_down = point_ptr[x * 2 + 1];
				int p_width = y_down - y_up;
				int quater_0, quater_1;
				if (click.is_positive)
				{
					quater_0 = y_up + 0.25 * p_width;
					quater_1 = y_up + 0.75 * p_width;
				}
				else
				{
					quater_0 = y_up - 0.25 * p_width;
					quater_1 = y_down + 0.25 * p_width;
				}
				click.y = abs(y - quater_0) < abs(y - quater_1) ? quater_0 : quater_1;  // 离哪个近就去哪个
				break;
			}
			case trace_axis::y: {  // y不变，x变
				if (point_ptr[y * 2] == 0)	break;
				auto x_up = point_ptr[y * 2];
				auto x_down = point_ptr[y * 2 + 1];
				int p_width = x_down - x_up;
				int quater_0, quater_1;
				if (click.is_positive)
				{
					quater_0 = x_up + 0.25 * p_width;
					quater_1 = x_up + 0.75 * p_width;
				}
				else
				{
					quater_0 = x_up - 0.25 * p_width;
					quater_1 = x_down + 0.25 * p_width;
				}
				click.x = abs(x - quater_0) < abs(x - quater_1) ? quater_0 : quater_1;  // 离哪个近就去哪个
			}
			default:
				break;
			}
		}
	}

    cv::Vec4f Istm::weld_center_line(cv::Mat& result){
        CV_Assert( result.channels() == 1 && result.dims == 2 );
        CV_Assert( result.depth() == CV_8U);
        if (trace_axis_ == trace_axis::y)
        {
            cv::rotate(result, result, ROTATE_90_COUNTERCLOCKWISE);
        }

        int rows = result.rows, cols = result.cols;
//        cv::AutoBuffer<int> buf_(cols + 1);
        cv::Mat buf_(1, cols, CV_32SC1);
        int* buf = (int*)buf_.data;
        std::vector<int> width_vec(rows, 0);
        cv::Mat points_mat(rows, 2, CV_16SC1);
        std::vector<int> pos(rows, 0);
        int count = 0, sum = 0;

        for( int i = 0; i < rows; ++i )
        {
            int j, k = 0;
            const uchar* ptr8 = result.ptr(i);
            for( j = 0; j < cols; ++j )
                if( ptr8[j] != 0 ) buf[k++] = j;

            if( k > 0 )
            {
                ++count; // 仅用于记录有点的
                int left = buf[0], right = buf[k - 1];
                sum += right - left;
                width_vec[i] = right - left;  // 宽度
                pos[i] = 0.5 * (right + left);  // 中心线
                ushort* point_ptr = (ushort*)points_mat.ptr(i);  // 原则上我不需要只要左点和右点
                point_ptr[0] = left, point_ptr[1] = right;  // 除非画图以及点跟踪
            }
        }

        float mean = float(sum) / (count + 1e-12);  // 计算mean和标准差
//        float std_ = 0.;
//        for (int i = 0; i < rows; ++i)
//        {
//            int width = width_vec[i];
//            if ( width > 0)
//            {
//                std_ += std::pow((width - mean), 2);
//            }
//        }
//        std_ = std_ / (count + 1e-12);
//        std_ = std::sqrt(std_);
//        std_ = std_ < 10 ? 10. : std_;
//        std_ = std_ > mean ? mean : std_;
//        int uper = std::ceil(mean + 0.15 * std_);
//        int downer = std::ceil(mean - 0.15 * std_);
        int uper = mean + 0.1 * mean;
        int downer = mean - 0.1 * mean;
        std::vector<cv::Point> points;  // 清洗outlier
        int row = rows - 1;
        for (int i = 0; i < rows; ++i)
        {
            int& width = width_vec[i];
            if (width < uper && width > downer)
//            if(width > mean)
            {
                if (trace_axis_ == trace_axis::y)
                    points.emplace_back(cv::Point(row - i, pos[i]));
                else
                    points.emplace_back(cv::Point(pos[i], i));
            }
            else{
                width_vec[i] = 0;
            }
        }
        int width_ = 0;
        for_each(width_vec.begin(), width_vec.end(), [&width_](int x){width_ += x;});
        gap_width_ = (float)width_ / (points.size() + 1e-12);

        if (points.empty()) return cv::Vec4f();
        
        cv::Vec4f line;
        cv::fitLine(points, line, cv::DIST_L2, 0, 0.01, 0.01);

        if (trace_type_ == trace_type::center)
		{
			point_trace_center(line);
		}
		else if(trace_type_ == trace_type::side) {
			point_trace_side(points_mat);
		}

        return line;
    }

    cv::Point2f Istm::homo_transform(const cv::Point2f& p){
        cv::Mat temp = cv::Mat::ones(3, 1, CV_32FC1);
        temp.at<float>(0) = p.x;
        temp.at<float>(1) = p.y;
        temp = homo_matrix_ * temp;
        return cv::Point2f(temp.at<float>(0) / temp.at<float>(2), temp.at<float>(1) / temp.at<float>(2));
    }

    void Istm::width_point_transform(const cv::Vec4f& line){
        float vx = line[0], vy = line[1];
        float x1 = 0., y1 = 0.;
        switch (trace_axis_)
        {
        case trace_axis::x:{
            y1 = vx > 0 ? -gap_width_ : gap_width_;
            x1 = vy * gap_width_ / std::abs(vx);
            break;
        }
        case trace_axis::y:{
            x1 = vy > 0 ? gap_width_ : -gap_width_;
            y1 = -vx * gap_width_ / std::abs(vy);
            break;
        }
        default:
            break;
        }
        cv::Point2f width_point = homo_transform(cv::Point2f(x1, y1)); // （0，0）经过单应性变换后并非就是(0,0)
        cv::Point2f original_point = homo_transform(cv::Point2f(0, 0));
        gap_width_ = cv::sqrt(cv::pow((width_point.x - original_point.x), 2) + cv::pow((width_point.y - original_point.y), 2));
        return;
        // if (vx == 0)
        // {
        //     y1 = 0;
        // }
        // else if (vy == 0)
        // {
        //     x1 = 0;
        // }
        // else{
        //     y1 = std::sqrt(std::pow(gap_width_, 2) * std::pow(vx, 2));
        //     x1 = -(vy * y1) / vx;  // x0x1 + y0y1 = 0 -> x1 = y0, y1 = -x0,但是这样太小了，单应性变换误差会很大
        // }                           // x1 ** 2 + y1 ** 2 = gap_width ** 2
        // return cv::Point2f(line[1], -line[0]);
        
    }

    void Istm::deviation(cv::Mat& image, cv::Vec4f& line){
        cv::Point2f cross_point(0, 0);
        float step = 0.0;
        float vx = line[0], vy = line[1], x0 = line[2], y0 = line[3];
        // printf("%f,%f\n", vx, vy);
        float k = vy / (vx + 1e-12);
        float b = y0 - k * x0;
        float ab2 = k * k + 1;
        cross_point.x = int((keyhole_.x + k * keyhole_.y - k * b) / ab2);
        cross_point.y = int((k * k * keyhole_.y + k * keyhole_.x + b) / ab2);
        cv::line(image, cv::Point(x0, y0), cv::Point(cross_point), cv::Scalar(0, 255, 0));
        cv::line(image, keyhole_, cv::Point(cross_point), cv::Scalar(0, 255, 0));
        cross_point = homo_transform(cross_point);
        cv::Point2f keyhole_h = homo_transform(keyhole_);
        deviation_ = cv::sqrt(cv::pow((cross_point.x - keyhole_h.x), 2) + cv::pow((cross_point.y - keyhole_h.y), 2));
        width_point_transform(line);
        switch (trace_axis_)
        {
        case trace_axis::x:{
            deviation_ = cross_point.y > keyhole_.y ? -deviation_ : deviation_;
            break;
        }
        case trace_axis::y:{
            deviation_ = cross_point.x > keyhole_.x ? -deviation_ : deviation_;
            break;
        } 
        default:
            break;
        }
    }

    void Istm::infer_and_draw(cv::Mat& image){
		lock_guard<mutex> l_infer(infer_lock_);
        cv::Mat input_image = image(cv::Rect(0, 0, image.cols, image.rows));
//		if (!roi_.empty())
        if (roi_.width != 0)
		{
			input_image = image(roi_);
		}
		Mat result;
		{
			std::lock_guard<std::mutex> l_click(clicks_lock_);
			if (clicks_.size() > 0)
			{
				result = forward(input_image);
			}
		}

        cv::Vec4f center_line;
        if (!result.empty())
        {

//            int width = input_image.cols;
//            int height = input_image.rows;
//            if(!(height == result.rows && width == result.cols)) return;

            if (roi_.width != 0)
			{
                cv::Mat roi;
				input_image.copyTo(roi);
                this->plattle(roi, result);
				roi.copyTo(input_image);
			}
			else
			{
                this->plattle(input_image, result);
			}
            center_line = weld_center_line(result);
//            if (!roi_.empty())
            if (roi_.width != 0)
            {
                center_line[2] += roi_.x;
                center_line[3] += roi_.y;
            }
            
//            result = result * 255;
//            cv::cvtColor(result, result, CV_GRAY2BGR);
//            result.copyTo(image);
            
        }
		{
			std::lock_guard<std::mutex> l(clicks_lock_);
			for (size_t i = 0; i < clicks_.size(); i++)
			{
				if (clicks_[i].is_positive)
				{
					cv::circle(input_image, cv::Point(clicks_[i].x, clicks_[i].y), 3, cv::Scalar(0, 255, 0), -1);
				}
				else {
					cv::circle(input_image, cv::Point(clicks_[i].x, clicks_[i].y), 3, cv::Scalar(0, 0, 255), -1);
				}

			}
		}
        if (keyhole_.x > 0)
        {
            cv::circle(image, keyhole_, 3, cv::Scalar(255, 0, 0), -1);
            if (!(center_line[2] == 0 && center_line[3] == 0))
            {
                deviation(image, center_line);
            }
            
        }
        if (roi_.width != 0)
        {
            cv::rectangle(image, roi_, cv::Scalar(0, 255, 0), 1);
        }
        return;
    }
};
