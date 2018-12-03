#include "mainwindow.h"
#include "ui_mainwindow.h"

quint32 userID = 0x22180000;
quint32 devID = 0;
char fileID[14];

void sleep(quint32 msec)
{
    QTime reachTime = QTime::currentTime().addMSecs(msec);

    while(QTime::currentTime() < reachTime)
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->progressBar->setValue(0);
    ui->lineEdit_3->setText("30");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    mp_clientSocket = new QTcpSocket();
    mp_clientSocket->connectToHost(ui->lineEdit->text(), ui->lineEdit_2->text().toInt(), QIODevice::ReadWrite);

    connect(mp_clientSocket, SIGNAL(readyRead()), this, SLOT(ClientRecvData()));
}

void MainWindow::on_pushButton_2_clicked()
{
    QDateTime current_date_time = QDateTime::currentDateTime();
    QString current_date = current_date_time.toString("yyyy MM dd hh mm ss");
    current_date.remove(" ");

    strcpy(fileID, current_date.toLatin1());
    send_packet_0x09(0x09, userID, devID, fileID);

    ui->progressBar->setValue(0);
    ui->textBrowser->append("sent <0x09>");
}

void MainWindow::on_pushButton_3_clicked()
{
    ui->textBrowser->clear();
    ui->progressBar->setValue(0);
}

void MainWindow::ClientRecvData()
{
    QByteArray recvBuf = mp_clientSocket->readAll();
    quint8 gotACK;

    if(!recvBuf.isEmpty())
    {
        if(recvBuf[0] == 0xEB && recvBuf[1] == 0x90 && recvBuf[2] == 0x08 && recvBuf[26] == 0)
        {
            gotACK = recvBuf[3];
            if(gotACK == 0x09)
            {
                ui->textBrowser->append("got <0x09>");
                send_packet_0x05(0x05, userID, devID, fileID);
                ui->textBrowser->append("sent <0x05>");
            }
            else if(gotACK == 0x05)
            {
                ui->textBrowser->append("got <0x05>");
                send_packet_ecgdata(ui->lineEdit_3->text().toUInt());
            }
            else if(gotACK == 0x0B)
            {
                ui->textBrowser->append("got <0x0B>");
                ui->textBrowser->append("sent data successfully!");
            }
            else
            {
                ui->textBrowser->append("recv ack err!");
            }
        }
        else
        {
            ui->textBrowser->append("recv packet err!");
        }
    }
}

void MainWindow::send_packet_0x09(quint8 order, quint32 userid, quint32 devid, const char *fileid)
{
    quint8 Generic_Data[128] = {0};
    quint32 i, len = 22;
    quint16 checksum = 0;

    Generic_Data[0] = 0xEB;
    Generic_Data[1] = 0x90;
    Generic_Data[2] = 0x08;
    Generic_Data[3] = order;
    Generic_Data[4] = (len + 10) >> 24;
    Generic_Data[5] = (len + 10) >> 16;
    Generic_Data[6] = (len + 10) >> 8;
    Generic_Data[7] = (len + 10);

    Generic_Data[8] = userid;
    Generic_Data[9] = userid >> 8;
    Generic_Data[10] = userid >> 16;
    Generic_Data[11] = userid >> 24;

    Generic_Data[12] = devid;
    Generic_Data[13] = devid >> 8;
    Generic_Data[14] = devid >> 16;
    Generic_Data[15] = devid >> 24;

    strcpy((char *)&Generic_Data[16], fileid);

    //计算校验码，不包含包头
    for (i = 2; i < len + 8; ++i)
    {
        checksum += Generic_Data[i];
    }

    Generic_Data[len + 8] = (checksum >> 8);
    Generic_Data[len + 9] = (checksum);

    mp_clientSocket->write((const char *)Generic_Data, len + 10);
}

