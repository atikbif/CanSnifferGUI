#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <QObject>
#include <QMutex>
#include <QString>

class BootLoader : public QObject
{
    QMutex mutex;
    bool stopFlag;
    QString ipAddress;
    static const int TRY_LIM = 10;

    QByteArray createErasePageRequest(int pageNum);
    QByteArray createWriteFlashRequest(quint32 addr, quint16 cnt, QByteArray::ConstIterator it);
    QByteArray createJumpToAppRequest();
    QByteArray createResetRequest();
    QByteArray createReadTypeRequest();
    bool checkCRCAnswer(char *data, int length);

    Q_OBJECT
public:
    explicit BootLoader(QString ipAddress, QObject *parent = nullptr);

signals:
    void loadComplete();
    void loadError(const QString &message);
    void info(const QString &message);
    void percentUpdate(int value);
public slots:
    void load(const QString &fName);
    void stopLoad();
};

#endif // BOOTLOADER_H
