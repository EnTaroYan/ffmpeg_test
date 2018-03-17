#include <QCoreApplication>
#include <QDebug>
#include <stdlib.h>
#include "encodeandsend.h"

void Delay_MSec(uint64_t msec);
int WriteBufferCam1(void *opaque, uint8_t *buf,int buf_size);
int WriteBufferCam2(void *opaque, uint8_t *buf,int buf_size);

EncodeAndSend *Cam1;
EncodeAndSend *Cam2;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    int ret;

    Cam1=new EncodeAndSend("/dev/video0","192.168.0.1",1234,WriteBufferCam1);
    //Cam2=new EncodeAndSend("/dev/video1","192.168.1.115",1235,WriteBufferCam2);

    ret=Cam1->InitAll();
    if(ret<0)
    {
        qDebug()<<"Cam1 init failed";
        return -1;
    }

//    ret=Cam2->InitAll();
//    if(ret<0)
//    {
//        qDebug()<<"Cam2 init failed";
//        return -1;
//    }

    ret=Cam1->WriteHeader();
    if(ret<0)
    {
        qDebug()<<"Cam1 write header failed";
        return -1;
    }

//    ret=Cam2->WriteHeader();
//    if(ret<0)
//    {
//        qDebug()<<"Cam2 write header failed";
//        return -1;
//    }

    while(1)
    {

        ret=Cam1->ReadFrame();
        if(ret<0)
        {
            qDebug()<<"Cam1 read frame failed";
            return -1;
        }

//        ret=Cam2->ReadFrame();
//        if(ret<0)
//        {
//            qDebug()<<"Cam2 read frame failed";
//            return -1;
//        }

        ret=Cam1->EncodeSend();
        if(ret<0)
        {
            qDebug()<<"Cam1 encode or send failed";
            return -1;
        }

        //        ret=Cam2->EncodeSend();
        //        if(ret<0)
        //        {
        //            qDebug()<<"Cam2 encode or send failed";
        //            return -1;
        //        }
    }
    Cam1->FreeAll();
    Cam2->FreeAll();
    return a.exec();
}

int WriteBufferCam1(void *opaque, uint8_t *buf,int buf_size)
{
    Cam1->pUdpSocket->writeDatagram((const char *)buf,buf_size,*(Cam1->ptargetIp),Cam1->targetPort);
    return 1;
}

int WriteBufferCam2(void *opaque, uint8_t *buf,int buf_size)
{
    Cam2->pUdpSocket->writeDatagram((const char *)buf,buf_size,*(Cam2->ptargetIp),Cam2->targetPort);
    return 1;
}

void Delay_MSec(uint64_t msec)
{
    QTime _Timer=QTime::currentTime().addMSecs(msec);
    while(QTime::currentTime()<_Timer)
        QCoreApplication::processEvents(QEventLoop::AllEvents,100);
}
