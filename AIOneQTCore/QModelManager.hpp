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
  QSDModelPtr sdm;

 public:
  QModelManager() : factory(std::make_unique<QModelFactory>()) {}

  void loadLLMAsync(QString path, LLModelOptionsAsync options = {}) {
    auto syncOptions = dynamic_cast<LLModelOptions&>(options);
    factory->loadLLMAsync(path, syncOptions, [this, options](QLLModelPtr model) {
      llm = std::move(model);
      chatManager = llm->createChatManager();
      options.done();
    });
  }

  void loadSDMAsync(QString path, SDModelOptionsAsync options = {}) {
    auto syncOptions = dynamic_cast<SDModelOptions&>(options);
    factory->loadSDMAsync(path, syncOptions, [this, options](QSDModelPtr model) {
      sdm = std::move(model);
      options.done();
    });
  }

  QLLModel *getLLM() { return llm.get(); }
  QSDModel *getSDM() { return sdm.get(); }
  QChatManager *getChatManager() { return chatManager.get(); }
  QModelFactory *getFactory() { return factory.get(); }
};

typedef std::unique_ptr<QModelManager> QModelManagerPtr;