#ifndef MODBUSTHREAD_H
#define MODBUSTHREAD_H

#include <QThread>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <modbus.h>
#include <iostream>
#include <QDebug>

using namespace std;



class ModbusThread: public QThread
{
public:
    ModbusThread();
    void init();
    void close();
    void run();
public:
    int s;
    modbus_t *ctx;
    modbus_mapping_t *mb_mapping;
    int rc;
    bool isConnect = false;
    int warning = 0;
};

#endif // MODBUSTHREAD_H
