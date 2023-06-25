#ifndef APP_ISTM_HPP
#define APP_ISTM_HPP

#include <opencv2/opencv.hpp>
#include <future>
#include <vector>
#include <memory>
#include <string>
#include "tensor.hpp"

namespace istm{
    enum class CoordType : int{
        Linfinite   = 0,
        L1          = 1,
        L2          = 2
    };

    enum class trace_axis : int{
        x = 0,
        y = 1
    };

    enum class trace_type :int
	{
		none = 0,
		center = 1,
		side = 2
	};

	enum class mask_type :int
	{
		none = 0,
		binary = 1,
		prob = 2
	};

    struct coord{
        int x;
        int y;
        bool is_positive;
        coord(int x, int y, bool is_positive):x(x), y(y), is_positive(is_positive){}
        cv::Point point() const {return cv::Point(x, y);}
    };

    struct istm_input{
        cv::Mat image;
        cv::Mat pre_mask;
        vector<coord> clicks;
        istm_input(){}
        istm_input(const cv::Mat& image, cv::Mat& pre_mask, const vector<coord>& clicks):image(image), pre_mask(pre_mask), clicks(clicks){}
    };

    class Infer{
    public:
        virtual std::shared_future<cv::Mat> commit(const istm::istm_input& input) = 0;
    };

    std::shared_ptr<Infer> create_infer(
        const std::string& engine_file,
        float confidence_threshold=0.25f
    );

    class Istm{
    public:
        // Istm() = delete;
        Istm(){
			infer_ = create_infer("", 0.49);
		}
        Istm(const string& engine_file, float threshold = 0.5f){
            // if(iLogger::end_with(engine_file, "onnx")){

            // }
            infer_ = create_infer(engine_file, threshold);
            threshold_ = threshold;
            init_filter();
        }

        ~Istm() {
			infer_.reset();
			clicks_.clear();
			// pre_mask_->release();
			// homo_matrix_->release();  // 可以不调用，因为用了智能指针会自动调用析构函数释放
            // workspace_->release();
			deviation_ = 0.0f;
			gap_width_ = 0.0f;
			threshold_ = 0.49f;
		}

        static bool build_model(const string& model_file);

        bool reset(const string& engine_file, float threshold = 0.5f);

        cv::Mat forward(const cv::Mat& image, const cv::Mat& init_mask = cv::Mat());

        void undo_click();
		void add_click(coord click);
		void clicks_clear();
        const vector<coord> get_clicks() const{ return clicks_; }

        void set_keyhole(cv::Point keyhole){ keyhole_ = keyhole; }
        cv::Point get_keyhole() const { return keyhole_; }

        void set_homo_matrix(const cv::Mat& homo_matrix);
        void homo_matrix_from_file(const std::string file);
        const cv::Mat get_homo_matrix() const { return homo_matrix_; }

		void set_roi(const cv::Rect roi) { clicks_clear();  roi_ = roi; }
        const cv::Rect get_roi() const{return roi_;}

        void set_trace_axis(const trace_axis axis){ trace_axis_ = axis; }
        const trace_axis get_trace_axis() const { return trace_axis_; }

        void set_trace_type(const trace_type type){ trace_type_ = type; }
        const trace_type get_trace_type() const { return trace_type_; }

        void set_mask_type(const mask_type type){ mask_type_ = type; }
        const mask_type get_mask_type() const { return mask_type_; }

        float get_width() const { return gap_width_;}
        float get_deviation() const { return deviation_; }

        void infer_and_draw(cv::Mat& image);

    private:
        cv::Point2f homography_transfom(float x, float y);
        void plattle(cv::Mat& image, cv::Mat& result);
        cv::Vec4f weld_center_line(cv::Mat& result);
        void init_filter(){
            filter_ = cv::Mat::zeros(cv::Size(1, 3), CV_8UC1);
            filter_.at<uchar>(0) = -3, filter_.at<uchar>(1) = 0, filter_.at<uchar>(2) = 3;
        }
        cv::Vec4f least_squares_fit(int* center_line, int end, bool is_x);
        void deviation(cv::Mat& image, cv::Vec4f& line);
        cv::Point2f homo_transform(const cv::Point2f& p);
        void width_point_transform(const cv::Vec4f& line);
		void point_trace_center(const cv::Vec4f& line);
		void point_trace_side(const int height, const int width, const int size);
        
        std::shared_ptr<Infer> infer_;
        std::vector<coord> clicks_;
        std::mutex clicks_lock_;
		std::mutex infer_lock_;
        cv::Mat pre_mask_;  // 默认就是float类型Tensor
        // shared_ptr<TRT::Tensor> workspace_; // 用于后处理
        cv::Mat filter_;
		// shared_ptr<TRT::Tensor> edge_points_;
        cv::Point keyhole_ = cv::Point();
        cv::Rect roi_;
        trace_axis trace_axis_  = trace_axis::x;
        trace_type trace_type_  = trace_type::none;
        mask_type mask_type_    = mask_type::prob;  // 用于迭代预测时，输入的分割结果类型
        cv::Mat homo_matrix_    = cv::Mat::ones(3, 3, CV_32FC1);
        float deviation_        = 0.0f;
        float gap_width_        = 0.0f;
        float threshold_        = 0.49f;
        
    }; 
}; // namespace istm

#endif // KIWI_APP_SCRFD_HPP