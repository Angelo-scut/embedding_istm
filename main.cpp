//#include "MonitorSystem.h"
#include "EEcamera.h"

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
