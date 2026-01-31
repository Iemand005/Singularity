#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#include <QObject>
#include <QString>
#include <QThread>
#include <QMutex>

#include "AIOneQTCore/QModelFactory.hpp"

class UIHandler : public QObject
{
    Q_OBJECT

public:
    explicit UIHandler(QObject *parent = nullptr);

    QModelFactoryPtr modelFactory;

    QLLModelPtr llm;
    QSDModelPtr sdm;

    std::unique_ptr<std::vector<Message>> chatMessages;

public slots:
    void handleButtonClick();
    void handleButtonClickWithParam(const QString &message);

    void loadModel(const QString &path);
    void loadSDModel(const QString &path);

    void prompt(const QString &message);
    void generateImage(const QString &prompt);

signals:
    void responseSent(const QString &response);
    void tokenReceived(const QString &token);
    void imageGenerated(const QImage &image);

private:
    QThread *workerThread = nullptr;
    QThread *llmWorkerThread = nullptr;
    QThread *sdWorkerThread = nullptr;
    QMutex mutex;
    QMutex llmMutex;
    QMutex sdMutex;
};

#endif // INPUTHANDLER_H
