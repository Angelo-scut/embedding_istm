QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets network

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    MonitorSystem.cpp \
    main.cpp \
    segmentationNetwork.cpp \
    status_Label.cpp \
    modbusthread.cpp

HEADERS += \
    MonitorSystem.h \
    segmentationNetwork.h \
    status_Label.h \
    modbusthread.h

FORMS += \
    MonitorSystem.ui

INCLUDEPATH +=/usr/include/opencv \
               /usr/include/opencv2
INCLUDEPATH += /usr/include/knn
INCLUDEPATH += /usr/include/modbus
LIBS += /usr/lib/aarch64-linux-gnu/librknn_api.so
LIBS += /usr/lib/aarch64-linux-gnu/libopencv_*.so
LIBS += /usr/lib/aarch64-linux-gnu/libmodbus.so
# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
