#include "canrequest.h"
#include <QDebug>

const std::array<QString,8> CanRequest::service = {
    "Channel",
    "Heartbeat",
    "Download",
    "Spare",
    "Write",
    "Read",
    "Action",
    "Event"
};

CanRequest::CanRequest(quint16 id, const QByteArray data):id(id),data(data)
{

}

int CanRequest::getSS() const
{
    if(data.count()) {
        return (quint8)data.at(0)>>5;
    }
    return 0;
}

int CanRequest::getEOID() const
{
    if(data.count()) {
        return (quint8)data.at(0)&0x1F;
    }
    return 0;
}

int CanRequest::getAddress() const
{
    if(data.count()>=2) {
        return (quint8)data.at(1);
    }
    return 0;
}

QByteArray CanRequest::getPC21Data() const
{
    if(data.count()>2) {
        return data.mid(2);
    }
    return QByteArray();
}

QString CanRequest::getService() const
{
    QString res = "";
    int serv_num = (quint8)id & 0x07;
    try {
        res = service.at(serv_num);
    }catch(std::out_of_range) {
        qDebug() << "out of range exception num:" << serv_num;
    }
    return res;
}


