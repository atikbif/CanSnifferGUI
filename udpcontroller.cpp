#include "udpcontroller.h"
#include "udpreader.h"
#include <QDebug>
#include <QVBoxLayout>
#include "archiveanalyzer.h"

UDPController::UDPController(QString ipAddress, QWidget *parent) : QDialog(parent),ipAddress(ipAddress)
{
    setWindowTitle("Чтение архива контроллера");
    //setFixedSize(300,50);
    QVBoxLayout *layout = new QVBoxLayout();
    bar = new QProgressBar();
    bar->setMaximum(100);
    bar->setMinimum(0);
    bar->setValue(0);
    layout->addWidget(bar);
    message = new QLabel("");
    layout->addWidget(message);
    setLayout(layout);

    reader = new UDPReader(ipAddress);
    reader->moveToThread(&readerThread);

    analyzer = nullptr;

    connect(&readerThread, &QThread::finished, reader, &QObject::deleteLater);
    connect(this, &UDPController::readArchive, reader, &UDPReader::startRead);
    connect(reader, &UDPReader::readComplete, this, &UDPController::readComplete);
    connect(reader, &UDPReader::readError, this, &UDPController::readError);
    connect(reader, &UDPReader::percentUpdate, this, &UDPController::percentUpdate);
    readerThread.start();
    //emit readArchive();
}

UDPController::~UDPController()
{
    reader->stopRead();
    readerThread.quit();
    readerThread.wait();

    if(analyzer) {
        analyzer->stopAnalyze();
        analyzeThread.quit();
        analyzeThread.wait();
    }
}

void UDPController::readComplete(const QString &fName)
{
    //qDebug() << fName;
    message->setText("Обработка данных");
    bar->setValue(0);

    analyzer = new ArchiveAnalyzer(fName);
    analyzer->moveToThread(&analyzeThread);
    connect(&analyzeThread, &QThread::finished, analyzer, &QObject::deleteLater);
    connect(this, &UDPController::analyze, analyzer, &ArchiveAnalyzer::startAnalyze);
    connect(analyzer, &ArchiveAnalyzer::analyzeComplete, this, &UDPController::analyzeComplete);
    connect(analyzer, &ArchiveAnalyzer::analyzeError, this, &UDPController::readError);
    connect(analyzer, &ArchiveAnalyzer::percentUpdate, this, &UDPController::percentUpdate);
    analyzeThread.start();
    emit analyze();
}

void UDPController::analyzeComplete()
{
    accept();
}

void UDPController::readError(const QString &message)
{
    qDebug() << message;
    this->message->setText(message);
}

void UDPController::percentUpdate(int value)
{
    bar->setValue(value);
}

