#include "UIHandler.h"

#include <QDebug>
#include <QImage>
#include <iostream>

UIHandler::UIHandler(QObject *parent) : QObject{parent}
{
    // modelFactory = std::make_unique<QModelFactory>();
    // chatMessages = std::make_unique<std::vector<Message>>();

    modelManager = std::make_unique<QModelManager>();

    std::cout << "Llama.cpp System Info: " << modelFactory->systemInfoStr() << std::endl;
}

void UIHandler::loadModel(const QString &path) {
    qDebug() << "Loading model at:" << path;
    
    if (llmWorkerThread && llmWorkerThread->isRunning()) {
        qWarning() << "Already loading LLM model";
        return;
    }

    LLModelOptionsAsync options;

    options.onDone = [&]() {
        // TODO: Enable UI
    };

    modelManager->loadLLMAsync(path, options);
}

void UIHandler::prompt(const QString &message) {
    QAsyncTextGenOptions options;
    options.onToken = [this](const QString &token) {
        tokenReceived(token);
    };
    modelManager->getChatManager()->sendAsync(message, options); // This does what all that garble below used todo
}

void UIHandler::loadSDModel(const QString &path) {
    qDebug() << "Loading SD model at:" << path;
    
    // sdm = nullptr; // Unload old first
    SDModelOptionsAsync options = {};
    options.onDone = [](){};
    modelManager->loadSDMAsync(path, options);

    auto D= 8;
    if (8==D) std::cout << "EEE";

    // auto e = []<>(){};

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
