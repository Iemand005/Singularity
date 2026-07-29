#pragma once

#include <QObject>
#include <ChatManager.hpp>

#include "QTextGenOptions.h"

class QLLModel;
namespace AIOne { class ILLMProvider; }



class QChatManager : public QObject, public ChatManager {
    Q_OBJECT

    ChatManager *super() { return (ChatManager *)this; }

public:

    QChatManager(QLLModel *model, const QString systemPrompt = "");
    QChatManager(AIOne::ILLMProvider *provider, const QString systemPrompt = "");

    void sendAsync(QString message, QAsyncTextGenOptions options) {
        AsyncTextGenOptions newOptions {{(TextGenOptionsBase)options}};
        newOptions.onToken = [options](std::string token) {
            if (options.onToken) options.onToken(QString(token.c_str()));
        };
        newOptions.onInputEval = options.onInputEval;
        newOptions.onDone = options.onDone;
        super()->sendAsync(message.toStdString(), newOptions);
    }

    void setSystemPrompt(QString prompt) {
        super()->setSystemPrompt(prompt.toStdString());
    };

};

using QChatManagerPtr = std::unique_ptr<QChatManager>;
