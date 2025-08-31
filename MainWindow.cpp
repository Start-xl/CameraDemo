#include "MainWindow.h"
#include "./ui_MainWindow.h"
#include <QStandardItemModel>
#include <QDebug>
#include "ImageStatisticWgt.h"

const QString CSVFILESAVEPATH = "../../example/unexported.csv";
const QString SCREENSHOTPATH = "../../example/screenshot";

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
        ui->interfaceIP_Line->setEnabled(false);
        ui->interfaceSubnet_Line->setEnabled(false);
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

        ui->interfaceIP_Line->setText(m_deviceInfoList.pDevInfo[0].InterfaceInfo.gigeInterfaceInfo.ipAddress);
        ui->interfaceSubnet_Line->setText(m_deviceInfoList.pDevInfo[0].InterfaceInfo.gigeInterfaceInfo.subnetMask);
        ui->deviceIP_Line->setText(m_deviceInfoList.pDevInfo[0].DeviceSpecificInfo.gigeDeviceInfo.ipAddress);
        ui->deviceSubnet_Line->setText(m_deviceInfoList.pDevInfo[0].DeviceSpecificInfo.gigeDeviceInfo.subnetMask);

        ui->cameraWgt->SetCamera(m_deviceInfoList.pDevInfo[0].cameraKey);
    }
    ui->startGrab_Btn->setEnabled(false);
    ui->stopGrab_Btn->setEnabled(false);
    ui->closeDevice_Btn->setEnabled(false);
    ui->saveImage_Btn->setEnabled(false);
    ui->imageStatistic_Btn->setEnabled(false);
    ui->deviceProperties_Wgt->setVisible(false);
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 双击连接相机之后，显示设备信息
 * @date 2025-08-24
 */
void MainWindow::displayDeviceInfo(const QModelIndex &index) {
    ui->deviceInfo_Wgt->verticalHeader()->setVisible(true);
    ui->deviceInfo_Wgt->verticalHeader()->setFixedWidth(180);

    ui->deviceInfo_Wgt->horizontalHeader()->setVisible(false); // 设置水平表头不可见
    ui->deviceInfo_Wgt->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    ui->deviceInfo_Wgt->setEditTriggers(QAbstractItemView::NoEditTriggers); // 设置整个表格为只读

    // 获取相关设备信息
    deviceMacAddress = m_deviceInfoList.pDevInfo[index.row()].DeviceSpecificInfo.gigeDeviceInfo.macAddress;
    deviceIP = m_deviceInfoList.pDevInfo[index.row()].DeviceSpecificInfo.gigeDeviceInfo.ipAddress;
    deviceSubnetMask = m_deviceInfoList.pDevInfo[index.row()].DeviceSpecificInfo.gigeDeviceInfo.subnetMask;
    deviceGateway = m_deviceInfoList.pDevInfo[index.row()].DeviceSpecificInfo.gigeDeviceInfo.defaultGateWay;
    deviceVersion = m_deviceInfoList.pDevInfo[index.row()].deviceVersion;
    serialNum = m_deviceInfoList.pDevInfo[index.row()].serialNumber;
    protocolVersion = m_deviceInfoList.pDevInfo[index.row()].DeviceSpecificInfo.gigeDeviceInfo.protocolVersion;

    QTableWidgetItem* deviceMacAddressItem = new QTableWidgetItem(deviceMacAddress);
    ui->deviceInfo_Wgt->setItem(0, 0, deviceMacAddressItem);
    deviceMacAddressItem->setToolTip(deviceMacAddress);
    QTableWidgetItem* deviceIPItem = new QTableWidgetItem(deviceIP);
    ui->deviceInfo_Wgt->setItem(1, 0, deviceIPItem);
    deviceIPItem->setToolTip(deviceIP);
    QTableWidgetItem* deviceSubnetMaskItem = new QTableWidgetItem(deviceSubnetMask);
    ui->deviceInfo_Wgt->setItem(2, 0, deviceSubnetMaskItem);
    deviceSubnetMaskItem->setToolTip(deviceSubnetMask);
    QTableWidgetItem* deviceGatewayItem = new QTableWidgetItem(deviceGateway);
    ui->deviceInfo_Wgt->setItem(3, 0, deviceGatewayItem);
    deviceGatewayItem->setToolTip(deviceGateway);
    QTableWidgetItem* deviceVersionItem = new QTableWidgetItem(deviceVersion);
    ui->deviceInfo_Wgt->setItem(4, 0, deviceVersionItem);
    deviceVersionItem->setToolTip(deviceVersion);
    QTableWidgetItem* serialNumItem = new QTableWidgetItem(serialNum);
    ui->deviceInfo_Wgt->setItem(5, 0, serialNumItem);
    serialNumItem->setToolTip(serialNum);
    QTableWidgetItem* protocolVersionItem = new QTableWidgetItem(protocolVersion);
    ui->deviceInfo_Wgt->setItem(6, 0, protocolVersionItem);
    protocolVersionItem->setToolTip(protocolVersion);


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

    ui->startGrab_Btn->setEnabled(true);
    ui->closeDevice_Btn->setEnabled(true);
    ui->deviceProperties_Wgt->setVisible(true);
    ui->setIP_Btn->setEnabled(false);
    ui->deviceIP_Line->setEnabled(false);
    ui->deviceSubnet_Line->setEnabled(false);

    // 连接相机之后显示统计信息，所有值为0
    // Show statistics after connecting camera, all values are 0
    ui->cameraWgt->resetStatistic();
    QString strStatic = ui->cameraWgt->getStatistic();
    ui->statistic_Lab->setText(strStatic);

    displayDeviceInfo(index); // 显示设备信息
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
    ui->saveImage_Btn->setEnabled(true);
    ui->imageStatistic_Btn->setEnabled(true);

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

    QFile file(CSVFILESAVEPATH);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << QStringLiteral("无法打开文件：") << CSVFILESAVEPATH;
    }

    QTextStream in(&file);
    QString headerLine;
    if (!in.atEnd()) {
        headerLine = in.readLine();
    }
    file.close();
    if (headerLine.isEmpty()) {
        qWarning() << QStringLiteral("文件为空或没有表头");
    }
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << QStringLiteral("无法写入文件：") << CSVFILESAVEPATH;
    }

    QTextStream out(&file);
    out << headerLine << "\n";
    file.close();

    QWidget::closeEvent(event);
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 关闭设备
 * @date 2025-08-24
 */
