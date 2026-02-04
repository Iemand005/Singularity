#ifndef IMAGEPREVIEW_H
#define IMAGEPREVIEW_H

#include <QGraphicsView>
#include <QGraphicsPixmapItem>

class ImagePreview : public QGraphicsView
{
    Q_OBJECT
public:
    ImagePreview();
    ImagePreview(QWidget *parent = nullptr) : QGraphicsView(parent) {
        setScene(new QGraphicsScene(this));
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setRenderHint(QPainter::Antialiasing);
        setRenderHint(QPainter::SmoothPixmapTransform);
        setMinimumSize(10, 10);
    }

public slots:
    void setPixmap(const QPixmap &pixmap) {
        scene()->clear();
        pixmapItem = scene()->addPixmap(pixmap);
        pixmapItem->setTransformationMode(mode);
        updateView();
    }

    void setImage(const QImage &image) {
        setPixmap(QPixmap::fromImage(image));
    }

    void setImageWithTransform(const QImage &image, bool enabled = false) {
        setPixmap(QPixmap::fromImage(image));
        setSmoothTransform(enabled);
    }

    void setSmoothTransform(bool enabled = true) {
        mode = enabled ? Qt::SmoothTransformation : Qt::FastTransformation;
        pixmapItem->setTransformationMode(mode);
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        QGraphicsView::resizeEvent(event);
        updateView();
    }

private:
    void updateView() {
        if (pixmapItem) fitInView(scene()->itemsBoundingRect(), Qt::KeepAspectRatio);
    }

    Qt::TransformationMode mode = Qt::FastTransformation;

    QGraphicsPixmapItem *pixmapItem = nullptr;
};

#endif // IMAGEPREVIEW_H
