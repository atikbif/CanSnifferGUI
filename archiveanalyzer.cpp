#include "archiveanalyzer.h"
#include <QDebug>
#include <QFile>
#include "algorithm"
#include "checksum.h"
#include <QTextStream>
#include <sstream>

CanRequest ArchiveAnalyzer::getNextRequest(const QByteArray &archive, QByteArray::const_iterator &it)
{
    QByteArray requestBody;
    auto endIt = archive.constEnd();
    auto startPos = std::find(it,endIt,0x31);
    it = startPos;
    int length=0;
    while(startPos!=endIt) {
        requestBody.clear();
        bool res = true;
        for(int i=0;i<3;i++) {
            it++;if(it!=endIt) requestBody.append(*it);else {res = false;break;}
        }
        if(res) {
            length = (quint8)requestBody.at(2);
            if(length<=8) {
                for(int i=0;i<length;i++) {
                    it++;if(it!=endIt) requestBody.append(*it);else {res = false;break;}
                }
            }else res = false;
            if(res) {

                auto crc = CheckSum::getCRC8(requestBody);
                it++;if(it!=endIt) {
                    if(crc==(unsigned char)(*it)) {
                        it++;
                        if((it!=endIt) && (*it==0x31)) {
                            // correct request
                            int id = ((quint16)((quint8)requestBody.at(0))<<8) | (quint8)requestBody.at(1);
                            return CanRequest(id,requestBody.mid(3,length));
                        }else res = false;
                    }else {res = false;}
                }else res = false;
            }
        }
        startPos++;
        if(startPos!=endIt) startPos = std::find(startPos,endIt,0x31);
        it = startPos;
    }
    return CanRequest();
}

QString ArchiveAnalyzer::getHexByte(quint8 value)
{
    return QString("0x%1").arg(value, 2, 16, QChar('0'));
}

ArchiveAnalyzer::ArchiveAnalyzer(const QString &fName, QObject *parent):QObject(parent), inpFileName(fName)
{
    stopFlag = false;
}

void ArchiveAnalyzer::startAnalyze()
{
    QFile inFile(inpFileName);
    if(inFile.open(QIODevice::ReadOnly)) {
        rawData = inFile.readAll();
        auto it = rawData.constBegin();
        auto req = getNextRequest(rawData,it);
        reqs.clear();
        while(it!=rawData.constEnd()) {
            reqs.append(req);
            req = getNextRequest(rawData,it);
        }
        qDebug() << "request count: " << reqs.count();
        inFile.close();
        QFile outFile("log_can.txt");
        if(outFile.open(QIODevice::WriteOnly)) {
            int lastPercent = 0;
            QTextStream stream(&outFile );
            int cnt = 0;
            bool exit = false;
            for(auto req:reqs) {
                if(cnt%1000==0) {
                    mutex.lock();
                    exit = stopFlag;
                    stopFlag = false;
                    mutex.unlock();
                }

                if(exit) {if(QFile::exists(outFile.fileName())) outFile.remove();outFile.close();return;}
                cnt++;
                stream << "ID:" << getHexByte(req.getID()) << "  DIR:" << getHexByte(req.getDir()) << "  SRV:";
                stream.setFieldWidth(10); stream.setFieldAlignment(QTextStream::AlignLeft);
                stream << req.getService();
                stream.setFieldWidth(0);
                stream << "  SS:" << getHexByte(req.getSS()) << "  EOID:" << getHexByte(req.getEOID()) <<
                          "  ADDR:" << getHexByte(req.getAddress()) << "  Data: ";
                for(auto value:req.getPC21Data()) stream << getHexByte(value) << " ";
                stream << endl;

                int percent = (double)cnt*100/reqs.count();
                if(percent!=lastPercent) {
                    emit percentUpdate(percent);
                }
                lastPercent = percent;
            }
            outFile.close();
            emit analyzeComplete();
        }else emit analyzeError("ошибка открытия файла " + outFile.fileName());
    }else emit analyzeError("ошибка открытия файла " + inFile.fileName());
}

void ArchiveAnalyzer::stopAnalyze()
{
    mutex.lock();
    stopFlag = true;
    mutex.unlock();
}
