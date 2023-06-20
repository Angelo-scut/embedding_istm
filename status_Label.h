#ifndef STATUS_LABEL_H
#define STATUS_LABEL_H

#include <QtWidgets/QMainWindow>
#include <QMouseEvent>
#include <QWidget>
#include <QLabel>
#include <QLayout>
#include <vector>
#include <string>
#include <QString>

using namespace std;

class status_Lable : public QWidget{
    Q_OBJECT
public:
    status_Lable(vector<string>& status_v, string fix_message, bool is_led);
    void updateUI(int status);
    QLabel* led_label; // 灯的颜色
    const QString m_red_SheetStyle = "min-width: 10px; min-height: 10px;max-width:10px; max-height: 10px;border-radius: 6px;  border:1px solid black;background:red";
    const QString m_green_SheetStyle = "min-width: 10px; min-height: 10px;max-width:10px; max-height: 10px;border-radius: 6px;  border:1px solid black;background:green";
    const QString m_grey_SheetStyle = "min-width: 10px; min-height: 10px;max-width:10px; max-height: 10px;border-radius: 6px;  border:1px solid black;background:grey";
    QLabel* fix_message;
    QLabel* status_message;
    QHBoxLayout* status_layout;
    vector<string> status_vec;
    bool is_led;
};
#endif // STATUS_LABEL_H
