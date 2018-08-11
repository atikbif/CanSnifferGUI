#ifndef BOOTLOADERCONTROLLER_H
#define BOOTLOADERCONTROLLER_H

#include <QDialog>
#include <QThread>
#include <QLabel>
#include <QProgressBar>
#include "bootloader.h"

class BootloaderController : public QDialog
{
    QThread loaderThread;
    QProgressBar *bar;
    QLabel *message;
    QString ipAddress;
    BootLoader *loader;
    Q_OBJECT
public:
    explicit BootloaderController(QString ipAddress, QWidget *parent = 0);
    ~BootloaderController();

public slots:
    void loadComplete();
    void loadError(const QString &message);
    void info(const QString &message);
    void percentUpdate(int value);
signals:
    void load(const QString &fName);
};

#endif // BOOTLOADERCONTROLLER_H
