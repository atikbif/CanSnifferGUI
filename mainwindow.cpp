#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QString>
#include <QRegExp>
#include <QTextCodec>
#include <QMessageBox>
#include <QProcess>
#include "udpcontroller.h"
#include <QDateTime>

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
        ui->label_version->setText(QString::number(versionHigh)+"."+QString::number(versionLow));
    }else QMessageBox::information(this, "Can Viewer","Устройство не отвечает");


}

void MainWindow::on_pushButtonSyncTime_clicked()
{
    UDPReader reader(ui->lineEditIP->text());
    reader.writeTime(QDateTime::currentDateTime());
    on_pushButtonReadConf_clicked();
}
