#include "UIHandler.h"

#include <QDebug>
#include <QImage>
#include <iostream>

UIHandler::UIHandler(QObject *parent) : QObject{parent}
{
    modelFactory = std::make_unique<QModelFactory>();
    chatMessages = std::make_unique<std::vector<Message>>();

    std::cout << "Llama.cpp System Info: " << modelFactory->systemInfoStr() << std::endl;
}


// void UIHandler::handleButtonClick() {
//     qDebug() << "Button clicked from C++!";

//     emit responseSent("Clicked handled in C++!");
// }

// void UIHandler::handleButtonClickWithParam(const QString &message) {
//     qDebug() << "Received from QML:" << message;
// }

void UIHandler::loadModel(const QString &path) {
    qDebug() << "Loading model at:" << path;
    
    if (llmWorkerThread && llmWorkerThread->isRunning()) {
        qWarning() << "Already loading LLM model";
        return;
    }

    this->llm = modelFactory->loadLLM(path);
}

void UIHandler::prompt(const QString &message) {
    Message chatMessage = {"user", message.toStdString()};
    chatMessages->push_back(chatMessage);

    std::string finalPrompt = this->llm->chatToPrompt(*chatMessages);
    qDebug() << "Generating response to:" << finalPrompt;

    if (workerThread && workerThread->isRunning()) {
        qWarning() << "Already processing a prompt";
        return;
    }

    workerThread = new QThread();

    QObject::connect(workerThread, &QThread::started, [this, finalPrompt]() {
        QMutexLocker locker(&llmMutex);

        if (!llm) {
            qWarning() << "LLM model not loaded";
            return;
        }

        TextGenerationStats stats = this->llm->completeAny(finalPrompt, [this](const std::string token) {
            tokenReceived(QString(token.c_str()));
        });

        std::cout << "Finished generation" << std::endl;
        std::cout << "- " << std::to_string(stats.tokensGenerated) << " tokens generated" << std::endl;
        std::cout << std::endl;

        Message resultMessage = {"assistant", stats.output};
        chatMessages->push_back(resultMessage);

        workerThread->quit();
    });
    QObject::connect(workerThread, &QThread::finished, [this]() {
        workerThread->deleteLater();
        workerThread = nullptr;
    });

    workerThread->start();
}

void UIHandler::loadSDModel(const QString &path) {
    qDebug() << "Loading SD model at:" << path;
    
    if (sdWorkerThread && sdWorkerThread->isRunning()) {
        qWarning() << "Already loading SD model";
        return;
    }

    sdm = modelFactory->loadSDM(path);
}



void UIHandler::generateImage(const QString &positive) {
    qDebug() << "Generating image for:" << positive;
    
    if (!sdm) {
        qWarning() << "SD model not loaded";
        return;
    }

    QString negative = "";
    SDImageOptions options; // Fetch thos settings from the UI and insert into this

    sdm->generateAsync(positive, negative, options, [this](QImage image) {
        emit imageGenerated(image);
    });
}
