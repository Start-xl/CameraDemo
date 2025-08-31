#include "ImageStatisticWgt.h"
#include "ui_ImageStatisticWgt.h"
#include <QDebug>
#include <QDir>
#include <QDate>
#include <QImageReader>
#include <QFileDialog>
#include <QTimer>

const QString SCREENSHOTPATH = "../../example/screenshot";
const QString SCREENSHOTSAVEPATH = "../../data/";
const QString CSVFILEPATH = "../../example/unexported.csv";

ImageStatisticWgt::ImageStatisticWgt(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ImageStatisticWgt)
    , currentIndex(-1)
    , currentCsvFilePath(CSVFILEPATH)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window);
    setWindowTitle(QStringLiteral("图像统计"));

    ui->imageName_Lab->setAlignment(Qt::AlignCenter);
    ui->csvFileName_Lab->setAlignment(Qt::AlignCenter);
    ui->csvFileName_Lab->setText("unexported.csv");

    connect(ui->viewImage_Btn, &QPushButton::clicked, this, &ImageStatisticWgt::open);
    connect(ui->previous_Btn, &QPushButton::clicked, this, &ImageStatisticWgt::prevImage);
    connect(ui->next_Btn, &QPushButton::clicked, this, &ImageStatisticWgt::nextImage);

    QDate date = QDate::currentDate();
    QString folderPath = SCREENSHOTSAVEPATH + date.toString("yyyy-MM-dd");
    bool isExists = QDir(folderPath).exists();
    if (!isExists) {
        QDir().mkpath(folderPath);
    }

    QDir dir(SCREENSHOTPATH);
    if (dir.exists()) {
        QStringList filters = {"*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif"};
        QStringList files = dir.entryList(filters, QDir::Files);
        if (!files.isEmpty()) {
            currentDir = dir.absolutePath();
            fileList = files;
            currentIndex = 0;
            loadImage(currentDir + "/" + fileList[currentIndex]);
            ui->imageName_Lab->setText(fileList[currentIndex]);
        }
    }

    loadCsvToTableView(currentCsvFilePath);
}

