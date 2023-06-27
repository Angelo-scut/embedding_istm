//#include "MonitorSystem.h"
#include "EEcamera.h"
//#include "app-istm.hpp"

#include <QApplication>
#include <QLocale>
#include <QTranslator>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    MonitorSystem w;
    EEcamera w;
    w.show();
    return a.exec();
}


//int main(int argc, char *argv[]){
//    std::string root_path = "/home/firefly/Desktop/K-TIG_Welding_Monitor_System/config/";
//    istm::Istm model(root_path + "weld_h18s_itermask_transfer.rknn");
//    cv::Mat image = cv::imread(root_path + "0.jpg");
//    istm::coord click(71, 150, true);
//    model.add_click(click);
//    cv::Mat result = model.forward(image);
////    cv::imshow("pre_mask", model.get_pre_mask());
////    cv::imshow("result", result);
////    cv::waitKey();
//    model.infer_and_draw(image);
//    cv::imwrite("result.jpg", image);
//}
