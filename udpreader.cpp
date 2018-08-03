#include "udpreader.h"
#include <QThread>
#include <QUdpSocket>
#include "checksum.h"
#include <QFile>

const QString UDPReader::rawFile =  "raw_can.bin";

QByteArray UDPReader::createReadPageRequest(int pageNum)
{
    QByteArray res;
    res.append(pageNum>>8);
    res.append(pageNum&0xFF);
    res.append((char)0xD0);
    res.append(pageNum>>8);
    res.append(pageNum&0xFF);
    int crc = CheckSum::getCRC16(res);
    res.append(crc & 0xFF);
    res.append(crc >> 8);
    return res;
}

bool UDPReader::checkAnswer(char *data, int pageNum, int length)
{
    if(CheckSum::getCRC16(data,length)==0) {
        int id = ((int)((quint8)data[0]) << 8) | (quint8)data[1];
        if(id==pageNum) return true;
    }
    return false;
}

UDPReader::UDPReader(QString ipAddress, QObject *parent) : QObject(parent)
{
    stopFlag = false;
    QRegExp exp("^(\\d{1,3})\\.(\\d{1,3}).(\\d{1,3}).(\\d{1,3})");
    if(exp.indexIn(ipAddress)!=-1) {
        this->ipAddress = QString::number(exp.cap(1).toInt()) +
            "." + QString::number(exp.cap(2).toInt()) +
            "." + QString::number(exp.cap(3).toInt()) +
            "." + QString::number(exp.cap(4).toInt());
    }else this->ipAddress="";
}

void UDPReader::startRead()
{
    QByteArray archive;
    archive.reserve((long)PAGE_CNT*PAGE_SIZE);
    QUdpSocket udp;
    udp.connectToHost(QHostAddress(ipAddress),7);
    double percent = 0;
    double step = 100.0/PAGE_CNT;
    char receiveBuf[1024];
    unsigned char try_num = 0;
    int res=0;
    udp.open(QIODevice::ReadWrite);
    bool exit = false;
    for(int i=0;i<PAGE_CNT;i++){
        mutex.lock();
        exit = stopFlag;
        mutex.unlock();
        if(exit) return;
        try_num=0;
        while(try_num<TRY_LIM) {
            udp.readAll();
            auto request = createReadPageRequest(i);
            udp.write(request);
            if(udp.waitForReadyRead(100)) {
                while(udp.hasPendingDatagrams()) {
                    res = udp.readDatagram(receiveBuf,sizeof(receiveBuf));
                }
                if(res>0) {
                    if(checkAnswer(receiveBuf,i,res)) {
                        archive.insert(i*PAGE_SIZE,&receiveBuf[DATA_OFFSET],PAGE_SIZE);
                        break;
                    }
                    else try_num++;
                }
            }else try_num++;
        }
        if(try_num==TRY_LIM) {
            emit readError("Устройство не отвечает");
            return;
        }

        percent+=step;
        QThread::msleep(1);
        emit percentUpdate(percent);
    }

    // запись архива в файл

    QFile file(rawFile);
    if(file.open(QIODevice::WriteOnly)) {
        file.write(archive);
        file.close();
        emit readComplete(rawFile);
    }else emit readError("Ошибка при сохранении файла");
}

void UDPReader::stopRead()
{
    mutex.lock();
    stopFlag = true;
    mutex.unlock();
}
