#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "IMVAPI/IMVApi.h"

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

private slots:
    void onTimerStreamStatistic();

    void on_startGrab_Btn_clicked();

    void on_stopGrab_Btn_clicked();

    void closeEvent(QCloseEvent *event);

    void on_deviceModel_Tree_doubleClicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui;

    IMV_DeviceList m_deviceInfoList;	// 发现的相机列表 | List of cameras found
    QTimer m_staticTimer;				// 定时器，定时刷新状态栏信息 | Timer, refresh status bar information regularly
    QString deviceName;
    QString deviceIP;
};
#endif // MAINWINDOW_H
