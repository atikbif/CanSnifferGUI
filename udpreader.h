#ifndef UDPREADER_H
#define UDPREADER_H

#include <QObject>
#include <QMutex>
#include <QString>

class UDPReader : public QObject
{
    QMutex mutex;
    bool stopFlag;
    QString ipAddress;
    Q_OBJECT
    QByteArray createReadPageRequest(int pageNum);
    bool checkAnswer(char *data, int pageNum, int length);

    static const int PAGE_CNT = 8192;
    static const int PAGE_SIZE = 528;
    static const int DATA_OFFSET = 2;
    static const int TRY_LIM = 10;
    static const QString rawFile;
public:
    explicit UDPReader(QString ipAddress, QObject *parent = nullptr);

signals:
    void readComplete(const QString &fName);
    void readError(const QString &message);
    void percentUpdate(int value);
public slots:
    void startRead();
    void stopRead();
};

#endif // UDPREADER_H
