#pragma once

#include <QObject>
#include <QThread>
#include <ModelFactory.hpp>

#include "QLLModel.hpp"
#include "QSDModel.hpp"

typedef std::unique_ptr<QLLModel> QLLModelPtr;
typedef std::unique_ptr<QSDModel> QSDModelPtr;

typedef FinishedCallback<QLLModelPtr> QLoadLLModelFinished;
typedef FinishedCallback<QSDModelPtr> QLoadSDModelFinished;

class QModelFactory : public QObject, public ModelFactory {
    Q_OBJECT

    ModelFactory *super() {
        return this;
    }

public:
    QLLModelPtr loadLLM(const QString &path, const LLModelOptions &options = {}) {
        initLlama();
        return std::make_unique<QLLModel>(path, options);
    }

    void loadLLMAsync(const QString &path, LLModelOptions options = {}, QLoadLLModelFinished onDone = nullptr) {
        runAsync([this, path, options, onDone]() { onDone(loadLLM(path, options)); });
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

    void convertSDModelAsync(const QString source, QuantTypes level, const QString destination, ProgressCallback onProgress = nullptr, FinishedCallback<bool> onDone = nullptr) {
        std::string src = source.toStdString(), dxt = destination.toStdString();
        super()->convertSDModelAsync(src, level, dxt, onProgress, onDone);
    }

};

using QModelFactoryPtr = std::unique_ptr<QModelFactory>;
