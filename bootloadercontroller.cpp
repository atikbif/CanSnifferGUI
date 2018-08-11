#include "bootloadercontroller.h"
#include <QDebug>
#include <QVBoxLayout>

BootloaderController::BootloaderController(QString ipAddress, QWidget *parent): QDialog(parent),ipAddress(ipAddress)
{
    setWindowTitle("Обновление программы");
    QVBoxLayout *layout = new QVBoxLayout();
    bar = new QProgressBar();
    bar->setMaximum(100);
    bar->setMinimum(0);
    bar->setValue(0);
    layout->addWidget(bar);
    message = new QLabel("");
    layout->addWidget(message);
    setLayout(layout);

    loader = new BootLoader(ipAddress);
    loader->moveToThread(&loaderThread);

    connect(&loaderThread, &QThread::finished, loader, &QObject::deleteLater);
    connect(this, &BootloaderController::load, loader, &BootLoader::load);
    connect(loader, &BootLoader::loadComplete, this, &BootloaderController::loadComplete);
    connect(loader, &BootLoader::loadError, this, &BootloaderController::loadError);
    connect(loader, &BootLoader::info, this, &BootloaderController::info);
    connect(loader, &BootLoader::percentUpdate, this, &BootloaderController::percentUpdate);
    loaderThread.start();
}

BootloaderController::~BootloaderController()
{
    loader->stopLoad();
    loaderThread.quit();
    loaderThread.wait();
}

void BootloaderController::loadComplete()
{
    accept();
}

void BootloaderController::loadError(const QString &message)
{
    qDebug() << message;
    this->message->setText(message);
}

void BootloaderController::info(const QString &message)
{
    this->message->setText(message);
}

void BootloaderController::percentUpdate(int value)
{
    bar->setValue(value);
}
