#pragma once

#include <LLModel.hpp>
#include <QObject>

class QLLModel : public LLModel, public QObject {

    std::shared_ptr<MessageContext> messageContext = nullptr;

public:

    QLLModel(QString path) : LLModel(path.toStdString()) {
        messageContext = createContext();
    }

};
