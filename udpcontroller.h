#ifndef UDPCONTROLLER_H
#define UDPCONTROLLER_H

#include <QDialog>
#include <QThread>
#include <QLabel>
#include <QProgressBar>
#include "udpreader.h"
#include "archiveanalyzer.h"

class UDPController: public QDialog
{
    Q_OBJECT
    QThread readerThread, analyzeThread;
    QProgressBar *bar;
    QLabel *message;
    QString ipAddress;

    UDPReader *reader;
    ArchiveAnalyzer *analyzer;
public:
    explicit UDPController(QString ipAddress, QWidget *parent = 0);
    ~UDPController();
public slots:
    void readComplete(const QString &fName);
    void analyzeComplete();
    void readError(const QString &message);
    void percentUpdate(int value);
signals:
    void readArchive();
    void analyze();
};


#endif // UDPCONTROLLER_H
