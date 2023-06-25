#include "status_Label.h"

status_Lable::status_Lable(vector<string>& status_v, string fix_message, bool is_led){
    status_v.swap(status_vec);
    this->is_led = is_led;
    this->fix_message = new QLabel(this);
    status_message = new QLabel(this);
    status_layout = new QHBoxLayout(this);
    if(is_led){
        led_label = new QLabel(this);
        status_layout->addWidget(this->led_label);
        status_layout->addWidget(this->fix_message);
        status_layout->addWidget(this->status_message);
    }
    else{
        status_layout->addWidget(this->fix_message);
        status_layout->addWidget(this->status_message);
    }
    this->fix_message->setText(QString::fromStdString(fix_message));
    updateUI(0);
}

void status_Lable::updateUI(int status){
    switch (status) {
    case 0:
        if(is_led){
            this->led_label->setStyleSheet(m_red_SheetStyle);
        }
        this->status_message->setText(QString::fromStdString(status_vec[status]));
        break;
    case 1:
        if(is_led){
            this->led_label->setStyleSheet(m_green_SheetStyle);
        }
        this->status_message->setText(QString::fromStdString(status_vec[status]));
        break;
    case 2:
        this->status_message->setText(QString::fromStdString(status_vec[status]));
        break;
    case 3:
        this->status_message->setText(QString::fromStdString(status_vec[status]));
        break;
    default:
        this->status_message->setText("error");
        break;
    }
}
