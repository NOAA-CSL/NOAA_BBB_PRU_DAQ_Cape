#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtNetwork>
#include <QUdpSocket>
#include <stdlib.h>
#include <QVector>
#include <QString>
#include <QStringList>
#include "qcustomplot.h" 

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("POPS Data");

    udpSocket = new QUdpSocket();
    udpSocket->bind(8000);

    popsCMD = new QByteArray();

    connect(udpSocket, SIGNAL(readyRead()),
            this, SLOT(processPendingDatagrams()));
    connect(ui->SkipSet, SIGNAL(currentIndexChanged(int)),
            this, SLOT(on_SkipSet_currentIndexChanged(int)));
    connect(ui->BinsSet, SIGNAL(currentIndexChanged(int)),
            this, SLOT(on_BinsSet_currentIndexChanged(int)));
    connect(ui->newFileButton, SIGNAL(released()),
            this, SLOT(on_newFileButton_released()));


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_newFileButton_released()
{
    popsCMD->clear();
    popsCMD->append("NewFile=1");
    sendDatagram();
}

void MainWindow::on_SkipSet_currentIndexChanged(int skip)
{
    popsCMD->clear();
    switch(skip)
    {
        case 0:
        {
            popsCMD->append("Skip=0 ");
            break;
        }
        case 1:
        {
            popsCMD->append("Skip=9 ");
            break;
        }
        case 2:
        {
            popsCMD->append("Skip=99 ");
            break;
        }
        default:
        {
            popsCMD->append("Skip=9 ");
            break;
        }
    }
    sendDatagram();
}

void MainWindow::on_BinsSet_currentIndexChanged(int bins)
{
    popsCMD->clear();
    switch(bins)
    {
        case 0:
        {
            popsCMD->append("nbins=16 ");
            break;
        }
        case 1:
        {
            popsCMD->append("nbins=25 ");
            break;
        }
        case 2:
        {
            popsCMD->append("nbins=100 ");
            break;
        }
        case 3:
        {
            popsCMD->append("nbins=200 ");
            break;
        }
        default:
        {
            popsCMD->append("nbins=25");
            break;
        }
    }
    sendDatagram();
}

void MainWindow::processPendingDatagrams()
{
    QByteArray datagram;
    datagram.resize(udpSocket->pendingDatagramSize());
    udpSocket->readDatagram(datagram.data(), datagram.size());

    QString Data = QString::fromUtf8(datagram);
    if(!Data.compare("STOP")) system("sudo shutdown -h now") ;
    else
    {
        QStringList list = Data.split(',');
        ui->statusLabel->setText(list[0]);
        ui->PartCon->setText(list[1]);
        ui->FlowRate->setText(list[2]);
        ui->Press->setText(list[3]);
        ui->TempC->setText(list[4]);
        ui->Skip->setText(list[5]);
        ui->Bins->setText(list[6]);
        bool ok;
        QString s = list[6];
        double nBins = s.toDouble(&ok);
        int iBins = s.toInt(&ok);
        s = list[7];
        double logmin = s.toDouble(&ok);
        s = list[8];
        double logmax = s.toDouble(&ok);

        double delw = (logmax-logmin)/(nBins+1.);
        double maxy = 0.;
        QVector<double> xbins(iBins+1),ybins(iBins+1);
        for (int i=0; i< iBins; i++)
        {
             s = list[i+9];
             ybins[i] = s.toDouble();
             if(ybins[i]>maxy)maxy=ybins[i];
             xbins[i] =logmin+((double)i)*delw;
        }
        ybins[iBins]=ybins[iBins-1];
        xbins[iBins]=logmin+nBins*delw;
        // Set up y axis range
        QVector<double> yrmax;
        yrmax << 50 << 100 << 150 << 250 << 500 << 750 << 1000 << 1500 \
              << 3000 << 5000 << 7500 << 10000 << 15000 << 20000;

        int j=0;
        while (maxy>yrmax[j])
        {
           j++;
        }

        ui->hPlot->addGraph();
        ui->hPlot->graph(0)->setPen(QPen(QColor(0,0,102)));
        ui->hPlot->graph(0)->setBrush(QBrush(QColor(51,153,255)));
        ui->hPlot->graph(0)->setLineStyle(QCPGraph::lsStepLeft);
        ui->hPlot->graph(0)->setData(xbins,ybins);
        ui->hPlot->xAxis->setRange(1.4,5.0);
        ui->hPlot->yAxis->setRange(0,yrmax[j]);

        ui->hPlot->addGraph();
        ui->hPlot->graph(1)->setLineStyle(QCPGraph::lsImpulse);
        ui->hPlot->graph(1)->setPen(QPen(QColor(0,0,102)));
        ui->hPlot->graph(1)->setData(xbins,ybins);

        ui->hPlot->xAxis->setBasePen(QPen(Qt::white));
        ui->hPlot->yAxis->setBasePen(QPen(Qt::white));
        ui->hPlot->xAxis->setTicks(true);
        ui->hPlot->yAxis->setTicks(true);
        ui->hPlot->xAxis->setTickLabels(true);
        ui->hPlot->yAxis->setTickLabels(true);
        ui->hPlot->xAxis->setTickPen(QPen(Qt::white));
        ui->hPlot->yAxis->setTickPen(QPen(Qt::white));
        ui->hPlot->xAxis->setTickLabelColor(Qt::white);
        ui->hPlot->yAxis->setTickLabelColor(Qt::white);
        ui->hPlot->xAxis->setSubTickPen(QPen(Qt::white));
        ui->hPlot->xAxis->setLabelColor(Qt::white);
        ui->hPlot->xAxis->setLabel("log10 peak");
        ui->hPlot->xAxis->grid()->setPen(QPen(QColor(102,102,255),1,Qt::DotLine));
        ui->hPlot->xAxis->grid()->setSubGridPen(QPen(QColor(80,80,255),1,Qt::DotLine));
        ui->hPlot->xAxis->grid()->setSubGridVisible(true);
        ui->hPlot->yAxis->setSubTickPen(QPen(Qt::white));
        ui->hPlot->yAxis->grid()->setPen(QPen(QColor(102,102,255),1,Qt::DotLine));
        ui->hPlot->yAxis->grid()->setSubGridPen(QPen(QColor(80,80,255),1,Qt::DotLine));
        ui->hPlot->yAxis->grid()->setSubGridVisible(true);
        ui->hPlot->axisRect()->setupFullAxesBox();
        ui->hPlot->xAxis2->setTickPen(QPen(Qt::white));
        ui->hPlot->yAxis2->setTickPen(QPen(Qt::white));
        ui->hPlot->xAxis2->setTickLabelColor(Qt::white);
        ui->hPlot->yAxis2->setTickLabelColor(Qt::white);
        ui->hPlot->xAxis2->setSubTickPen(QPen(Qt::white));
        ui->hPlot->xAxis2->setBasePen(QPen(Qt::white));
        ui->hPlot->yAxis2->setBasePen(QPen(Qt::white));
        ui->hPlot->setBackground(QBrush(QColor(0,0,102)));
        ui->hPlot->replot();

    }
}

void MainWindow::sendDatagram()
{

    udpSocket->writeDatagram(popsCMD->data(), popsCMD->size(),
                             QHostAddress("10.1.1.3"), 8001);
}
