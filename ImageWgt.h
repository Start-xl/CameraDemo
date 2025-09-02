#ifndef IMAGEWGT_H
#define IMAGEWGT_H

#include <QWidget>
#include <QImage>
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>

class ImageWgt : public QWidget
{
    Q_OBJECT
public:
    explicit ImageWgt(QWidget *parent = nullptr);

    void setImage(const QImage &img);
    void zoom(double factor);
    void resetZoom();
    void fitToWidget();

protected:
    void paintEvent(QPaintEvent*) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    QImage image;
    double scaleFactor;
    QPoint offset;
    bool dragging;
    QPoint lastPos;

    void clampOffset();
};

#endif // IMAGEWGT_H
