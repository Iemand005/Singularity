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
using QLoadSDModelFinished = std::function<void(QSDModelPtr model)>;

class QModelFactory : public QObject, public ModelFactory {
    Q_OBJECT

    ModelFactory *super() {
        return this;
    }

    void runAsync(std::function<void()> func) {
        QThread *loaderThread = new QThread();
        QObject::connect(loaderThread, &QThread::started, func);
        loaderThread->start();
    }

public:
    QLLModelPtr loadLLM(const QString &path, const LLModelOptions &options = {}, ProgressCallback onProgress = nullptr) {
        initLlama();
        return std::make_unique<QLLModel>(path, options, onProgress);
    }

    void loadLLMAsync(const QString &path, const LLModelOptions &options = {}, QLoadLLModelFinished onDone = nullptr, ProgressCallback onProgress = nullptr) {
        runAsync([this, path, options, onDone, onProgress]() { onDone(loadLLM(path, options, onProgress)); });
    }

    QSDModelPtr loadSDM(const QString &path, SDModelOptions options = {}) {
        return std::make_unique<QSDModel>(path, options);
    }

    void loadSDMAsync(const QString &path, QLoadSDModelFinished onDone = nullptr) {
        runAsync([this, path, onDone]() { onDone(loadSDM(path)); });
    }

    void loadSDMAsync(const QString &path, SDModelOptions options, QLoadSDModelFinished onDone = nullptr) {
        runAsync([this, path, options, onDone]() { onDone(loadSDM(path, options)); });
    }

};

using QModelFactoryPtr = std::unique_ptr<QModelFactory>;
