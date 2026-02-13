#pragma once

#include <codecvt>
#include <locale>
#include <string>

#include "QChatManager.hpp"
#include "QModelFactory.hpp"

class QModelManager {
  QModelFactoryPtr factory;

  QChatManagerPtr chatManager;

  QLLModelPtr llm;

 public:
  QModelManager() : factory(std::make_unique<QModelFactory>()) {}

  void loadLLMAsync(QString path, LLModelOptionsAsync options = {}) {
    LLModelOptions& syncOptions = dynamic_cast<LLModelOptions&>(options);
    factory->loadLLMAsync(path, syncOptions, [this, options](QLLModelPtr model) {
      llm = std::move(model);
      chatManager = std::make_unique<QChatManager>(getLLM());
      if (options.onDone) options.onDone();
    });
  }

  QLLModel* getLLM() { return llm.get(); }
  QChatManager* getChatManager() { return chatManager.get(); }
};

typedef std::unique_ptr<QModelManager> QModelManagerPtr;