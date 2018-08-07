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

QByteArray UDPReader::createReadConfRequest()
{
    QByteArray res;
    res.append('\0');
    res.append(0x01);
    res.append((char)0xD1);
    int crc = CheckSum::getCRC16(res);
    res.append(crc & 0xFF);
    res.append(crc >> 8);
    return res;
}

QByteArray UDPReader::createWriteTimeRequest(const QDateTime &dt)
{
    QByteArray res;
    res.append('\0');
    res.append(0x01);
    res.append((char)0xE1);
    res.append(dt.time().second());
    res.append(dt.time().minute());
    res.append(dt.time().hour());
    res.append(dt.date().day());
    res.append(dt.date().month());
    int year = dt.date().year();
    if(year<2000) year = 0;else year-=2000;
    res.append(year);
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

bool UDPReader::checkCRCAnswer(char *data, int length)
{
    if(CheckSum::getCRC16(data,length)==0) {
        return true;
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

QByteArray UDPReader::readConf()
{
    QByteArray resData;
    QUdpSocket udp;
    unsigned char try_num = 0;
    char receiveBuf[1024];
    udp.connectToHost(QHostAddress(ipAddress),7);
    udp.open(QIODevice::ReadWrite);
    udp.readAll();
    while(try_num<TRY_LIM) {
        udp.readAll();
        auto request = createReadConfRequest();
        udp.write(request);
        if(udp.waitForReadyRead(100)) {
            int cnt = 0;
            while(udp.hasPendingDatagrams()) {
                cnt = udp.readDatagram(receiveBuf,sizeof(receiveBuf));
            }
            if(cnt>0) {
                if(checkCRCAnswer(receiveBuf,cnt)) {
                    for(int i=0;i<4;i++) resData.append(receiveBuf[3+i]); // time
                    for(int i=0;i<12;i++) resData.append(receiveBuf[7+i]); // id
                    resData.append(receiveBuf[19]); // version
                    resData.append(receiveBuf[20]);
                    return resData;
                }
                else try_num++;
            }
        }else try_num++;
    }
    if(try_num==TRY_LIM) {
        emit readError("Устройство не отвечает");
    }
    return resData;
}

void UDPReader::writeTime(const QDateTime &dt)
{
    QUdpSocket udp;
    udp.connectToHost(QHostAddress(ipAddress),7);
    udp.open(QIODevice::ReadWrite);
    auto request = createWriteTimeRequest(dt);
    udp.write(request);
    QThread::sleep(1);
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