void MainWindow::on_closeDevice_Btn_clicked() {
    on_stopGrab_Btn_clicked();

    ui->cameraWgt->CameraClose();
    ui->statistic_Lab->setText("");

    ui->closeDevice_Btn->setEnabled(false);
    ui->startGrab_Btn->setEnabled(false);
    ui->stopGrab_Btn->setEnabled(false);
    ui->saveImage_Btn->setEnabled(false);
    ui->imageStatistic_Btn->setEnabled(false);
    ui->deviceProperties_Wgt->setVisible(false);
    ui->setIP_Btn->setEnabled(true);
    ui->deviceIP_Line->setEnabled(true);
    ui->deviceSubnet_Line->setEnabled(true);
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 打开图像统计界面
 * @date 2025-08-27
 */
void MainWindow::on_imageStatistic_Btn_clicked() {
    imageStatistic = new ImageStatisticWgt(this);
    imageStatistic->show();
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 保存当前图像
 * @date 2025-08-31
 */
void MainWindow::on_saveImage_Btn_clicked() {
    QTimer* timer = new QTimer();
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [=]() {
        ui->saveFile_Lab->setText("");
    });

    if (ui->cameraWgt->saveSnapshotToDir(SCREENSHOTPATH)) {
        ui->saveFile_Lab->setText(QStringLiteral("图像保存成功!"));
        timer->start(1500);
    } else {
        ui->saveFile_Lab->setText(QStringLiteral("图像保存失败!"));
        timer->start(1500);
    }
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 修改设备IP
 * @date 2025-08-31
 */
void MainWindow::on_setIP_Btn_clicked() {
    QItemSelectionModel* selectionModel = ui->deviceModel_Tree->selectionModel();
    int currentIndex = selectionModel->currentIndex().row();
    ui->cameraWgt->SetCamera(m_deviceInfoList.pDevInfo[currentIndex].cameraKey);

    deviceIP = ui->deviceIP_Line->text();
    deviceSubnetMask = ui->deviceSubnet_Line->text();

    QTimer* timer = new QTimer();
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [=]() {
        ui->saveFile_Lab->setText("");
    });

    // bool success = ui->cameraWgt->setDeviceIP(deviceIP, deviceSubnetMask, "0.0.0.0");
    // if (success) {
    //     ui->saveFile_Lab->setText(QStringLiteral("设备IP设置成功!"));
    //     timer->start(1500);
    // } else {
    //     ui->saveFile_Lab->setText(QStringLiteral("设备与主机IP网段不匹配!"));
    //     timer->start(1500);
    // }
}

