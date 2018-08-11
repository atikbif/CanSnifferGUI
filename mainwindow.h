#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void deletePingProcess();
    void on_pushButtonRead_clicked();

    void on_pushButtonReadConf_clicked();

    void on_pushButtonSyncTime_clicked();

    void on_pushButtonDownload_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
