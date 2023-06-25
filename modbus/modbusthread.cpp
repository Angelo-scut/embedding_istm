#include "modbusthread.h"

ModbusThread::ModbusThread()
{
    init();

}
void ModbusThread::init(){
    this->s = -1;
    this->ctx = NULL;
    this->mb_mapping = NULL;
    this->rc = -1;

    //初始化modbus rtu how to decide the device default
    this->ctx = modbus_new_rtu("/dev/ttysWK0", 115200, 'N', 8, 1);  // create a modbus struct, para:modbus_t *modbus_new_rtu(const *device, int baud, char parity, int data_bit, int stop_bit)
    //设定从设备地址
    modbus_set_slave(this->ctx, 1);
    //modbus连接
    modbus_connect(this->ctx);

    //寄存器map初始化
    mb_mapping = modbus_mapping_new(MODBUS_MAX_READ_BITS, 0,
                                    MODBUS_MAX_READ_REGISTERS, 0);
    if (mb_mapping == NULL) {
        fprintf(stderr, "Failed to allocate the mapping: %s\n",
                modbus_strerror(errno));
        modbus_free(this->ctx);
        //return -1;
        return;
    }
//    for(uint i=0; i<MODBUS_MAX_RW_WRITE_REGISTERS; i++){
//        this->mb_mapping->tab_registers[i] = 0;
//    }
   //初始几个寄存器
    // 0 no action 1 open camera and start detect; 4 stop detect
    this->mb_mapping->tab_registers[0] = 0;
    this->mb_mapping->tab_registers[1] = 380;  // current
    this->mb_mapping->tab_registers[2] = 0;  // voltage
    this->mb_mapping->tab_registers[3] = 260;  // welding speed
    this->mb_mapping->tab_registers[4] = 0;  // wire speed
    this->mb_mapping->tab_registers[5] = 0;  // welding angle

    this->mb_mapping->tab_registers[10] = 0;  // warning
    this->mb_mapping->tab_registers[11] = 0;  // deviation
    this->mb_mapping->tab_registers[12] = 0;  // seam width
    this->mb_mapping->tab_registers[13] = 0;  // entrance area
    this->mb_mapping->tab_registers[14] = 0;  // entrance length
    this->mb_mapping->tab_registers[15] = 0;  // entrance width
}

void ModbusThread::close(){
    modbus_close(this->ctx);
    modbus_mapping_free(this->mb_mapping);
    modbus_free(this->ctx);
}

void ModbusThread::run()
{
    //循环
    while( 1 ){
//        qDebug() << "haha" << endl;
         uint8_t query[MODBUS_TCP_MAX_ADU_LENGTH];

         //轮询接收数据，并做相应处理
         this->rc = modbus_receive(this->ctx, query);
         if (this->rc > 0) {
             modbus_reply(this->ctx, query, this->rc, this->mb_mapping);
//             qDebug() << "receive sucess" << this->mb_mapping->tab_registers[0]
//                      << this->mb_mapping->tab_registers[1]
//                      << this->mb_mapping->tab_registers[2]
//                      << this->mb_mapping->tab_registers[3]
//                      << this->mb_mapping->tab_registers[4]
//                      << this->mb_mapping->tab_registers[5]<< endl;
             this->mb_mapping->tab_registers[10] = warning;
             this->isConnect = true;

         } else if (this->rc  == -1) {
             /* Connection closed by the client or error */
//             qDebug() << "connect error" << endl;
//             qDebug() << "reconnecting" << endl;
             modbus_close(this->ctx);
             modbus_connect(this->ctx);
//             this->close();
//             this->init();
             this->isConnect = false;
//             break;
         }
         else
         {
//              qDebug() << "receive none" << endl;
             this->isConnect = true;
         }
     }
}
