#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QString>
#include <QRegExp>
#include <QTextCodec>
#include <QMessageBox>
#include <QProcess>
#include "udpcontroller.h"
#include "bootloadercontroller.h"
#include <QDateTime>
#include <QFileDialog>

//#include "archiveanalyzer.h"


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowFlags(Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);
    connect(ui->lineEditIP,SIGNAL(returnPressed()),ui->pushButtonRead,SLOT(click()));

    //ArchiveAnalyzer analyzer("raw_can_01.bin");
    //analyzer.startAnalyze();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::deletePingProcess()
{
    QProcess* pingProcess= dynamic_cast<QProcess*>(sender());
    if(pingProcess) {
        QTextCodec *codec = QTextCodec::codecForName("CP866");
        QString p_stdout = codec->toUnicode(pingProcess->readAllStandardOutput());
        QString p_stderr = codec->toUnicode(pingProcess->readAllStandardError());
        QMessageBox::information(this,"PING",p_stdout + "\n" + p_stderr);
        pingProcess->deleteLater();
    }
}

void MainWindow::on_pushButtonRead_clicked()
{
    UDPController udp(ui->lineEditIP->text());
    emit udp.readArchive();
    udp.exec();
}

void MainWindow::on_pushButtonReadConf_clicked()
{
    UDPReader reader(ui->lineEditIP->text());
    auto resData = reader.readConf();

    if(resData.count()>=18) {
        quint32 timeSec = (quint8)resData.at(0);
        timeSec <<= 8;
        timeSec |= (quint8)resData.at(1);
        timeSec <<= 8;
        timeSec |= (quint8)resData.at(2);
        timeSec <<= 8;
        timeSec |= (quint8)resData.at(3);
        QDateTime startDate(QDate(2000, 1, 1), QTime(0, 0, 0));
        startDate = startDate.addSecs(timeSec);

        ui->lineEditTime->setText(startDate.toString("dd.MM.yyyy  HH:mm:ss"));

        QString id;
        for(int i=0;i<12;i++) id+=QString("%1").arg(resData.at(i+4), 2, 16, QChar('0'));
        ui->lineEditID->setText(id);

        quint8 versionHigh = resData.at(16);
        quint8 versionLow = resData.at(17);
        ui->lineEditVersion->setText(QString::number(versionHigh)+"."+QString::number(versionLow));
        ui->statusBar->showMessage("Настройки успешно считаны",2000);
    }else if(resData.count()==0) ui->statusBar->showMessage("Устройство не отвечает",2000);
    else if(resData.at(0)=='1') ui->statusBar->showMessage("Устройство в режиме загрузчика.\nНеобходимо загрузить программу.",2000);
    else if(resData.at(0)=='2') ui->statusBar->showMessage("Неизвестный тип устройства.",2000);
}

void MainWindow::on_pushButtonSyncTime_clicked()
{
    ui->statusBar->showMessage("Синхронизация времени с компьютером",2000);
    UDPReader reader(ui->lineEditIP->text());
    reader.writeTime(QDateTime::currentDateTime());
    on_pushButtonReadConf_clicked();
}

void MainWindow::on_pushButtonDownload_clicked()
{
    auto reply = QMessageBox::question(this, "Загрузка ПО", "Загрузка приведёт к стиранию текущей программы.\nПродолжить?",
                                    QMessageBox::Yes|QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        QString fName = QFileDialog::getOpenFileName(this, "Open File",
                                                        "",
                                                        "Файл прошивки (*.bin)");
        if(!fName.isEmpty()) {
            BootloaderController boot(ui->lineEditIP->text());
            emit boot.load(fName);
            boot.exec();
        }
    }

}
