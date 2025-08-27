#include "ImageStatisticWgt.h"
#include "ui_ImageStatisticWgt.h"

ImageStatisticWgt::ImageStatisticWgt(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ImageStatisticWgt)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window);
    setWindowTitle(QStringLiteral("图像统计"));

    model = qobject_cast<QStandardItemModel*>(ui->statisticTable->model());
    model = new QStandardItemModel(10, 5, this);

    model->setHeaderData(0, Qt::Horizontal, QStringLiteral("批次号"));
    model->setHeaderData(1, Qt::Horizontal, QStringLiteral("测试图片"));
    model->setHeaderData(2, Qt::Horizontal, QStringLiteral("坏点数"));
    model->setHeaderData(3, Qt::Horizontal, QStringLiteral("坏点位置"));
    model->setHeaderData(4, Qt::Horizontal, QStringLiteral("备注"));

    ui->statisticTable->setModel(model);
}

ImageStatisticWgt::~ImageStatisticWgt()
{
    delete ui;
}
