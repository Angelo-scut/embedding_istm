QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets network

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    modbus/modbusthread.cpp \
    rknn_infer/app-istm.cpp \
    rknn_infer/infer-rknn.cpp \
    rknn_infer/json.cpp \
    rknn_infer/logger.cpp \
    rknn_infer/tensor.cpp \
    ui/EEcamera.cpp


HEADERS += \
    modbus/modbusthread.h \
    rknn_infer/app-istm.hpp \
    rknn_infer/infer.hpp \
    rknn_infer/infer-rknn.hpp \
    rknn_infer/json.hpp \
    rknn_infer/logger.hpp \
    rknn_infer/producer.hpp \
    rknn_infer/tensor.hpp \
    ui/EEcamera.h


FORMS += \
    ui/EEcamera.ui
ROOT_PATH = /home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm
INCLUDEPATH +=/usr/include/opencv \
               /usr/include/opencv2
INCLUDEPATH += /usr/include/knn
INCLUDEPATH += /usr/include/modbus
#INCLUDEPATH += /home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/include
#INCLUDEPATH += /home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/include/opencv2
INCLUDEPATH += /home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/modbus
INCLUDEPATH += /home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/rknn_infer
INCLUDEPATH += /home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/ui
INCLUDEPATH += /home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/source
LIBS += /usr/lib/aarch64-linux-gnu/librknn_api.so
LIBS += /usr/lib/aarch64-linux-gnu/libopencv_*.so
#LIBS += /home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_*.so
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_stitching.so
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_core.so.405
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_stitching.so.4.5.5
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_stitching.so.405
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_imgproc.so.405
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_videoio.so.405
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_ml.so
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_flann.so.405
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_video.so
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_features2d.so.405
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_imgcodecs.so.4.5.5
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_dnn.so.405
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_videoio.so.4.5.5
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_core.so.4.5.5
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_imgcodecs.so.405
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_objdetect.so
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_objdetect.so.4.5.5
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_features2d.so.4.5.5
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_flann.so.4.5.5
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_calib3d.so
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_highgui.so.4.5.5
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_core.so
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_highgui.so.405
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_video.so.4.5.5
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_calib3d.so.405
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_videoio.so
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_photo.so.4.5.5
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_imgcodecs.so
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_ml.so.405
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_imgproc.so.4.5.5
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_dnn.so
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_photo.so.405
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_photo.so
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_features2d.so
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_flann.so
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_ml.so.4.5.5
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_dnn.so.4.5.5
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_video.so.405
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_calib3d.so.4.5.5
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_imgproc.so
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_highgui.so
#LIBS+=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/libopencv_objdetect.so.405

LIBS += /usr/lib/aarch64-linux-gnu/libmodbus.so
#export OPENCV_LIB_PATH=/home/firefly/Desktop/K-TIG_Welding_Monitor_System/weldingMonitor/embedding_istm/opencv-4.5.5/lib/
#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$OPENCV_LIB_PATH
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
