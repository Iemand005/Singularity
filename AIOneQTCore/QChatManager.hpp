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
        newOptions.onThinkStateChange = [options](bool thinking) {
            if (options.onThinkStateChange) options.onThinkStateChange(thinking);
        };
        newOptions.onTokenReasoning = [options](std::string token, bool thinking) {
            if (options.onTokenReasoning) options.onTokenReasoning(QString(token.c_str()), thinking);
        };
        newOptions.onError = [options](std::string err) {
            if (options.onError) options.onError(QString(err.c_str()));
        };
        super()->sendAsync(message.toStdString(), newOptions);
    }

    void setSystemPrompt(QString prompt) {
        super()->setSystemPrompt(prompt.toStdString());
    };

    void continueAsync(uint64_t parentId, QAsyncTextGenOptions options) {
        AsyncTextGenOptions newOptions {{(TextGenOptionsBase)options}};
        newOptions.onToken = [options](std::string token) {
            if (options.onToken) options.onToken(QString(token.c_str()));
        };
        newOptions.onInputEval = options.onInputEval;
        newOptions.onDone = options.onDone;
        newOptions.onThinkStateChange = [options](bool thinking) {
            if (options.onThinkStateChange) options.onThinkStateChange(thinking);
        };
        newOptions.onTokenReasoning = [options](std::string token, bool thinking) {
            if (options.onTokenReasoning) options.onTokenReasoning(QString(token.c_str()), thinking);
        };
        newOptions.onError = [options](std::string err) {
            if (options.onError) options.onError(QString(err.c_str()));
        };
        super()->continueAsync(parentId, newOptions);
    }

    void regenerateAsync(uint64_t parentId, QAsyncTextGenOptions options) {
        AsyncTextGenOptions newOptions {{(TextGenOptionsBase)options}};
        newOptions.onToken = [options](std::string token) {
            if (options.onToken) options.onToken(QString(token.c_str()));
        };
        newOptions.onInputEval = options.onInputEval;
        newOptions.onDone = options.onDone;
        newOptions.onThinkStateChange = [options](bool thinking) {
            if (options.onThinkStateChange) options.onThinkStateChange(thinking);
        };
        newOptions.onTokenReasoning = [options](std::string token, bool thinking) {
            if (options.onTokenReasoning) options.onTokenReasoning(QString(token.c_str()), thinking);
        };
        newOptions.onError = [options](std::string err) {
            if (options.onError) options.onError(QString(err.c_str()));
        };
        super()->regenerateAsync(parentId, newOptions);
    }

};

using QChatManagerPtr = std::unique_ptr<QChatManager>;
