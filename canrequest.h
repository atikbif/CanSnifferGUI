#ifndef CANREQUEST_H
#define CANREQUEST_H

#include <QByteArray>
#include <QString>
#include <array>

class CanRequest
{
    quint16 id;
    quint32 reqTime;
    QByteArray data;
    static const std::array<QString,8> service;
    static const std::array<QString,8> ssDefinition;
    static QString getIOType(int num);
public:
    CanRequest(quint16 id=0, quint32 reqTime=0, const QByteArray data=QByteArray());
    quint16 getID() const {return id;}
    int getDir() const {return (id >> 10) &0x01;}
    int getSS() const;
    int getEOID() const;
    int getAddress() const;
    QByteArray getPC21Data() const;
    QString getService() const;
    QByteArray getData() const {return data;}
    QString getComment() const;
    QString getTime() const;
    quint32 getIntTime() const {return reqTime;}
};

#endif // CANREQUEST_H
