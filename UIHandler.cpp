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

void UIHandler::loadModel(const QString &path) {
    qDebug() << "Loading model at:" << path;
    
    if (llmWorkerThread && llmWorkerThread->isRunning()) {
        qWarning() << "Already loading LLM model";
        return;
    }

    LLModelOptions options;

    modelFactory->loadLLMAsync(path, options, [this](QLLModelPtr model) {
        llm = std::move(model);
        chatManager = llm->createChatManager();
    });

}

void UIHandler::prompt(const QString &message) {
    QAsyncTextGenOptions options;
    options.onToken = [this](const QString &token) {
        tokenReceived(token);
    };
    chatManager->sendAsync(message, options); // This does what all that garble below used todo
}

void UIHandler::loadSDModel(const QString &path) {
    qDebug() << "Loading SD model at:" << path;
    
    sdm = nullptr; // Unload old first
    modelFactory->loadSDMAsync(path, [this](QSDModelPtr model) {
        sdm = std::move(model);
    });
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
