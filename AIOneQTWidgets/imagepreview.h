#ifndef IMAGEPREVIEW_H
#define IMAGEPREVIEW_H

#include <QGraphicsView>
#include <QObject>
#include <QWidget>

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

    void setImage(const QPixmap &pixmap) {
        scene()->clear();
        pixmapItem = scene()->addPixmap(pixmap);
        updateView();
    }

    void setImage(const QImage &image) {
        setImage(QPixmap::fromImage(image));
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

    QGraphicsPixmapItem *pixmapItem = nullptr;
};

#endif // IMAGEPREVIEW_H
