#ifndef IMAGESTATISTICWGT_H
#define IMAGESTATISTICWGT_H

#include <QWidget>
#include <QStandardItemModel>

namespace Ui {
class ImageStatisticWgt;
}

class ImageStatisticWgt : public QWidget
{
    Q_OBJECT

public:
    explicit ImageStatisticWgt(QWidget *parent = nullptr);
    ~ImageStatisticWgt();

private:
    Ui::ImageStatisticWgt *ui;

    QStandardItemModel* model;
};

#endif // IMAGESTATISTICWGT_H