ImageStatisticWgt::~ImageStatisticWgt()
{
    delete ui;
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 关闭图像统计界面删除截图
 * @date 2025-08-28
 */
void ImageStatisticWgt::closeEvent(QCloseEvent *event) {
    QDir dir(SCREENSHOTPATH);
    QStringList filters = {"*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif"};
    QStringList files = dir.entryList(filters, QDir::Files);

    for (QString &f : files) {
        QFile::remove(dir.absoluteFilePath(f));
    }
    QWidget::closeEvent(event);
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 打开截图保存路径下的图片
 * @date 2025-08-29
 */
void ImageStatisticWgt::open() {
    QDate date = QDate::currentDate();
    QString folderPath = SCREENSHOTSAVEPATH + date.toString("yyyy-MM-dd");

    QString filePath = QFileDialog::getOpenFileName(this, QStringLiteral("打开图片"),
                                                    folderPath,
                                                    "Images (*.png *.jpg *.jpeg *.bmp *.gif)");
    QString fileName = QFileInfo(filePath).fileName();
    ui->imageName_Lab->setText(fileName);
    if (!filePath.isEmpty()) {
        loadImage(filePath);

        QDir dir = QFileInfo(filePath).absoluteDir();
        QStringList filters = {"*.png", "*.jpg", "*.jpeg", "*.bmp", "*.gif"};
        fileList = dir.entryList(filters, QDir::Files);
        currentDir = dir.absolutePath();
        currentIndex = fileList.indexOf(QFileInfo(filePath).fileName());
    }
}

void ImageStatisticWgt::nextImage() {
    if (fileList.isEmpty() || currentIndex < 0) {
        return;
    }

    if (currentIndex < fileList.size() - 1) {
        currentIndex++;
        loadImage(currentDir + "/" + fileList[currentIndex]);
        ui->imageName_Lab->setText(fileList[currentIndex]);
    }
}

void ImageStatisticWgt::prevImage() {
    if (fileList.isEmpty() || currentIndex < 0) {
        return;
    }

    if (currentIndex > 0) {
        currentIndex--;
        loadImage(currentDir + "/" + fileList[currentIndex]);
        ui->imageName_Lab->setText(fileList[currentIndex]);
    }
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 加载图片
 * @date 2025-08-28
 */
void ImageStatisticWgt::loadImage(const QString &fileName) {
    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    QImage newImage = reader.read();
    if (!newImage.isNull()) {
        ui->imageWgt->setImage(newImage);
    }
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 保存有用截图
 * @date 2025-08-29
 */
void ImageStatisticWgt::copyCurrentImage(const QString &targetDir) {
    if (fileList.isEmpty() || currentIndex < 0) {
        displayStatusBar(QStringLiteral("当前没有显示图片!"), 1500);
        return;
    }

    QDir dir(targetDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QString currentFile = fileList[currentIndex];
    QString srcPath = currentDir + "/" + currentFile;

    QFileInfo fileInfo(currentFile);
    QString baseName = fileInfo.completeBaseName();
    QString suffix = fileInfo.suffix();

    QString dstPath = dir.absoluteFilePath(currentFile);
    int counter = 1;
    while (QFile::exists(dstPath)) {
        QString newName = QString("%1_%2.%3").arg(baseName).arg(counter).arg(suffix);
        dstPath = dir.absoluteFilePath(newName);
        counter++;
    }

    if (QFile::copy(srcPath, dstPath)) {
        displayStatusBar(QStringLiteral("图片保存成功!"), 1500);
    } else {
        displayStatusBar(QStringLiteral("图片保存失败!"), 1500);
    }
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 将表格中数据导出到csv文件
 * @date 2025-08-29
 */
bool ImageStatisticWgt::exportTableViewToCsv(QTableView *tableView, const QString &filePath, const QString &separator) {
    if (!ui->statisticTable || !ui->statisticTable->model()) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    QAbstractItemModel* model = ui->statisticTable->model();

    QStringList headerList;
    for (int col = 0; col < model->columnCount(); ++col) {
        headerList << model->headerData(col, Qt::Horizontal).toString();
    }
    out << headerList.join(separator) << "\n";

    for (int row = 0; row < model->rowCount(); ++row) {
        QStringList rowList;
        for (int col = 0; col < model->columnCount(); ++col) {
            QString data = model->data(model->index(row, col)).toString();

            if (data.contains(separator) || data.contains("\"")) {
                data.replace("\"", "\"\"");
                data = "\"" + data + "\"";
            }
            rowList << data;
        }
        out << rowList.join(separator) << "\n";
    }
    file.close();
    return true;
}

QStringList ImageStatisticWgt::parseCsvLine(const QString &line) {
    QStringList fields;
    QString field;
    bool inQuotes = false;

    for (int i = 0; i < line.length(); ++i) {
        QChar c = line.at(i);

        if (c == '\"') {
            inQuotes = !inQuotes;
        } else if (c == ',' && !inQuotes) {
            fields << field;
            field.clear();
        } else {
            field.append(c);
        }
    }
    fields << field;

    for (QString &f : fields) {
        if (f.startsWith('"') && f.endsWith('"')) {
            f = f.mid(1, f.length() - 2);
        }
    }
    return fields;
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 状态栏显示
 * @date 2025-08-29
 */
void ImageStatisticWgt::displayStatusBar(const QString &statusText, const int displayTime) {
    QTimer* timer = new QTimer();
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [=]() {
        ui->statisticBar_Lab->setText("");
    });

    ui->statisticBar_Lab->setText(statusText);
    timer->start(displayTime);
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 将csv文件内容加载到界面上
 * @date 2025-08-29
 */
void ImageStatisticWgt::loadCsvToTableView(const QString &filePath) {
    QString fileName = QFileInfo(filePath).fileName();
    ui->csvFileName_Lab->setText(fileName);

    if (!filePath.isEmpty()) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            displayStatusBar(QStringLiteral("Warning: 无法打开文件: ") + file.errorString(), 1500);
        } else {
            model = qobject_cast<QStandardItemModel*>(ui->statisticTable->model());
            model = new QStandardItemModel(0, 0, this);
            ui->statisticTable->setModel(model);
            QTextStream in(&file);
            model = qobject_cast<QStandardItemModel*>(ui->statisticTable->model());
            int row = 0;
            while (!in.atEnd()) {
                QString line = in.readLine();
                if (line.trimmed().isEmpty()) {
                    continue;
                }

                QStringList fields = parseCsvLine(line);

                if (row == 0) {
                    model->setColumnCount(fields.size());
                    for (int col = 0; col < fields.size(); ++col) {
                        model->setHeaderData(col, Qt::Horizontal, fields[col]);
                    }
                } else {
                    QList<QStandardItem*> items;
                    for (const QString & field : fields) {
                        items << new QStandardItem(field);
                    }
                    model->appendRow(items);
                }
                row++;
            }
            ui->statisticTable->setModel(model);
            file.close();
        }
    }
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 将有用截图保存到其他路径
 * @date 2025-08-29
 */
void ImageStatisticWgt::on_saveImage_Btn_clicked() {
    QDate date = QDate::currentDate();
    QString saveImagePath = SCREENSHOTSAVEPATH + date.toString("yyyy-MM-dd");
    copyCurrentImage(saveImagePath);
}

void ImageStatisticWgt::on_addRow_Btn_clicked() {
    int row = model->rowCount();
    model->insertRow(row);
}

void ImageStatisticWgt::on_exportFile_Btn_clicked() {
    if (!ui->statisticTable || !ui->statisticTable->model()) {

    } else {
        QDate date = QDate::currentDate();

        QString filePath = QFileDialog::getSaveFileName(parentWidget(), QStringLiteral("导出CSV文件"),
                                                        SCREENSHOTSAVEPATH + "/" + date.toString("yyyy-MM-dd") + ".csv",
                                                        QStringLiteral("CSV文件 (*.csv);;所有文件 (*.*)"));
        if (!filePath.isEmpty()) {
            bool isSuccess = exportTableViewToCsv(ui->statisticTable, filePath);
            if (isSuccess) {
                displayStatusBar(QStringLiteral("文件导出成功!"), 1500);
            } else {
                displayStatusBar(QStringLiteral("文件导出失败!"), 1500);
            }
        }
    }
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 打开csv文件
 * @date 2025-08-30
 */
void ImageStatisticWgt::on_openFile_Btn_clicked() {
    QDate date = QDate::currentDate();
    QString filePath = QFileDialog::getOpenFileName(parentWidget(), QStringLiteral("选择文件"),
                                                    SCREENSHOTSAVEPATH + "/" + date.toString("yyyy-MM-dd") + ".csv",
                                                    QStringLiteral("CSV文件 (*.csv);;所有文件 (*.*)"));
    if (!filePath.isEmpty()) {
        currentCsvFilePath = filePath;
        loadCsvToTableView(filePath);
        displayStatusBar(QStringLiteral("文件打开成功!"), 1500);
    }
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 保存csv文件
 * @date 2025-08-30
 */
void ImageStatisticWgt::on_saveFile_Btn_clicked() {
    QFile file(currentCsvFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.close();
    }
    exportTableViewToCsv(ui->statisticTable, currentCsvFilePath);
    displayStatusBar(QStringLiteral("文件保存成功!"), 1500);
}

