#include "bootloader.h"
#include <QUdpSocket>
#include "checksum.h"
#include <QFile>
#include <QByteArray>
#include <QThread>

QByteArray BootLoader::createErasePageRequest(int pageNum)
{
    QByteArray res;
    res.append('\0');
    res.append(pageNum);
    res.append(0xE8);
    res.append(pageNum);
    int crc = CheckSum::getCRC16(res);
    res.append(crc & 0xFF);
    res.append(crc >> 8);
    return res;
}

QByteArray BootLoader::createWriteFlashRequest(quint32 addr, quint16 cnt, QByteArray::ConstIterator it)
{
    QByteArray res;
    static quint16 id = 0;
    res.append(id>>8);
    res.append(id&0xFF);
    res.append(0xE9);
    res.append((addr>>24)&0xFF);
    res.append((addr>>16)&0xFF);
    res.append((addr>>8)&0xFF);
    res.append(addr&0xFF);
    res.append(cnt>>8);
    res.append(cnt&0xFF);
    for (quint16 i=0;i<cnt;i++) {
        res.append(*it);
        it++;
    }
    int crc = CheckSum::getCRC16(res);
    res.append(crc & 0xFF);
    res.append(crc >> 8);
    id++;
    return res;
}

QByteArray BootLoader::createJumpToAppRequest()
{
    QByteArray res;
    static quint16 id = 0;
    res.append(id>>8);
    res.append(id&0xFF);
    res.append(0xEA);
    int crc = CheckSum::getCRC16(res);
    res.append(crc & 0xFF);
    res.append(crc >> 8);
    id++;
    return res;
}

QByteArray BootLoader::createResetRequest()
{
    QByteArray res;
    static quint16 id = 0;
    res.append(id>>8);
    res.append(id&0xFF);
    res.append(0xEF);
    int crc = CheckSum::getCRC16(res);
    res.append(crc & 0xFF);
    res.append(crc >> 8);
    id++;
    return res;
}

QByteArray BootLoader::createReadTypeRequest()
{
    QByteArray res;
    static quint16 id = 0;
    res.append(id>>8);
    res.append(id&0xFF);
    res.append(0xA0);
    int crc = CheckSum::getCRC16(res);
    res.append(crc & 0xFF);
    res.append(crc >> 8);
    id++;
    return res;
}

bool BootLoader::checkCRCAnswer(char *data, int length)
{
    if(CheckSum::getCRC16(data,length)==0) {
        return true;
    }
    return false;
}

BootLoader::BootLoader(QString ipAddress, QObject *parent) : QObject(parent)
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

