#pragma once

#include <QObject>
#include <ChatManager.hpp>

class QLLModel;

typedef std::function<void(const QString &token)> QTokenCallback;

class QChatManager : public QObject, public ChatManager {
    Q_OBJECT

    ChatManager *super() { return (ChatManager *)this; }

public:

    QChatManager(QLLModel *model);

    void sendAsync(QString message, FinishCallback onDone, QTokenCallback onToken = nullptr, ProgressCallback onInputEval = nullptr) {
        super()->sendAsync(message.toStdString(), onDone, [onToken](std::string token) { onToken(QString(token.c_str())); }, onInputEval);
    }

};

using QChatManagerPtr = std::unique_ptr<QChatManager>;
