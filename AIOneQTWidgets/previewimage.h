#ifndef PREVIEWIMAGE_H
#define PREVIEWIMAGE_H

#include <QLabel>
#include <QObject>
#include <QWidget>

class PreviewImage : public QLabel
{
    Q_OBJECT
public:
    PreviewImage();
    explicit PreviewImage(QWidget *parent = nullptr) : QLabel(parent) {
        setAlignment(Qt::AlignCenter);
    }

    void setImage(const QPixmap &pixmap) {
        this->pixmap = pixmap;
        updateScaledPixmap();
    }

    void setImage(const QImage &image) {
        setImage(QPixmap::fromImage(image));
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        QLabel::resizeEvent(event);
        updateScaledPixmap();
    }

private:
    void updateScaledPixmap() {
        if (pixmap.isNull()) return;

        QPixmap scaled = pixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QLabel::setPixmap(scaled);
    }

    QPixmap pixmap;
};

#endif // PREVIEWIMAGE_H
