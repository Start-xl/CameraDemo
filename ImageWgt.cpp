#include "ImageWgt.h"

ImageWgt::ImageWgt(QWidget *parent)
    : QWidget{parent}
    , scaleFactor(1.0)
    , dragging(false)
{}

void ImageWgt::setImage(const QImage &img) {
    image = img;
    resetZoom();
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 实现图片缩放
 * @date 2025-08-28
 */
void ImageWgt::zoom(double factor) {
    scaleFactor *= factor;
    clampOffset();
    update();
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 恢复图片原始尺寸
 * @date 2025-08-28
 */
void ImageWgt::resetZoom() {
    scaleFactor = 1.0;
    offset = QPoint(0, 0);
    update();
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 渲染图片
 * @date 2025-08-28
 */
void ImageWgt::paintEvent(QPaintEvent *) {
    QPainter painter(this);

    if (!image.isNull()) {
        QSize imgSize = image.size() * scaleFactor;

        QPoint topLeft;
        if (imgSize.width() <= width() && imgSize.height() <= height()) {
            topLeft = QPoint((width() - imgSize.width()) / 2,
                             (height() - imgSize.height()) / 2);
        } else {
            topLeft = offset;
        }
        painter.drawImage(QRect(topLeft, imgSize), image);
    }
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 实现鼠标滚轮缩放
 * @date 2025-08-28
 */
void ImageWgt::wheelEvent(QWheelEvent *event) {
    if (event->angleDelta().y() > 0) {
        zoom(1.25);
    } else {
        zoom(0.8);
    }
}

void ImageWgt::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        dragging = true;
        lastPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
    } else if (event->button() == Qt::RightButton) { // 点击右键恢复图片原始尺寸
        resetZoom();
    }
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 实现鼠标按住拖动
 * @date 2025-08-28
 */
void ImageWgt::mouseMoveEvent(QMouseEvent *event) {
    if (dragging && !image.isNull()) {
        QPoint delta = event->pos() - lastPos;
        lastPos = event->pos();
        offset += delta;
        clampOffset();
        update();
    }
}

void ImageWgt::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        dragging = false;
        setCursor(Qt::ArrowCursor);
    }
}

/**
 * @author xl-1/4
 * @version 1.0
 * @brief TODO 限制拖动范围
 * @date 2025-08-28
 */
void ImageWgt::clampOffset() {
    if (image.isNull()) {
        return;
    }

    QSize imgSize = image.size() * scaleFactor;

    if (imgSize.width() <= width() && imgSize.height() <= height()) {
        offset = QPoint((width() - imgSize.width()) / 2,
                        (height() - imgSize.height()) / 2);
    } else {
        int minX = width() - imgSize.width();
        int minY = height() - imgSize.height();
        int maxX = 0;
        int maxY = 0;

        offset.setX(std::min(std::max(offset.x(), minX), maxX));
        offset.setY(std::min(std::max(offset.y(), minY), maxY));
    }
}
