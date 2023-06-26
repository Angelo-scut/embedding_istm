QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets network

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    ui/MonitorSystem.cpp \
    ui/status_Label.cpp \
    modbus/modbusthread.cpp \
    rknn_infer/app-istm.cpp \
    rknn_infer/infer-rknn.cpp \
    rknn_infer/json.cpp \
    rknn_infer/logger.cpp \
    rknn_infer/tensor.cpp


HEADERS += \
    modbus/modbusthread.h \
    rknn_infer/app-istm.hpp \
    rknn_infer/infer.hpp \
    rknn_infer/infer-rknn.hpp \
    rknn_infer/json.hpp \
    rknn_infer/logger.hpp \
    rknn_infer/producer.hpp \
    rknn_infer/tensor.hpp \
    ui/MonitorSystem.h \
    ui/status_Label.h


FORMS += \
    ui/MonitorSystem.ui

INCLUDEPATH +=/usr/include/opencv \
               /usr/include/opencv2
INCLUDEPATH += /usr/include/knn
INCLUDEPATH += /usr/include/modbus
INCLUDEPATH += ./modbus
INCLUDEPATH += ./rknn_infer
INCLUDEPATH += ./ui
INCLUDEPATH += ./source
LIBS += /usr/lib/aarch64-linux-gnu/librknn_api.so
LIBS += /usr/lib/aarch64-linux-gnu/libopencv_*.so
LIBS += /usr/lib/aarch64-linux-gnu/libmodbus.so
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
