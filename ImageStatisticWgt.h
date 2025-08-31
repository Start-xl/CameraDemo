#ifndef IMAGESTATISTICWGT_H
#define IMAGESTATISTICWGT_H

#include <QWidget>
#include <QStandardItemModel>
#include <QTableView>

namespace Ui {
class ImageStatisticWgt;
}

class ImageStatisticWgt : public QWidget
{
    Q_OBJECT

public:
    explicit ImageStatisticWgt(QWidget *parent = nullptr);
    ~ImageStatisticWgt();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void open();

    void nextImage();

    void prevImage();

    void on_saveImage_Btn_clicked();

    void on_addRow_Btn_clicked();

    void on_exportFile_Btn_clicked();

    void on_openFile_Btn_clicked();

    void on_saveFile_Btn_clicked();

private:
    Ui::ImageStatisticWgt *ui;

    QStandardItemModel* model;
    QString currentCsvFilePath;

    QStringList fileList;           // 图片列表
    QString currentDir;             // 打开的图片所在路径
    int currentIndex;               // 当前显示的图片在图片列表中的索引

    void loadImage(const QString &fileName);
    void copyCurrentImage(const QString &targetDir);

    bool exportTableViewToCsv(QTableView* tableView, const QString &filePath, const QString &separator = ",");
    QStringList parseCsvLine(const QString &line);
    void displayStatusBar(const QString &statusText, const int displayTime);
    void loadCsvToTableView(const QString &filePath);
};

#endif // IMAGESTATISTICWGT_H
