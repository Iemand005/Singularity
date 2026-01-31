#pragma once

#include <LLModel.hpp>
#include <QObject>

class QLLModel : public QObject, public LLModel {
    Q_OBJECT
    std::shared_ptr<MessageContext> messageContext = nullptr;

public:

    QLLModel(const QString path) : LLModel(path.toStdString()) {
        messageContext = createContext();
    }

};
