#ifndef ARCHIVEANALYZER_H
#define ARCHIVEANALYZER_H

#include <QString>
#include <QObject>
#include <QByteArray>
#include "canrequest.h"
#include <QVector>
#include <QMutex>

class ArchiveAnalyzer:public QObject
{
    Q_OBJECT
    QMutex mutex;
    bool stopFlag;
    QString inpFileName;
    QByteArray rawData;
    CanRequest getNextRequest(const QByteArray &archive, QByteArray::const_iterator &it);
    QVector<CanRequest> reqs; 
public:
    static QString getHexByte(quint8 value);
    static QString getTwoHexByte(quint16 value);
    explicit ArchiveAnalyzer(const QString &fName, QObject *parent = nullptr);
signals:
    analyzeError(const QString &message);
    percentUpdate(int value);
    analyzeComplete();
public slots:
    void startAnalyze();
    void stopAnalyze();
};

#endif // ARCHIVEANALYZER_H
