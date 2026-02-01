#pragma once

#include <QObject>
#include <LLModel.hpp>

#include "QChatManager.hpp"

class QLLModel : public QObject, public LLModel {
    Q_OBJECT
    std::shared_ptr<TextContext> messageContext = nullptr;

public:

    LLModel *super() {
        return (LLModel *)this;
    }

    QLLModel(const QString &path) : QLLModel(path, nullptr) {}

    QLLModel(const QString &path, ProgressCallback onProgress = nullptr) : QLLModel(path, LLModelOptions{}, onProgress) {}

    QLLModel(const QString &path, const LLModelOptions &options = {}, ProgressCallback onProgress = nullptr) : LLModel(path.toStdString(), options, onProgress) {
        messageContext = createContext();
    }

    QChatManagerPtr createChatManager() {
        return std::make_unique<QChatManager>(this);
    }

};
