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

    QLLModel(const QString path) : LLModel(path.toStdString()) {
        messageContext = createContext();
    }

    QChatManagerPtr createChatManager() {
        return std::make_unique<QChatManager>(this);
    }

};