void MainWindow::send_packet_0x05(quint8 order, quint32 userid, quint32 devid, const char *fileid)
{
    quint8 Generic_Data[128] = {0};
    quint32 i, len = 44, seconds = ui->lineEdit_3->text().toUInt();
    quint16 checksum = 0;

    Generic_Data[0] = 0xEB;
    Generic_Data[1] = 0x90;
    Generic_Data[2] = 0x08;
    Generic_Data[3] = order;
    Generic_Data[4] = (len) >> 24;
    Generic_Data[5] = (len) >> 16;
    Generic_Data[6] = (len) >> 8;
    Generic_Data[7] = (len);

    Generic_Data[8] = userid;
    Generic_Data[9] = userid >> 8;
    Generic_Data[10] = userid >> 16;
    Generic_Data[11] = userid >> 24;

    Generic_Data[12] = devid;
    Generic_Data[13] = devid >> 8;
    Generic_Data[14] = devid >> 16;
    Generic_Data[15] = devid >> 24;

    Generic_Data[16] = 11;
    Generic_Data[17] = 37;

    Generic_Data[18] = 1;
    Generic_Data[19] = 1;
    Generic_Data[20] = 1;
    Generic_Data[21] = 0;
    Generic_Data[22] = 0;
    Generic_Data[23] = (quint8)(813 >> 8);
    Generic_Data[24] = (quint8)(813);

    Generic_Data[25] = seconds >> 16;
    Generic_Data[26] = seconds >> 8;
    Generic_Data[27] = seconds;

    strcpy((char *)&Generic_Data[28], fileid);

    //计算校验码，不包含包头
    for (i = 2; i < len - 2; ++i)
    {
        checksum += Generic_Data[i];
    }

    Generic_Data[len - 2] = (checksum >> 8);
    Generic_Data[len - 1] = (checksum);

    mp_clientSocket->write((const char *)Generic_Data, len);
}

void MainWindow::send_packet_ecgdata(quint32 seconds)
{
    quint16 packetlen = 1360;
    quint32 totdatabytes = seconds * 2000 * 17;
    quint32 totbytes = totdatabytes + 30 + 2;
    quint32 totpacket = totdatabytes / packetlen + 2;
    quint16 sum = 0;
    quint8 sendBuf[100] = {0};
    quint8 ecgdata[65536] = {0};
    char tmpbuf[100] = {0};

    if(seconds != 10 && seconds != 15 && seconds != 30)
    {
        ui->textBrowser->append("input err!");
        return;
    }

    for(quint32 i = 0; i < totpacket; i++)
    {
        if(i == 0)
        {
            sendBuf[0] = 0xEB;
            sendBuf[1] = 0x90;
            sendBuf[2] = 0x08;
            sendBuf[3] = 0x0B;

            sendBuf[4] = totbytes >> 24;
            sendBuf[5] = totbytes >> 16;
            sendBuf[6] = totbytes >> 8;
            sendBuf[7] = totbytes;

            sendBuf[8] = userID;
            sendBuf[9] = userID >> 8;
            sendBuf[10] = userID >> 16;
            sendBuf[11] = userID >> 24;

            sendBuf[12] = devID;
            sendBuf[13] = devID >> 8;
            sendBuf[14] = devID >> 16;
            sendBuf[15] = devID >> 24;

            strcpy((char *)&sendBuf[16], fileID);

            for(quint32 j = 2; j < 30; j++)
            {
                sum += sendBuf[j];
            }
            mp_clientSocket->write((const char *)sendBuf, 30);

            sprintf(tmpbuf, "sent ecgdata head. total packet=%d", totpacket - 2);
            ui->textBrowser->append(tmpbuf);
        }
        else if(i == totpacket - 1)
        {
            sendBuf[0] = sum >> 8;
            sendBuf[1] = sum;
            mp_clientSocket->write((const char *)sendBuf, 2);
        }
        else
        {
            mp_clientSocket->write((const char *)ecgdata, packetlen);

            sprintf(tmpbuf, "sent mid(%d)", i);
            ui->textBrowser->append(tmpbuf);
        }

        ui->progressBar->setValue((i + 1) * 100 / totpacket);
        sleep(20);
    }
}
