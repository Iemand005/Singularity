#pragma once

#include <QObject>
#include <LLModel.hpp>

#include "QChatManager.hpp"

class QLLModel : public QObject, public LLModel {
    Q_OBJECT
    std::shared_ptr<TextContext> messageContext = nullptr;

public:

    LLModel *super() {
        return this;
    }

    QLLModel(const QString &path, const LLModelOptions &options = {}) : LLModel(path.toStdString(), options) {
        messageContext = createContext();
    }

    QChatManagerPtr createChatManager(const QString systemPrompt = "") {
        return std::make_unique<QChatManager>(this, systemPrompt);
    }

};
