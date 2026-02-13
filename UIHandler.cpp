#include "UIHandler.h"

#include <QDebug>
#include <QImage>
#include <iostream>

UIHandler::UIHandler(QObject *parent) : QObject{parent}
{
    // modelFactory = std::make_unique<QModelFactory>();
    // chatMessages = std::make_unique<std::vector<Message>>();

    modelManager = std::make_unique<QModelManager>();

    std::cout << "Llama.cpp System Info: " << modelManager->getFactory()->systemInfoStr() << std::endl;
}

void UIHandler::loadModel(const QString &path) {
    qDebug() << "Loading model at:" << path;
    
    if (llmWorkerThread && llmWorkerThread->isRunning()) {
        qWarning() << "Already loading LLM model";
        return;
    }

    LLModelOptionsAsync options;
    modelManager->loadLLMAsync(path, options);
}

void UIHandler::prompt(const QString &message) {
    QAsyncTextGenOptions options;
    options.onToken = [&](const QString &token) {
        tokenReceived(token);
    };
    modelManager->getChatManager()->sendAsync(message, options); // This does what all that garble below used todo
}

void UIHandler::loadSDModel(const QString &path) {
    qDebug() << "Loading SD model at:" << path;
    
    SDModelOptionsAsync options = {};
    modelManager->loadSDMAsync(path, options);
}



void UIHandler::generateImage(const QString &positive) {
    qDebug() << "Generating image for:" << positive;

    QString negative = "";
    SDImageOptions options; // Fetch thos settings from the UI and insert into this

    modelManager->getSDM()->generateAsync(positive, negative, options, [this](QImage image) {
        emit imageGenerated(image);
    });
}