void BootLoader::load(const QString &fName)
{
    QFile binFile(fName);
    if(binFile.open(QIODevice::ReadOnly)) {
        QByteArray data = binFile.readAll();
        while(data.count()%4) data.append(0xFF); // размер должен быть кратным 4 байтам
        const quint32 sectSize = 128*1024;
        int sectCnt = data.count()%sectSize?data.count()/sectSize+1:data.count()/sectSize;
        if(sectCnt>7) {
            sectCnt  = 7;
            data.resize(sectSize*sectCnt);
        }
        sectCnt++;
        double percWriteStep = data.count()%512?100.0/(data.count()/512+1):100.0/(data.count()/512);
        // стирание Flash
        const unsigned char startSectNum=4;
        unsigned char endSectnum = startSectNum + sectCnt - 1;
        QUdpSocket udp;
        unsigned char try_num = 0;
        char receiveBuf[1024];
        udp.connectToHost(QHostAddress(ipAddress),7);
        udp.open(QIODevice::ReadWrite);
        udp.readAll();
        double percEraseStep = 100.0/(endSectnum-startSectNum+1);
        double percent = 0;
        bool exit = false;
        emit info("Переход в режим загрузчика...");
        auto request = createErasePageRequest(4);
        udp.write(request);
        if(udp.waitForReadyRead(2000)) {
            request = createResetRequest();
            udp.write(request);
            QThread::msleep(1000);
        }
        try_num=0;
        while(try_num<TRY_LIM) {
            mutex.lock();
            exit = stopFlag;
            mutex.unlock();
            if(exit) return;
            udp.readAll();
            auto request = createReadTypeRequest();
            udp.write(request);
            if(udp.waitForReadyRead(100)) {
                int cnt = 0;
                while(udp.hasPendingDatagrams()) {
                    cnt = udp.readDatagram(receiveBuf,sizeof(receiveBuf));
                }
                if(cnt>0) {
                    if(checkCRCAnswer(receiveBuf,cnt)) {
                        if(((quint8)receiveBuf[0]!=0x00)||((quint8)receiveBuf[1]!=0xDA)||((quint8)receiveBuf[2]!=0x83)||((quint8)receiveBuf[3]!=0x2F)) {
                            emit loadError("Не удалось перейти в режим загрузчика");
                            return;
                        }
                        break;
                    }
                    else try_num++;
                }
            }else try_num++;
            QThread::msleep(10);
        }
        if(try_num==TRY_LIM) {
            emit loadError("Устройство не отвечает");
            return;
        }

        emit info("Стирание ...");
        for(auto sectNum=startSectNum;sectNum<=endSectnum;sectNum++){
            try_num=0;
            while(try_num<TRY_LIM) {
                mutex.lock();
                exit = stopFlag;
                mutex.unlock();
                if(exit) return;
                udp.readAll();
                auto request = createErasePageRequest(sectNum);
                udp.write(request);
                if(udp.waitForReadyRead(2000)) {
                    int cnt = 0;
                    while(udp.hasPendingDatagrams()) {
                        cnt = udp.readDatagram(receiveBuf,sizeof(receiveBuf));
                    }
                    if(cnt>0) {
                        if(checkCRCAnswer(receiveBuf,cnt)) {
                            percent += percEraseStep;
                            emit percentUpdate(percent);
                            break;
                        }
                        else try_num++;
                    }
                }else try_num++;
                QThread::msleep(10);
            }
            if(try_num==TRY_LIM) {
                emit loadError("Устройство не отвечает");
                return;
            }
        }
        emit info("Загрузка ...");
        percent = 0;
        emit percentUpdate(percent);

        auto endIt = data.constEnd();
        auto curIt = data.constBegin();
        quint32 addr = 0;
        auto dist=std::distance(curIt,endIt);
        while(dist!=0) {
            quint16 length = 512;
            if(dist<512) length = dist;
            try_num=0;
            while(try_num<TRY_LIM) {
                mutex.lock();
                exit = stopFlag;
                mutex.unlock();
                if(exit) return;
                udp.readAll();
                auto request = createWriteFlashRequest(addr,length,curIt);
                udp.write(request);
                if(udp.waitForReadyRead(100)) {
                    int cnt = 0;
                    while(udp.hasPendingDatagrams()) {
                        cnt = udp.readDatagram(receiveBuf,sizeof(receiveBuf));
                    }
                    if(cnt>0) {
                        if(checkCRCAnswer(receiveBuf,cnt)) {
                            percent += percWriteStep;
                            emit percentUpdate(percent);
                            break;
                        }
                        else try_num++;
                    }
                }else try_num++;
                QThread::msleep(10);
            }
            if(try_num==TRY_LIM) {
                emit loadError("Устройство не отвечает");
                return;
            }
            curIt+=length;
            addr+=length;
            dist=std::distance(curIt,endIt);
        }

        QThread::msleep(100);
        // jump to application

        try_num=0;
        while(try_num<TRY_LIM) {
            mutex.lock();
            exit = stopFlag;
            mutex.unlock();
            if(exit) return;
            udp.readAll();
            auto request = createJumpToAppRequest();
            udp.write(request);
            if(udp.waitForReadyRead(100)) {
                int cnt = 0;
                while(udp.hasPendingDatagrams()) {
                    cnt = udp.readDatagram(receiveBuf,sizeof(receiveBuf));
                }
                if(cnt>0) {
                    if(checkCRCAnswer(receiveBuf,cnt)) {
                        emit loadComplete();
                        return;
                    }
                    else try_num++;
                }
            }else try_num++;
            QThread::msleep(10);
        }
        if(try_num==TRY_LIM) {
            emit loadError("Неудачная попытка запуска приложения");
            return;
        }

    }else {
        emit loadError("Ошибка открытия файла " + fName);
    }
}

void BootLoader::stopLoad()
{
    mutex.lock();
    stopFlag = true;
    mutex.unlock();
}
