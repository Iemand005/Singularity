#pragma once

#include <QObject>
#include <QThread>
#include <ModelFactory.hpp>

#include "QLLModel.hpp"
#include "QSDModel.hpp"

// Large Language model pointer
using QLLModelPtr = std::unique_ptr<QLLModel>;
// Stable Diffusion model pointer
using QSDModelPtr = std::unique_ptr<QSDModel>;

using QLoadLLModelFinished = std::function<void(QLLModelPtr model)>;

class QModelFactory : public QObject, public ModelFactory {
    Q_OBJECT

    ModelFactory *super() {
        return this;
    }

public:
    QLLModelPtr loadLLM(const QString &path, const LLModelOptions &options = {}, ProgressCallback onProgress = nullptr) {
        initLlama();
        return std::make_unique<QLLModel>(path, options, onProgress);
    }

    void loadLLMAsync(QObject *mainThread, const QString &path, const LLModelOptions &options = {}, QLoadLLModelFinished onDone = nullptr, ProgressCallback onProgress = nullptr) {
        QThread *loaderThread = new QThread();

        // Use a lambda or function
        QObject::connect(loaderThread, &QThread::started, [this, mainThread, path, options, onDone, onProgress]() {


            onDone(loadLLM(path, options, [](const float &progress) {
                QMetaObject::invokeMethod(mainThread, [onProgress, progress]() {
                    // This lambda runs on UI thread
                    onProgress(progress);
                });
            }));
        });

        loaderThread->start();
    }

    QSDModelPtr loadSDM(const QString &path) {
        return std::make_unique<QSDModel>(path);
    }

};

using QModelFactoryPtr = std::unique_ptr<QModelFactory>;
