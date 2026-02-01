
#include "QChatManager.hpp"
#include "QLLModel.hpp"

QChatManager::QChatManager(QLLModel *model, const QString systemPrompt) : ChatManager(model->super(), systemPrompt.toStdString()) {}
