#pragma once

#include <QObject>
#include <QImage>
#include <SDModel.hpp>
#include <Callbacks.h>

class QSDModel : public QObject, public SDModel {
    Q_OBJECT

    SDModel *super() {
        return (SDModel *)this;
    }

    QImage convertToQImage(const SDImage& sdImage) {
        if (sdImage.channel == 3) {
            QImage qImage(sdImage.width, sdImage.height, QImage::Format_RGB32);

            for (uint32_t y = 0; y < sdImage.height; y++) {
                QRgb* scanline = (QRgb*)qImage.scanLine(y);
                const uint8_t* src = sdImage.data + (y * sdImage.width * 3);

                for (uint32_t x = 0; x < sdImage.width; x++)
                    scanline[x] = qRgb(src[x * 3], src[x * 3 + 1], src[x * 3 + 2]);
            }
            return qImage;
        } else if (sdImage.channel == 4)
            return QImage(sdImage.data, sdImage.width, sdImage.height, sdImage.width * 4, QImage::Format_RGBA8888).copy();

        return QImage();
    }



public:

    QSDModel(const QString &path, SDModelOptions options = {}) : SDModel(path.toStdString(), options) {
        this->setPreviewCallback([this](int step, SDImage* sdImage, bool isNoisy) {
            QImage image = this->convertToQImage(*sdImage);
            emit this->previewGenerated(step, image, isNoisy);
        });
    }

    using QImageCallback = std::function<void(QImage &image)>;

    void generateAsync(const QString &positive, const QString &negative, SDImageOptions options = SDImageOptions{}, QImageCallback callback = nullptr) {
        super()->generateAsync(positive.toStdString(), negative.toStdString(), options, [this, callback](SDImage sdImage) {
            QImage image = convertToQImage(sdImage);
            if (callback) callback(image);
        });
    }

    QImage generateImage(QString &positive, QString &negative, SDImageOptions options = {}) {
        const SDImage image = super()->generateImage(positive.toStdString(), negative.toStdString(), options);
        return convertToQImage(image);
    }

signals:
    void stepProgress(int step, int totalSteps);
    
    void previewGenerated(int step, const QImage& preview, bool isNoisy);
};
