#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QString>
#include <QRegExp>
#include <QTextCodec>
#include <QMessageBox>
#include <QProcess>
#include "udpcontroller.h"

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
    udp.exec();
}
