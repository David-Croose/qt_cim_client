#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QDateTime>
#include <QTime>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    QTcpSocket *mp_clientSocket;

private slots:
    void on_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_3_clicked();

    void ClientRecvData();

    void send_packet_0x09(quint8 order, quint32 userid, quint32 devid, const char *fileid);

    void send_packet_0x05(quint8 order, quint32 userid, quint32 devid, const char *fileid);

    void send_packet_ecgdata(quint32 seconds);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
