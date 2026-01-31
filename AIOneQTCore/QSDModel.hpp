#pragma once

#include <QObject>
#include <QImage>
#include <SDModel.hpp>


class QSDModel : public QObject, public SDModel {
    Q_OBJECT

    SDModel *super() {
        return (SDModel *)this;
    }

    QImage convertToQImage(const sd_image_t& sd_img) {
        if (sd_img.channel == 3) {
            QImage image(sd_img.width, sd_img.height, QImage::Format_RGB32);

            for (int y = 0; y < sd_img.height; y++) {
                QRgb* scanline = (QRgb*)image.scanLine(y);
                const uint8_t* src = sd_img.data + (y * sd_img.width * 3);

                for (int x = 0; x < sd_img.width; x++)
                    scanline[x] = qRgb(src[x * 3], src[x * 3 + 1], src[x * 3 + 2]);
            }
            return image;
        } else if (sd_img.channel == 4) {
            return QImage(sd_img.data, sd_img.width, sd_img.height,
                          sd_img.width * 4, QImage::Format_RGBA8888).copy();
        }

        return QImage();
    }



public:

    QSDModel(QString &path) : SDModel(path.toStdString()) {
        this->setPreviewCallback([this](int step, int frameCount, sd_image_t* sdImage, bool isNoisy) {
            QImage image = this->convertToQImage(*sdImage);
            emit this->previewGenerated(step, image, isNoisy);
        });
    }

    using QImageCallback = std::function<void(QImage &image)>;

    void generateAsync(QString &positive, QString &negative, SDImageOptions options = SDImageOptions{}, QImageCallback callback = nullptr) {
        super()->generateAsync(positive.toStdString(), negative.toStdString(), options, [this, callback](sd_image_t sdImage) {
            QImage image = convertToQImage(sdImage);
            if (callback) callback(image);
        });
    }

    QImage generateImage(QString &positive, QString &negative, SDImageOptions options = {}) {
        const sd_image_t image = super()->generateImage(positive.toStdString(), negative.toStdString(), options);
        // if (save) this->saveImageAsPNG(image, positive.toStdString() + "rawr.png");
        return convertToQImage(image);
    }

    

    // using QPreviewCallback = std::function<void(int step, int frame_count, sd_image_t* image, bool is_noisy)>;

signals:
    void stepProgress(int step, int totalSteps);
    
    void previewGenerated(int step, const QImage& preview, bool isNoisy);
};
