
#include "QChatManager.hpp"
#include "QLLModel.hpp"
#include "../AIOne/src/Providers/ILLMProvider.hpp"

QChatManager::QChatManager(QLLModel *model, const QString systemPrompt) : ChatManager(model->super(), systemPrompt.toStdString()) {}

QChatManager::QChatManager(AIOne::ILLMProvider *provider, const QString systemPrompt)
    : ChatManager(*provider) {
    if (!systemPrompt.isEmpty()) setSystemPrompt(systemPrompt);
}
