#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "IMVAPI/IMVApi.h"
#include "ImageStatisticWgt.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void initUi();

    void displayDeviceInfo(const QModelIndex &index);

private slots:
    void onTimerStreamStatistic();

    void on_startGrab_Btn_clicked();

    void on_stopGrab_Btn_clicked();

    void closeEvent(QCloseEvent *event);

    void on_deviceModel_Tree_doubleClicked(const QModelIndex &index);

    void on_closeDevice_Btn_clicked();

    void on_imageStatistic_Btn_clicked();

    void on_saveImage_Btn_clicked();

    void on_setIP_Btn_clicked();

    void on_deviceModel_Tree_clicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui;

    IMV_DeviceList m_deviceInfoList;	// 发现的相机列表 | List of cameras found
    QTimer m_staticTimer;				// 定时器，定时刷新状态栏信息 | Timer, refresh status bar information regularly
    QString deviceName;
    QString deviceIP;

    QString deviceMacAddress;           // 物理地址
    QString deviceSubnetMask;           // 子网掩码
    QString deviceGateway;              // 网关
    QString deviceVersion;              // 设备版本
    QString serialNum;                  // 序列号
    QString protocolVersion;            // 协议版本

    int currentIndex;                   // 当前点击树项目的index

    ImageStatisticWgt* imageStatistic;
};
#endif // MAINWINDOW_H
