#include "MainWindow.h"
#include "./ui_MainWindow.h"
#include <QStandardItemModel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(&m_staticTimer, &QTimer::timeout, this, &MainWindow::onTimerStreamStatistic);

    initUi();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initUi() {
    int ret = IMV_OK;

    ui->statistic_Lab->setText("");// 连接相机之前不显示状态栏 | Don't show status bar before connecting camera

    ret = IMV_EnumDevices(&m_deviceInfoList, interfaceTypeAll);
    if (IMV_OK != ret) {
        printf("Enumeration devices failed! ErrorCode[%d]\n", ret);
        return;
    }
    if (m_deviceInfoList.nDevNum < 1) {
        // ui->deviceModel_CBox->setEnabled(false);
        // ui->openDevice_Btn->setEnabled(false);
    } else {
        // ui->deviceModel_CBox->setEnabled(true);
        // ui->openDevice_Btn->setEnabled(true);

        // 初始化 model
        QStandardItemModel* model = new QStandardItemModel();
        // TreeView控件载入 model
        ui->deviceModel_Tree->setModel(model);
        ui->deviceModel_Tree->setHeaderHidden(true); // 隐藏表头
        ui->deviceModel_Tree->setEditTriggers(QAbstractItemView::NoEditTriggers); // 设置节点内容不可编辑

        for (unsigned int i = 0; i < m_deviceInfoList.nDevNum; i++) {
            deviceName = m_deviceInfoList.pDevInfo[i].cameraName;
            deviceIP = m_deviceInfoList.pDevInfo[i].DeviceSpecificInfo.gigeDeviceInfo.ipAddress;
            QStandardItem* rootItem = new QStandardItem(deviceName + " (" + deviceIP + ")");
            model->appendRow(rootItem);
        }

        ui->cameraWgt->SetCamera(m_deviceInfoList.pDevInfo[0].cameraKey);
    }
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 双击连接相机
 * @date 2025-08-23
 */
void MainWindow::on_deviceModel_Tree_doubleClicked(const QModelIndex &index) {
    // 设置要连接的相机
    ui->cameraWgt->SetCamera(m_deviceInfoList.pDevInfo[index.row()].cameraKey);

    if (!ui->cameraWgt->CameraOpen()) {
        return;
    }

    // 连接相机之后显示统计信息，所有值为0
    // Show statistics after connecting camera, all values are 0
    ui->cameraWgt->resetStatistic();
    QString strStatic = ui->cameraWgt->getStatistic();
    ui->statistic_Lab->setText(strStatic);
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 开始采集
 * @date 2025-08-10
 */
void MainWindow::on_startGrab_Btn_clicked() {
    // 设置连续拉流
    // set continue grabbing
    // ui->widget->CameraChangeTrig(CammerWidget::trigContinous);

    ui->cameraWgt->CameraStart();

    ui->startGrab_Btn->setEnabled(false);
    ui->stopGrab_Btn->setEnabled(true);

    ui->cameraWgt->resetStatistic();
    m_staticTimer.start(100);
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 停止采集
 * @date 2025-08-10
 */
void MainWindow::on_stopGrab_Btn_clicked() {
    m_staticTimer.stop();

    ui->cameraWgt->CameraStop();

    ui->startGrab_Btn->setEnabled(true);
    ui->stopGrab_Btn->setEnabled(false);
}

void MainWindow::onTimerStreamStatistic() {
    QString strStatic = ui->cameraWgt->getStatistic();
    ui->statistic_Lab->setText(strStatic);
}

void MainWindow::closeEvent(QCloseEvent * event) {
    on_stopGrab_Btn_clicked();
    ui->cameraWgt->CameraClose();
}
