#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork>
#include <QUdpSocket>
#include "qcustomplot.h" 

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
    void processPendingDatagrams();

    void sendDatagram();

    void on_newFileButton_released();

    void on_SkipSet_currentIndexChanged(int index);

    void on_BinsSet_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
    QUdpSocket *udpSocket;
    QByteArray *popsCMD;
};

#endif // MAINWINDOW_H
