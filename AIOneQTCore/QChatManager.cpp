
#include "QChatManager.hpp"
#include "QLLModel.hpp"

QChatManager::QChatManager(QLLModel *model) {
    super()->setModel(model->super());
}
