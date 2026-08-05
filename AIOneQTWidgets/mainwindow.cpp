#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "messagewidget.h"

#include "../AIOne/src/Message.hpp"

#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include <QThread>
#include <QVBoxLayout>
#include <QTimer>
#include <QMessageBox>
#include <QKeyEvent>
#include <QScrollBar>
#include <QScopeGuard>
#include <QDateTime>

// Normalize a provider base URL so it ends with exactly one "/v1" segment.
// Adds "https://" when a scheme is missing, strips trailing "/v1" repeats
// (e.g. "/v1/v1") and adds "/v1" when missing.
static QString normalizedApiBaseUrl(QString url) {
    url = url.trimmed();
    if (url.isEmpty())
        return QStringLiteral("https://api.groq.com/openai/v1");
    if (!url.contains("://"))
        url = "https://" + url;
    while (url.endsWith('/')) url.chop(1);
    while (url.endsWith("/v1", Qt::CaseInsensitive)) {
        url.chop(3);
        while (url.endsWith('/')) url.chop(1);
    }
    url += "/v1";
    return url;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // ---- Chat Storage Init ----
    ChatStorage::init();
    m_chatsRoot = QString::fromStdString(ChatStorage::rootPath());

    loadSettings();

    // ---- Sidebar ----
    m_sidebar = new QWidget(this);
    auto *sbLayout = new QVBoxLayout(m_sidebar);
    sbLayout->setContentsMargins(4, 4, 4, 4);
    sbLayout->setSpacing(4);

    m_newChatBtn = new QPushButton("New Chat", m_sidebar);
    m_chatList = new QListWidget(m_sidebar);
    m_chatList->setFrameShape(QFrame::NoFrame);
    sbLayout->addWidget(m_newChatBtn);
    sbLayout->addWidget(m_chatList);

    m_sidebar->setMinimumWidth(0);
    m_sidebar->setMaximumWidth(300);

    // Wrap existing llmTab content in a container for the splitter
    auto *llmTab = ui->llmTab;
    auto *llmLayout = qobject_cast<QVBoxLayout*>(llmTab->layout());
    auto *rightContainer = new QWidget();
    rightContainer->setLayout(llmLayout); // reparents layout

    m_chatSplitter = new QSplitter(Qt::Horizontal, llmTab);
    m_chatSplitter->addWidget(m_sidebar);
    m_chatSplitter->addWidget(rightContainer);
    m_chatSplitter->setStretchFactor(0, 0);
    m_chatSplitter->setStretchFactor(1, 1);
    m_chatSplitter->setSizes({200, 800});

    // Replace the llmTab's layout with one containing only the splitter
    auto *outerLayout = new QVBoxLayout(llmTab);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->addWidget(m_chatSplitter);

    // Restore sidebar width from settings (default 200)
    int sidebarWidth = 200;
    m_chatSplitter->setSizes({sidebarWidth, llmTab->width() - sidebarWidth});

    connect(m_newChatBtn, &QPushButton::clicked, this, &MainWindow::onNewChat);
    connect(m_chatList, &QListWidget::currentRowChanged, this, &MainWindow::onChatSelected);

    // ---- LLM ----

    ui->llmLoadProgressBar->hide();
    connect(ui->loadLLMButton, &QPushButton::clicked, [this]() {
        qDebug() << "I need that!";

        QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("Open GGUF file"),
            nullptr,
            tr("GGUF files (*.gguf);")
            );
        qDebug() << "and this is the file" << fileName;

        ui->llmLoadProgressBar->showIntermediate();

        llm = nullptr;

        LLModelOptions options;
        options.onProgress = progressFor(ui->llmLoadProgressBar);

        factory->loadLLMAsync(fileName, options, [this, fileName](QLLModelPtr model) {
            if (!model) {
                qDebug() << "Umm this isn't normal ain't normal the model is empty bruh bro?!?!?!";
                return;
            }
            this->llm = std::move(model);
            this->chatManager = llm->createChatManager();
            m_currentModelName = fileName;

            qDebug() << "Loaded LLM";

            QMetaObject::invokeMethod(ui->llmInputFrame, [this]() {
                disconnect(ui->systemPromptInput, &QPlainTextEdit::textChanged, nullptr, nullptr);
                connect(ui->systemPromptInput, &QPlainTextEdit::textChanged, this, [this]() {
                    if (chatManager && !m_loadingChat) {
                        chatManager->setSystemPrompt(ui->systemPromptInput->toPlainText());
                        saveChatMetaDelayed();
                    }
                });
                ui->llmLoadProgressBar->hide();
                ui->llmInputFrame->setEnabled(true);

                // Create a new chat for this model
                if (!m_loadingChat) {
                    onNewChat();
                }
            });
        });
    });

    connect(ui->sendButton, &QPushButton::clicked, this, &MainWindow::send);

    connect(ui->continueButton, &QPushButton::clicked, [this]() {
        QString message = ui->messageInput->toPlainText();

        AsyncTextGenOptions options;
        options.maxTokens = 1;
        options.onToken = [this](const std::string &token) {
            QMetaObject::invokeMethod(ui->llmInputFrame, [this, token]() {
                ui->messageInput->insertPlainText(QString(token.c_str()));
            });
        };

        chatManager->completeAsync(message.toStdString(), options);
    });

    ui->messageInput->installEventFilter(this);

    connect(ui->maxTokensInput, &QSpinBox::valueChanged, [this]() {
        if (chatManager && !m_loadingChat && ui->maxTokensCheck->isChecked())
            chatManager->currentChatOptions()->maxTokens = ui->maxTokensInput->value();
    });

    connect(ui->maxTokensCheck, &QCheckBox::toggled, [this](bool checked) {
        ui->maxTokensInput->setEnabled(checked);
        if (chatManager && !m_loadingChat)
            chatManager->currentChatOptions()->maxTokens = checked ? ui->maxTokensInput->value() : 0;
    });

    // ---- Stable Diffusion ----

    ui->sdmLoadProgressBar->hide();
    ui->generationProgressBar->hide();

    connect(ui->loadSDButton, &QPushButton::clicked, [this]() {
        qDebug() << "I need that als too!";

        QString fileName = openFileDialog(tr("Open Stable Diffusion model file"), tr("Stable Diffusion models (*.safetensors *.gguf);;All Files (*)"));

        ui->sdmLoadProgressBar->showIntermediate();
        ui->statusbar->showMessage("Loading model...");

        if (this->sdm) this->sdm = nullptr;

        auto options = sdModelOptions;
        options.flashAttention = ui->flashAttentionBox->checked();
        options.freeParamsImmediately = ui->freeParamsBox->checked();
        options.keepClipOnCpu = ui->clipOnCpuBox->checked();
        options.keepControlNetOnCpu = ui->controlNetOnCpuBox->checked();
        options.keepVaeOnCpu = ui->vaeOnCpuBox->checked();
        if (ui->customVaeBox->checked()) options.vaePath = vaePath.toStdString();
        if (ui->useTaeBox->checked()) options.taePath = taePath.toStdString();

        options.onProgress = progressFor(ui->sdmLoadProgressBar);

        factory->loadSDMAsync(fileName, options, [this](QSDModelPtr model) {
            this->sdm = std::move(model);
            connect(this->sdm.get(), &QSDModel::previewGenerated, this, &MainWindow::onPreviewGenerated);

            QMetaObject::invokeMethod(ui->sdmLoadProgressBar, [this]() {
                ui->sdmLoadProgressBar->hide();
                ui->statusbar->showMessage("Done!");
            });
        });
    });

    connect(ui->vaeButton, &QPushButton::clicked, [this]() {
        this->vaePath = openFileDialog(tr("Open VAE .safetensors file"));
    });

    connect(ui->chooseTaeButton, &QPushButton::clicked, [this]() {
        this->taePath = openFileDialog(tr("Open TAE .safetensors file"));
    });

    connect(ui->clipGButton, &QPushButton::clicked, [this]() {
        this->sdModelOptions.clipGPath = openFileDialog(tr("Open CLIP G .safetensors file")).toStdString();
    });

    connect(ui->clipLButton, &QPushButton::clicked, [this]() {
        this->sdModelOptions.clipLPath = openFileDialog(tr("Open CLIP L .safetensors file")).toStdString();
    });

    connect(ui->generateButton, &QPushButton::clicked, [this]() {
        qDebug() << "Generating image...";

        ui->statusbar->showMessage("Generating...");

        auto options = sdImageOptions;
        options.cfgScale = ui->cfgInput->value();
        options.width = ui->widthBox->value();
        options.height = ui->heightBox->value();
        options.stepCount = ui->stepCountInput->value();
        options.clipSkip = ui->clipSkipInput->value();

        options.tiling.enabled = ui->vaeTilingBox->checked();
        options.tiling.overlap = ui->tilingOverlapInput->value();
        options.tiling.tileHeight = ui->tilingHeightInput->value();
        options.tiling.tileWidth = ui->tilingWidthInput->value();

        options.seed = ui->seedInput->value();

        if (ui->useTaeBox->checked()) options.previewMode = SDPreviewMode::TAE;

        auto bar = ui->generationProgressBar;
        bar->showAndReset();
        bar->setMaximum(options.stepCount);

        QString positive = ui->positiveInput->toPlainText();
        QString negative = ui->negativeInput->toPlainText();

        sdm->generateAsync(positive, negative, options, [this](QImage image) {
            showImage(image, true);
            QMetaObject::invokeMethod(ui->generationProgressBar, [this]() {
                ui->generationProgressBar->hide();
                ui->statusbar->showMessage("Done!");
                if (ui->randomizeSeedBox->isChecked()) ui->seedInput->setValue(sdm->newSeed());
            });
        });

        qDebug() << "Loaded Stable Diffusion model.";
    });

    // SD Quantization options

    connect(ui->selectQuantSourceButton, &QPushButton::clicked, [this]() {
        qDebug() << "Selecting model...";
        QString fileName = openFileDialog("Open CLIP L .safetensors file", "SafeTensors files (*.safetensors);");
        this->quantModelPath = fileName;
    });

    connect(ui->quantizeButton, &QPushButton::clicked, [this]() {
        qDebug() << "Quantizing...";
        auto bar = ui->quantProgressBar;
        bar->showIntermediate();

        QString fileName = openFileDialog("Open L SAVE TO RAARRAwwawawa .gguf file", "SafeTensors shit files (*.gguf);");

        auto type = QuantTypes(ui->quantInputBox->currentIndex());
        factory->convertSDModelAsync(this->quantModelPath, type, fileName, progressFor(bar), [bar](bool success){
            QMetaObject::invokeMethod(bar, &ProgressBar::hide);
        });
    });

    // ---- OpenAI Provider ----

    // Base URL presets (selectable, editable for custom endpoints)
    ui->openAIBaseUrlBox->setEditable(true);
    ui->openAIBaseUrlBox->addItem("Groq", QString("https://api.groq.com/openai/v1"));
    ui->openAIBaseUrlBox->addItem("OpenAI", QString("https://api.openai.com/v1"));
    ui->openAIBaseUrlBox->addItem("OpenRouter", QString("https://openrouter.ai/api/v1"));
    ui->openAIBaseUrlBox->addItem("Together AI", QString("https://api.together.xyz/v1"));
    ui->openAIBaseUrlBox->addItem("DeepInfra", QString("https://api.deepinfra.com/v1"));
    ui->openAIBaseUrlBox->addItem("Fireworks AI", QString("https://api.fireworks.ai/inference/v1"));
    ui->openAIBaseUrlBox->addItem("Mistral AI", QString("https://api.mistral.ai/v1"));

    // Restore previously selected base URL (adding it as a custom entry if needed)
    // (normalized in loadSettings; this just fills in the default)
    QString savedUrl = QString::fromStdString(m_settings.apiBaseUrl);
    if (savedUrl.isEmpty())
        savedUrl = normalizedApiBaseUrl(QString());
    m_settings.apiBaseUrl = savedUrl.toStdString();

    int savedIdx = ui->openAIBaseUrlBox->findData(savedUrl);
    if (savedIdx >= 0) {
        ui->openAIBaseUrlBox->setCurrentIndex(savedIdx);
    } else {
        ui->openAIBaseUrlBox->addItem(savedUrl, savedUrl);
        ui->openAIBaseUrlBox->setCurrentIndex(ui->openAIBaseUrlBox->count() - 1);
    }

    // Load the API key stored for the selected URL
    ui->openAIKey->setText(apiKeyForUrl(savedUrl));

    // Switching provider loads that provider's stored key
    connect(ui->openAIBaseUrlBox, qOverload<const QString &>(&QComboBox::currentTextChanged), this, [this]() {
        QString url = currentApiBaseUrl();
        if (url.isEmpty()) return;
        QString normalized = normalizedApiBaseUrl(url);
        // Keep the combo display in sync with the stored (normalized) URL
        if (normalized != url)
            ui->openAIBaseUrlBox->setEditText(normalized);
        m_settings.apiBaseUrl = normalized.toStdString();
        ui->openAIKey->setText(apiKeyForUrl(normalized));
        saveSettings();
    });

    // Auto-connect to the provider on launch if a key is saved for the URL
    QTimer::singleShot(0, this, [this]() {
        if (!currentApiBaseUrl().isEmpty()) {
            QString apiKey = apiKeyForUrl(currentApiBaseUrl());
            if (!apiKey.isEmpty())
                connectOpenAI(apiKey);
        }
    });

    connect(ui->openAIButton, &QPushButton::clicked, [this]() {
        QString apiKey = ui->openAIKey->text().trimmed();
        if (apiKey.isEmpty()) return;
        connectOpenAI(apiKey);
    });

    // ---- Finish initialization ----
    refreshChatList();
    loadLastChat();
}

ProgressCallback MainWindow::progressFor(ProgressBar *bar) {
    return [bar](const float &progress) {
        QMetaObject::invokeMethod(bar, &ProgressBar::setPercentage, Qt::QueuedConnection, progress);
    };
}

QString MainWindow::openFileDialog(const QString &title, QString fileTypes) {
    lastPath = QFileDialog::getOpenFileName(this, title, lastPath, fileTypes);
    qDebug() << "and this is the file" << lastPath;
    return lastPath;
}

void MainWindow::updateSeed() {
    if (ui->randomizeSeedBox->isChecked()) ui->seedInput->setValue(sdm->newSeed());
}

void MainWindow::connectOpenAI(const QString &apiKey) {
    if (apiKey.isEmpty()) return;

    QString baseUrl = normalizedApiBaseUrl(currentApiBaseUrl());
    if (baseUrl == "/v1")
        baseUrl = QString::fromStdString(m_settings.apiBaseUrl);

    // Save API key, mapped to its base URL
    m_settings.apiKey = apiKey.toStdString();
    m_settings.apiBaseUrl = baseUrl.toStdString();
    m_settings.apiKeys[baseUrl.toStdString()] = apiKey.toStdString();
    saveSettings();

    ui->openAIButton->setEnabled(false);
    ui->openAIButton->setText("Fetching...");

    openAIProvider = std::make_unique<AIOne::OpenAIProvider>(baseUrl.toStdString(), apiKey.toStdString());

    std::thread([this]() {
        auto models = openAIProvider->getModels();
        std::string error = openAIProvider->getLastError();
        QMetaObject::invokeMethod(this, [this, models, error]() {
            if (models.empty() && !error.empty()) {
                QMessageBox::warning(this, "Model fetch failed",
                    QString("Could not fetch models from the provider.\n\n%1")
                        .arg(QString::fromStdString(error)));
                ui->openAIButton->setText("Switch Model");
                ui->openAIButton->setEnabled(true);
                return;
            }

            QStringList modelNames;
            for (const auto& m : models)
                modelNames << QString::fromStdString(m.id);

            ui->modelBox->clear();
            ui->modelBox->addItems(modelNames);

            disconnect(ui->modelBox, &QComboBox::currentIndexChanged, nullptr, nullptr);
            connect(ui->modelBox, &QComboBox::currentIndexChanged, this, [this](int index) {
                if (index >= 0) {
                    QString model = ui->modelBox->currentText();
                    m_settings.lastAIModel = model.toStdString();
                    saveSettings();
                    setupCloudChatManager(model, ui->openAIKey->text().trimmed());
                }
            });

            // Restore previously selected model (fires the handler above)
            int selectIdx = -1;
            if (!m_settings.lastAIModel.empty()) {
                selectIdx = ui->modelBox->findText(QString::fromStdString(m_settings.lastAIModel));
            }
            if (selectIdx < 0 && ui->modelBox->count() > 0)
                selectIdx = 0;
            if (selectIdx >= 0)
                ui->modelBox->setCurrentIndex(selectIdx);

            ui->openAIButton->setText("Switch Model");
            ui->openAIButton->setEnabled(true);
        });
    }).detach();
}

void MainWindow::setupCloudChatManager(const QString &modelId, const QString &apiKey) {
    if (!openAIProvider) {
        QString baseUrl = currentApiBaseUrl();
        if (baseUrl.isEmpty())
            baseUrl = QString::fromStdString(m_settings.apiBaseUrl);
        openAIProvider = std::make_unique<AIOne::OpenAIProvider>(
            baseUrl.toStdString(), apiKey.toStdString());
    }

    auto newChat = std::make_unique<QChatManager>(openAIProvider.get());
    newChat->setModel(modelId.toStdString());
    newChat->setSystemPrompt(ui->systemPromptInput->toPlainText());

    chatManager = std::move(newChat);
    m_currentModelName = modelId;

    ui->llmLoadProgressBar->hide();
    ui->llmInputFrame->setEnabled(true);

    disconnect(ui->systemPromptInput, &QPlainTextEdit::textChanged, nullptr, nullptr);
    connect(ui->systemPromptInput, &QPlainTextEdit::textChanged, this, [this]() {
        if (chatManager && !m_loadingChat) {
chatManager->setSystemPrompt(ui->systemPromptInput->toPlainText());
        saveChatMetaDelayed();
        }
    });

    ui->statusbar->showMessage("Using OpenAI model: " + modelId);

    if (!m_loadingChat) {
        onNewChat();
    }
}

void MainWindow::initChatStorage() {
    ChatStorage::init();
}

void MainWindow::refreshChatList() {
    m_chatList->blockSignals(true);
    m_chatList->clear();

    auto chats = ChatStorage::scan();
    for (const auto& c : chats) {
        auto *item = new QListWidgetItem(QString::fromStdString(c.title));
        item->setData(Qt::UserRole, QString::fromStdString(c.folder));

        // Show timestamp
        QDateTime dt = QDateTime::fromMSecsSinceEpoch(c.updated);
        item->setToolTip(dt.toString("yyyy-MM-dd hh:mm"));

        m_chatList->addItem(item);
    }

    m_chatList->blockSignals(false);
}

void MainWindow::onChatSelected(int row) {
    if (row < 0 || m_loadingChat) return;

    auto *item = m_chatList->item(row);
    if (!item) return;

    QString folder = item->data(Qt::UserRole).toString();
    if (folder.isEmpty()) return;

    // Force-reset any in-flight generation
    forceStopGeneration();

    // Save current chat before switching
    if (chatManager) {
        chatManager->saveCurrentChatMetadata();
    }

    m_loadingChat = true;
    // Ensure m_loadingChat is reset even if something throws
    auto resetLoading = qScopeGuard([this]() { m_loadingChat = false; });

    // Load the selected chat
    auto meta = ChatStorage::loadMetadata(folder.toStdString());

    if (!chatManager) {
        QString baseUrl = currentApiBaseUrl();
        if (baseUrl.isEmpty())
            baseUrl = QString::fromStdString(m_settings.apiBaseUrl);
        chatManager = std::make_unique<QChatManager>(
            new AIOne::OpenAIProvider(baseUrl.toStdString(), ""));
    }

    chatManager->loadChat(folder.toStdString());
    m_currentModelName = QString::fromStdString(meta.model);

    // Restore UI state - read system prompt from loaded chat messages, not metadata
    auto* loadedChat = chatManager->getCurrentChat();
    std::string actualSystemPrompt;
    if (loadedChat) {
        auto loadMsgs = loadedChat->getMessages();
        if (!loadMsgs.empty() && loadMsgs[0].role == "system")
            actualSystemPrompt = loadMsgs[0].content;
    }
    ui->systemPromptInput->setPlainText(QString::fromStdString(actualSystemPrompt));

    bool tokensEnabled = meta.params.maxTokens > 0;
    ui->maxTokensCheck->setChecked(tokensEnabled);
    ui->maxTokensInput->setValue(tokensEnabled ? meta.params.maxTokens : 50000);

    rebuildConversationDisplay();

    ui->llmInputFrame->setEnabled(true);
}

void MainWindow::onNewChat() {
    if (!chatManager) {
        ui->statusbar->showMessage("Load a model first");
        return;
    }

    if (!chatManager->getCurrentChat())
        return;

    // Force-reset any in-flight generation
    forceStopGeneration();

    // Save current chat first
    chatManager->saveCurrentChatMetadata();

    // Get current system prompt and params from UI
    std::string systemPrompt = ui->systemPromptInput->toPlainText().toStdString();
    TextGenOptionsBase params;
    params.maxTokens = ui->maxTokensCheck->isChecked() ? ui->maxTokensInput->value() : 0;

    // Create new chat with "Untitled" - title will be auto-generated from first user message
    std::string newFolder = chatManager->createNewChat("Untitled", m_currentModelName.toStdString(),
                                systemPrompt, params);

    // Clear message display
    m_slotItems.clear();
    ui->listWidget->clear();
    ui->llmInputFrame->setEnabled(true);

    // Refresh sidebar
    refreshChatList();
    // Select the newly created chat (it sorts to the top by updated date)
    int newRow = 0;
    QString newFolderStr = QString::fromStdString(newFolder);
    for (int i = 0; i < m_chatList->count(); ++i) {
        if (m_chatList->item(i)->data(Qt::UserRole).toString() == newFolderStr) {
            newRow = i;
            break;
        }
    }
    m_chatList->setCurrentRow(newRow);
}

void MainWindow::syncChatToUI() {
    rebuildConversationDisplay();
}

void MainWindow::saveChatMetaDelayed() {
    static QTimer *debounce = nullptr;
    if (!debounce) {
        debounce = new QTimer(this);
        debounce->setSingleShot(true);
        connect(debounce, &QTimer::timeout, this, [this]() {
            if (chatManager) chatManager->saveCurrentChatMetadata();
        });
    }
    debounce->start(2000);
}

void MainWindow::loadSettings() {
    m_settings = ChatStorage::loadSettings();

    // Migrate stored per-URL API keys (and the current base URL) to normalized
    // URLs so a key saved under e.g. "https://api.groq.com/openai" is still found
    // after the base URL is normalized to ".../openai/v1".
    std::map<std::string, std::string> normalized;
    for (const auto& [url, key] : m_settings.apiKeys)
        normalized[normalizedApiBaseUrl(QString::fromStdString(url)).toStdString()] = key;
    m_settings.apiKeys = std::move(normalized);
    m_settings.apiBaseUrl = normalizedApiBaseUrl(
        QString::fromStdString(m_settings.apiBaseUrl)).toStdString();
}

void MainWindow::saveSettings() {
    ChatStorage::saveSettings(m_settings);
}

QString MainWindow::currentApiBaseUrl() const {
    int idx = ui->openAIBaseUrlBox->currentIndex();
    if (idx >= 0) {
        QString url = ui->openAIBaseUrlBox->itemData(idx).toString().trimmed();
        if (!url.isEmpty())
            return url;
    }
    return ui->openAIBaseUrlBox->currentText().trimmed();
}

QString MainWindow::apiKeyForUrl(const QString &url) const {
    auto it = m_settings.apiKeys.find(url.toStdString());
    if (it != m_settings.apiKeys.end())
        return QString::fromStdString(it->second);
    return QString();
}

void MainWindow::loadLastChat() {
    if (!m_settings.lastChatFolder.empty()) {
        // Find the chat in the list
        for (int i = 0; i < m_chatList->count(); ++i) {
            auto *item = m_chatList->item(i);
            if (item->data(Qt::UserRole).toString() == QString::fromStdString(m_settings.lastChatFolder)) {
                m_chatList->setCurrentRow(i);
                return;
            }
        }
    }
    // If no last chat or not found, select the most recent
    if (m_chatList->count() > 0) {
        m_chatList->setCurrentRow(0);
    }
}

void MainWindow::onSendDone() {
    if (chatManager) {
        chatManager->saveCurrentChatMetadata();
        refreshChatList();
    }
    // streaming widget is already in place with final content — no rebuild needed
}

void MainWindow::forceStopGeneration() {
    if (m_generating || m_stopRequested) {
        m_stopRequested = true;
        m_generating = false;
        m_generatingWidget = nullptr;
        m_generatingItem = nullptr;
        ui->sendButton->setText("Send");
        ui->messageInput->setEnabled(true);
        ui->inputEvalProgressBar->setIndeterminate(false);
        ui->inputEvalProgressBar->hide();
    }
}

void MainWindow::rebuildConversationDisplay() {
    if (!chatManager || !chatManager->getCurrentChat()) return;

    auto* chat = chatManager->getCurrentChat();
    auto activePath = chat->getActivePath();

    // Hard fallback: if activePath is empty or has only system messages,
    // display all non-system messages in order.
    auto allMsgs = chat->getMessages();
    size_t nonSystem = 0;
    for (auto& m : allMsgs)
        if (m.role != "system") ++nonSystem;

    bool useFallback = false;
    size_t shownInPath = 0;
    for (auto& m : activePath)
        if (m.role != "system") ++shownInPath;
    if (shownInPath == 0 && nonSystem > 0) useFallback = true;

    m_slotItems.clear();
    ui->listWidget->clear();

    const auto& displayMsgs = useFallback ? allMsgs : activePath;
    const bool lastIsUser = !displayMsgs.empty() && displayMsgs.back().role == "user";

    for (size_t msgIdx = 0; msgIdx < displayMsgs.size(); ++msgIdx) {
        const auto& msg = displayMsgs[msgIdx];
        if (msg.role == "system") continue;

        auto *item = new QListWidgetItem(ui->listWidget);
        auto *w = new MessageWidget(ui->listWidget);
        uint64_t slotParentId = msg.parentId;

        w->setContent(QString::fromStdString(msg.content));
        w->setParentId(slotParentId);
        w->setAssistantMessage(msg.role == "assistant");

        if (lastIsUser && msgIdx == displayMsgs.size() - 1) {
            w->setGenerateVisible(true);
            connect(w, &MessageWidget::generateRequested, this, [this, msg]() {
                onGenerateLastResponse(msg.id);
            });
        }

        const Message* displayMsg = &msg;
        if (msg.role == "assistant") {
            if (useFallback) {
                w->setVersionInfo(0, 1);
            } else {
                auto siblings = chat->getSiblings(slotParentId);
                size_t idx = 0;
                for (size_t i = 0; i < siblings.size(); ++i) {
                    if (siblings[i].id == msg.id) { idx = i; break; }
                }
                if (idx >= siblings.size()) idx = 0;
                // Keep the runtime version selection in sync with the active path
                chat->setCurrentVersionIndex(slotParentId, idx);
                displayMsg = &siblings[idx];
                w->setContent(QString::fromStdString(displayMsg->content));
                w->setVersionInfo(idx, siblings.size());
            }
        }

        w->setMessageId(displayMsg->id);
        w->setTimestamp(displayMsg->timestamps.creationTime);
        w->finish();

        connect(w, &MessageWidget::prevRequested, this, [this, slotParentId]() {
            onVersionPrev(slotParentId);
        });
        connect(w, &MessageWidget::nextRequested, this, [this, slotParentId]() {
            onVersionNext(slotParentId);
        });
        connect(w, &MessageWidget::regenerateRequested, this, [this, slotParentId]() {
            onRegenerateRequested(slotParentId);
        });
        connect(w, &MessageWidget::editRequested, this, [this, slotParentId]() {
            onEditRequested(slotParentId);
        });
        connect(w, &MessageWidget::continueRequested, this, [this, slotParentId]() {
            onContinueRequested(slotParentId);
        });
        connect(w, &MessageWidget::forkRequested, this, &MainWindow::onForkRequested);
        connect(w, &MessageWidget::sizeChanged, this, [this, item, w]() {
            if (item && w) {
                item->setSizeHint(w->minimumSizeHint());
                ui->listWidget->doItemsLayout();
            }
        });

        ui->listWidget->setItemWidget(item, w);
        item->setSizeHint(w->minimumSizeHint());
        m_slotItems[slotParentId] = item;
    }
}

void MainWindow::onVersionPrev(uint64_t parentId) {
    cancelEditMode();
    auto* chat = chatManager->getCurrentChat();
    size_t idx = chat->getCurrentVersionIndex(parentId);
    if (idx == 0) return;
    chat->setCurrentVersionIndex(parentId, idx - 1);

    auto it = m_slotItems.find(parentId);
    if (it == m_slotItems.end()) return;

    auto *item = it->second;
    auto siblings = chat->getSiblings(parentId);
    size_t newIdx = chat->getCurrentVersionIndex(parentId);
    if (newIdx >= siblings.size()) return;

    auto *w = new MessageWidget(ui->listWidget);
    w->setContent(QString::fromStdString(siblings[newIdx].content));
    w->finish();
    w->setVersionInfo(newIdx, siblings.size());
    w->setParentId(parentId);
    w->setMessageId(siblings[newIdx].id);
    w->setTimestamp(siblings[newIdx].timestamps.creationTime);

    connect(w, &MessageWidget::prevRequested, this, [this, parentId]() {
        onVersionPrev(parentId);
    });
    connect(w, &MessageWidget::nextRequested, this, [this, parentId]() {
        onVersionNext(parentId);
    });
    connect(w, &MessageWidget::regenerateRequested, this, [this, parentId]() {
        onRegenerateRequested(parentId);
    });
    connect(w, &MessageWidget::editRequested, this, [this, parentId]() {
        onEditRequested(parentId);
    });
    connect(w, &MessageWidget::continueRequested, this, [this, parentId]() {
        onContinueRequested(parentId);
    });
    connect(w, &MessageWidget::forkRequested, this, &MainWindow::onForkRequested);
    connect(w, &MessageWidget::sizeChanged, this, [this, item, w]() {
        if (item && w) {
            item->setSizeHint(w->minimumSizeHint());
            ui->listWidget->doItemsLayout();
        }
    });

    ui->listWidget->setItemWidget(item, w);
    item->setSizeHint(w->minimumSizeHint());
}

void MainWindow::onVersionNext(uint64_t parentId) {
    cancelEditMode();
    auto* chat = chatManager->getCurrentChat();
    auto siblings = chat->getSiblings(parentId);
    size_t idx = chat->getCurrentVersionIndex(parentId);
    if (idx + 1 >= siblings.size()) return;
    chat->setCurrentVersionIndex(parentId, idx + 1);

    auto it = m_slotItems.find(parentId);
    if (it == m_slotItems.end()) return;

    auto *item = it->second;
    size_t newIdx = chat->getCurrentVersionIndex(parentId);
    if (newIdx >= siblings.size()) return;

    auto *w = new MessageWidget(ui->listWidget);
    w->setContent(QString::fromStdString(siblings[newIdx].content));
    w->finish();
    w->setVersionInfo(newIdx, siblings.size());
    w->setParentId(parentId);
    w->setMessageId(siblings[newIdx].id);
    w->setTimestamp(siblings[newIdx].timestamps.creationTime);

    connect(w, &MessageWidget::prevRequested, this, [this, parentId]() {
        onVersionPrev(parentId);
    });
    connect(w, &MessageWidget::nextRequested, this, [this, parentId]() {
        onVersionNext(parentId);
    });
    connect(w, &MessageWidget::regenerateRequested, this, [this, parentId]() {
        onRegenerateRequested(parentId);
    });
    connect(w, &MessageWidget::editRequested, this, [this, parentId]() {
        onEditRequested(parentId);
    });
    connect(w, &MessageWidget::continueRequested, this, [this, parentId]() {
        onContinueRequested(parentId);
    });
    connect(w, &MessageWidget::forkRequested, this, &MainWindow::onForkRequested);
    connect(w, &MessageWidget::sizeChanged, this, [this, item, w]() {
        if (item && w) {
            item->setSizeHint(w->minimumSizeHint());
            ui->listWidget->doItemsLayout();
        }
    });

    ui->listWidget->setItemWidget(item, w);
    item->setSizeHint(w->minimumSizeHint());
}

void MainWindow::onRegenerateRequested(uint64_t parentId) {
    if (m_generating) return;
    cancelEditMode();

    m_stopRequested = false;
    m_generating = true;

    ui->sendButton->setText("Stop");
    ui->messageInput->setEnabled(false);
    ui->inputEvalProgressBar->setRange(0, 0);
    ui->inputEvalProgressBar->show();

    auto it = m_slotItems.find(parentId);
    if (it == m_slotItems.end()) return;
    auto *item = it->second;

    m_generatingWidget = new MessageWidget(ui->listWidget);
    m_generatingWidget->setParentId(parentId);
    ui->listWidget->setItemWidget(item, m_generatingWidget);
    item->setSizeHint(m_generatingWidget->minimumSizeHint());

    connect(m_generatingWidget, &MessageWidget::sizeChanged, this, [this, item]() {
        if (item && m_generatingWidget) {
            item->setSizeHint(m_generatingWidget->minimumSizeHint());
            ui->listWidget->doItemsLayout();
        }
    });

    QAsyncTextGenOptions options;
    options.maxTokens = ui->maxTokensCheck->isChecked() ? ui->maxTokensInput->value() : 0;

    options.onError = [this](const QString &err) {
        QMetaObject::invokeMethod(this, [this, err]() {
            QMessageBox::warning(this, "Request failed", err);
        });
    };

    options.onTokenReasoning = [this](const QString &token, bool thinking) {
        QMetaObject::invokeMethod(m_generatingWidget, [this, token, thinking]() {
            if (!m_generatingWidget) return;
            m_generatingWidget->appendTokenReasoning(token, thinking);
        });
    };

    options.onThinkStateChange = [this](bool thinking) {
        QMetaObject::invokeMethod(m_generatingWidget, [this, thinking]() {
            if (m_generatingWidget)
                m_generatingWidget->setThinking(thinking);
        });
    };

    options.onToken = [this](const QString &token) {
        if (m_stopRequested) return;
        QMetaObject::invokeMethod(ui->listWidget, [this, token]() {
            if (!m_generatingWidget || m_stopRequested) return;
            auto *vbar = ui->listWidget->verticalScrollBar();
            bool nearBottom = vbar->value() >= vbar->maximum() - 50;
            m_generatingWidget->appendToken(token);
            if (nearBottom)
                ui->listWidget->scrollToBottom();
            auto display = ui->tokensGeneratedDisplay;
            display->display(display->intValue() + 1);
        });
    };

    options.onDone = [this, parentId](const TextGenResult &output) {
        QMetaObject::invokeMethod(this, [this, parentId, output]() {
            auto* chat = chatManager->getCurrentChat();

            if (m_generatingWidget) {
                auto content = QString::fromStdString(output.output.content);
                m_generatingWidget->setContent(content);
                auto siblings = chat->getSiblings(parentId);
                size_t idx = siblings.empty() ? 0 : siblings.size() - 1;
                m_generatingWidget->setVersionInfo(idx, std::max(siblings.size(), (size_t)1));
                m_generatingWidget->finish();
            }

            m_generating = false;
            m_stopRequested = false;

            auto it = m_slotItems.find(parentId);
            if (it != m_slotItems.end()) {
                auto* item = it->second;
                ui->listWidget->setItemWidget(item, m_generatingWidget);
                item->setSizeHint(m_generatingWidget->minimumSizeHint());

                connect(m_generatingWidget, &MessageWidget::prevRequested, this, [this, parentId]() {
                    onVersionPrev(parentId);
                });
                connect(m_generatingWidget, &MessageWidget::nextRequested, this, [this, parentId]() {
                    onVersionNext(parentId);
                });
                connect(m_generatingWidget, &MessageWidget::regenerateRequested, this, [this, parentId]() {
                    onRegenerateRequested(parentId);
                });
                connect(m_generatingWidget, &MessageWidget::editRequested, this, [this, parentId]() {
                    onEditRequested(parentId);
                });
                connect(m_generatingWidget, &MessageWidget::continueRequested, this, [this, parentId]() {
                    onContinueRequested(parentId);
                });
                connect(m_generatingWidget, &MessageWidget::sizeChanged, this, [this, item]() {
                    if (item && m_generatingWidget) {
                        item->setSizeHint(m_generatingWidget->minimumSizeHint());
                        ui->listWidget->doItemsLayout();
                    }
                });
            }
            m_generatingWidget = nullptr;

            ui->sendButton->setText("Send");
            ui->messageInput->setEnabled(true);
            ui->inputEvalProgressBar->setIndeterminate(false);
            ui->inputEvalProgressBar->hide();

            // Update version tracking: set to the last version
            auto siblings = chat->getSiblings(parentId);
            if (!siblings.empty())
                chat->setCurrentVersionIndex(parentId, siblings.size() - 1);

            if (chatManager) {
                chatManager->saveCurrentChatMetadata();
                refreshChatList();
            }
        });
    };

    options.onInputEval = progressFor(ui->inputEvalProgressBar);

    chatManager->regenerateAsync(parentId, options);
}

void MainWindow::onGenerateLastResponse(uint64_t parentId) {
    if (m_generating) return;
    cancelEditMode();
    auto* chat = chatManager ? chatManager->getCurrentChat() : nullptr;
    if (!chat) return;

    m_stopRequested = false;
    m_generating = true;

    ui->sendButton->setText("Stop");
    ui->messageInput->setEnabled(false);
    ui->inputEvalProgressBar->setRange(0, 0);
    ui->inputEvalProgressBar->show();

    ui->listWidget->addItem("");
    m_generatingItem = ui->listWidget->item(ui->listWidget->count() - 1);
    ui->tokensGeneratedDisplay->display(0);

    m_generatingWidget = new MessageWidget(ui->listWidget);
    m_generatingWidget->setParentId(parentId);
    ui->listWidget->setItemWidget(m_generatingItem, m_generatingWidget);
    m_generatingItem->setSizeHint(m_generatingWidget->minimumSizeHint());

    auto *genW = m_generatingWidget;
    auto *genItem = m_generatingItem;
    connect(genW, &MessageWidget::sizeChanged, this, [this, genW, genItem]() {
        if (genItem && genW) {
            genItem->setSizeHint(genW->minimumSizeHint());
            ui->listWidget->doItemsLayout();
        }
    });

    QAsyncTextGenOptions options;
    options.maxTokens = ui->maxTokensCheck->isChecked() ? ui->maxTokensInput->value() : 0;

    options.onError = [this](const QString &err) {
        QMetaObject::invokeMethod(this, [this, err]() {
            QMessageBox::warning(this, "Request failed", err);
        });
    };

    options.onTokenReasoning = [this](const QString &token, bool thinking) {
        QMetaObject::invokeMethod(m_generatingWidget, [this, token, thinking]() {
            if (!m_generatingWidget) return;
            m_generatingWidget->appendTokenReasoning(token, thinking);
        });
    };

    options.onThinkStateChange = [this](bool thinking) {
        QMetaObject::invokeMethod(m_generatingWidget, [this, thinking]() {
            if (m_generatingWidget)
                m_generatingWidget->setThinking(thinking);
        });
    };

    options.onToken = [this](const QString &token) {
        if (m_stopRequested) return;
        QMetaObject::invokeMethod(ui->listWidget, [this, token]() {
            if (!m_generatingWidget || m_stopRequested) return;
            auto *vbar = ui->listWidget->verticalScrollBar();
            bool nearBottom = vbar->value() >= vbar->maximum() - 50;
            m_generatingWidget->appendToken(token);
            if (nearBottom)
                ui->listWidget->scrollToBottom();
            auto display = ui->tokensGeneratedDisplay;
            display->display(display->intValue() + 1);
        });
    };

    options.onDone = [this, parentId](const TextGenResult &output) {
        QMetaObject::invokeMethod(this, [this, parentId, output]() {
            auto* chat = chatManager->getCurrentChat();

            auto *genW = m_generatingWidget;
            auto *genItem = m_generatingItem;
            m_generatingWidget = nullptr;
            m_generatingItem = nullptr;

            if (genW) {
                auto content = QString::fromStdString(output.output.content);
                genW->setContent(content);
                auto siblings = chat->getSiblings(parentId);
                size_t idx = siblings.empty() ? 0 : siblings.size() - 1;
                genW->setVersionInfo(idx, std::max(siblings.size(), (size_t)1));
                genW->finish();
            }

            m_generating = false;
            m_stopRequested = false;

            if (genItem) {
                m_slotItems[parentId] = genItem;
                if (genW) {
                    connect(genW, &MessageWidget::prevRequested, this, [this, parentId]() {
                        onVersionPrev(parentId);
                    });
                    connect(genW, &MessageWidget::nextRequested, this, [this, parentId]() {
                        onVersionNext(parentId);
                    });
                    connect(genW, &MessageWidget::regenerateRequested, this, [this, parentId]() {
                        onRegenerateRequested(parentId);
                    });
                    connect(genW, &MessageWidget::editRequested, this, [this, parentId]() {
                        onEditRequested(parentId);
                    });
                    connect(genW, &MessageWidget::continueRequested, this, [this, parentId]() {
                        onContinueRequested(parentId);
                    });
                    connect(genW, &MessageWidget::forkRequested, this, &MainWindow::onForkRequested);
                    genItem->setSizeHint(genW->minimumSizeHint());
                }
            }

            ui->sendButton->setText("Send");
            ui->messageInput->setEnabled(true);
            ui->inputEvalProgressBar->setIndeterminate(false);
            ui->inputEvalProgressBar->hide();

            // Update version tracking: set to the last version
            auto siblings = chat->getSiblings(parentId);
            if (!siblings.empty())
                chat->setCurrentVersionIndex(parentId, siblings.size() - 1);

            if (chatManager) {
                chatManager->saveCurrentChatMetadata();
                refreshChatList();
            }
        });
    };

    options.onInputEval = progressFor(ui->inputEvalProgressBar);

    chatManager->regenerateAsync(parentId, options);
}

void MainWindow::cancelEditMode() {
    if (m_editing) {
        m_editing = false;
        m_editingParentId = 0;
        ui->sendButton->setText("Send");
    }
}

void MainWindow::onEditRequested(uint64_t parentId) {
    if (m_generating) return;
    auto* chat = chatManager ? chatManager->getCurrentChat() : nullptr;
    if (!chat) return;

    auto siblings = chat->getSiblings(parentId);
    size_t idx = chat->getCurrentVersionIndex(parentId);
    if (idx >= siblings.size()) idx = siblings.empty() ? 0 : siblings.size() - 1;
    if (siblings.empty() || idx >= siblings.size()) return;

    ui->messageInput->setPlainText(QString::fromStdString(siblings[idx].content));
    m_editing = true;
    m_editingParentId = parentId;
    ui->sendButton->setText("Send Edit");
    ui->messageInput->setFocus();
}

void MainWindow::onContinueRequested(uint64_t parentId) {
    if (m_generating) return;
    cancelEditMode();
    auto* chat = chatManager ? chatManager->getCurrentChat() : nullptr;
    if (!chat) return;

    auto siblings = chat->getSiblings(parentId);
    size_t idx = chat->getCurrentVersionIndex(parentId);
    if (idx >= siblings.size()) idx = siblings.empty() ? 0 : siblings.size() - 1;
    if (siblings.empty() || idx >= siblings.size()) return;

    m_stopRequested = false;
    m_generating = true;

    ui->sendButton->setText("Stop");
    ui->messageInput->setEnabled(false);
    ui->inputEvalProgressBar->setRange(0, 0);
    ui->inputEvalProgressBar->show();

    auto it = m_slotItems.find(parentId);
    if (it == m_slotItems.end()) return;
    auto *item = it->second;
    m_generatingWidget = qobject_cast<MessageWidget*>(ui->listWidget->itemWidget(item));
    if (!m_generatingWidget) {
        m_generating = false;
        return;
    }

    QAsyncTextGenOptions options;
    options.maxTokens = ui->maxTokensCheck->isChecked() ? ui->maxTokensInput->value() : 0;

    options.onError = [this](const QString &err) {
        QMetaObject::invokeMethod(this, [this, err]() {
            QMessageBox::warning(this, "Request failed", err);
        });
    };

    options.onTokenReasoning = [this](const QString &token, bool thinking) {
        QMetaObject::invokeMethod(m_generatingWidget, [this, token, thinking]() {
            if (!m_generatingWidget) return;
            m_generatingWidget->appendTokenReasoning(token, thinking);
        });
    };

    options.onThinkStateChange = [this](bool thinking) {
        QMetaObject::invokeMethod(m_generatingWidget, [this, thinking]() {
            if (m_generatingWidget)
                m_generatingWidget->setThinking(thinking);
        });
    };

    options.onToken = [this](const QString &token) {
        if (m_stopRequested) return;
        QMetaObject::invokeMethod(ui->listWidget, [this, token]() {
            if (!m_generatingWidget || m_stopRequested) return;
            auto *vbar = ui->listWidget->verticalScrollBar();
            bool nearBottom = vbar->value() >= vbar->maximum() - 50;
            m_generatingWidget->appendToken(token);
            if (nearBottom)
                ui->listWidget->scrollToBottom();
        });
    };

    options.onDone = [this, parentId](const TextGenResult &output) {
        QMetaObject::invokeMethod(this, [this, parentId, output]() {
            auto* chat = chatManager->getCurrentChat();

            if (m_generatingWidget) {
                auto siblings = chat->getSiblings(parentId);
                size_t idx = chat->getCurrentVersionIndex(parentId);
                if (idx >= siblings.size()) idx = siblings.empty() ? 0 : siblings.size() - 1;
                if (idx < siblings.size()) {
                    // ChatManager::continueAsync already appended the continuation
                    m_generatingWidget->setContent(QString::fromStdString(siblings[idx].content));
                    m_generatingWidget->setVersionInfo(idx, siblings.size());
                    m_generatingWidget->finish();
                }
            }

            m_generating = false;
            m_stopRequested = false;
            m_generatingWidget = nullptr;

            ui->sendButton->setText("Send");
            ui->messageInput->setEnabled(true);
            ui->inputEvalProgressBar->setIndeterminate(false);
            ui->inputEvalProgressBar->hide();

            if (chatManager) {
                chatManager->saveCurrentChatMetadata();
                refreshChatList();
            }
        });
    };

    options.onInputEval = progressFor(ui->inputEvalProgressBar);

    chatManager->continueAsync(parentId, options);
}

void MainWindow::onForkRequested(uint64_t messageId) {
    if (m_generating) return;
    cancelEditMode();
    auto* chat = chatManager ? chatManager->getCurrentChat() : nullptr;
    if (!chat) return;

    // Chain of messages from the root up to (and including) the fork message
    auto chain = chat->getMessageChain(messageId);
    if (chain.empty()) return;

    // Save the current chat before switching to the fork
    chatManager->saveCurrentChatMetadata();

    std::string systemPrompt = ui->systemPromptInput->toPlainText().toStdString();
    if (systemPrompt.empty() && !chain.empty() && chain[0].role == "system")
        systemPrompt = chain[0].content;

    std::string modelName = chat->model();
    if (modelName.empty()) modelName = m_currentModelName.toStdString();
    TextGenOptionsBase params = *chat->getOptions();

    // Base the fork title on the source chat so the new folder is identifiable
    std::string baseTitle = "Fork";
    std::string curFolder = chat->folder();
    if (!curFolder.empty()) {
        auto pos = curFolder.rfind('/');
        if (pos != std::string::npos) curFolder = curFolder.substr(pos + 1);
        pos = curFolder.rfind('\\');
        if (pos != std::string::npos) curFolder = curFolder.substr(pos + 1);
        auto srcMeta = ChatStorage::loadMetadata(curFolder);
        if (!srcMeta.title.empty() && srcMeta.title != "Untitled")
            baseTitle = "Fork of " + srcMeta.title;
    }

    // Create the new chat folder (persists chat.json with the copied settings)
    std::string newFolder = chatManager->createNewChat(baseTitle, modelName, systemPrompt, params);

    // Copy the message history up to the fork point into the new folder
    auto* newChat = chatManager->getCurrentChat();
    newChat->setMessages(chain);
    for (auto& m : chain)
        newChat->saveMessage(m);

    // Keep the fork title and bump the updated time so it sorts to the top
    ChatMetadata fm = ChatStorage::loadMetadata(newFolder);
    fm.title = baseTitle;
    fm.updated = QDateTime::currentMSecsSinceEpoch();
    ChatStorage::saveMetadata(newFolder, fm);

    // Switch the UI to the forked chat without reloading from disk
    m_slotItems.clear();
    ui->listWidget->clear();
    rebuildConversationDisplay();

    refreshChatList();
    for (int i = 0; i < m_chatList->count(); ++i) {
        if (m_chatList->item(i)->data(Qt::UserRole).toString() == QString::fromStdString(newFolder)) {
            m_chatList->blockSignals(true);
            m_chatList->setCurrentRow(i);
            m_chatList->blockSignals(false);
            break;
        }
    }

    ui->statusbar->showMessage("Forked chat: " + QString::fromStdString(baseTitle));
}

void MainWindow::send() {
    if (m_generating) {
        m_stopRequested = true;
        forceStopGeneration();
        return;
    }

    QString message = ui->messageInput->toPlainText();
    if (message.isEmpty() || !chatManager) return;

    // Edit mode: turn the input into a new version of the message being edited
    if (m_editing) {
        auto* chat = chatManager->getCurrentChat();
        uint64_t editParent = m_editingParentId;
        m_editing = false;
        m_editingParentId = 0;
        ui->sendButton->setText("Send");
        ui->messageInput->setPlainText("");
        if (!chat) return;

        chat->addMessage(Message("assistant", message.toStdString(), editParent));
        auto siblings = chat->getSiblings(editParent);
        if (!siblings.empty())
            chat->setCurrentVersionIndex(editParent, siblings.size() - 1);

        chatManager->saveCurrentChatMetadata();
        refreshChatList();
        rebuildConversationDisplay();
        ui->listWidget->scrollToBottom();
        return;
    }

    m_stopRequested = false;
    m_generating = true;

    ui->sendButton->setText("Stop");
    ui->messageInput->setEnabled(false);
    ui->inputEvalProgressBar->setRange(0, 0);
    ui->inputEvalProgressBar->show();

    ui->messageInput->setPlainText("");
    ui->listWidget->addItem(message);

    ui->listWidget->addItem("");
    m_generatingItem = ui->listWidget->item(ui->listWidget->count() - 1);
    ui->tokensGeneratedDisplay->display(0);

    m_generatingWidget = new MessageWidget(ui->listWidget);
    ui->listWidget->setItemWidget(m_generatingItem, m_generatingWidget);
    m_generatingItem->setSizeHint(m_generatingWidget->minimumSizeHint());

    // Capture the widget/item so the handler keeps working after onDone nulls
    // the m_generating* members (expanding the think area after completion).
    auto *genW = m_generatingWidget;
    auto *genItem = m_generatingItem;
    connect(genW, &MessageWidget::sizeChanged, this, [this, genW, genItem]() {
        if (genItem && genW) {
            genItem->setSizeHint(genW->minimumSizeHint());
            ui->listWidget->doItemsLayout();
        }
    });

    QAsyncTextGenOptions options;
    options.maxTokens = ui->maxTokensCheck->isChecked() ? ui->maxTokensInput->value() : 0;

    options.onError = [this](const QString &err) {
        QMetaObject::invokeMethod(this, [this, err]() {
            QMessageBox::warning(this, "Request failed", err);
        });
    };

    options.onTokenReasoning = [this](const QString &token, bool thinking) {
        QMetaObject::invokeMethod(m_generatingWidget, [this, token, thinking]() {
            if (!m_generatingWidget) return;
            m_generatingWidget->appendTokenReasoning(token, thinking);
        });
    };

    options.onThinkStateChange = [this](bool thinking) {
        QMetaObject::invokeMethod(m_generatingWidget, [this, thinking]() {
            if (m_generatingWidget)
                m_generatingWidget->setThinking(thinking);
        });
    };

    options.onToken = [this](const QString &token) {
        if (m_stopRequested) return;
        QMetaObject::invokeMethod(ui->listWidget, [this, token]() {
            if (!m_generatingWidget || m_stopRequested) return;
            auto *vbar = ui->listWidget->verticalScrollBar();
            bool nearBottom = vbar->value() >= vbar->maximum() - 50;
            m_generatingWidget->appendToken(token);
            if (nearBottom)
                ui->listWidget->scrollToBottom();
            auto display = ui->tokensGeneratedDisplay;
            display->display(display->intValue() + 1);
        });
    };

    options.onDone = [this](const TextGenResult &output) {
        QMetaObject::invokeMethod(this, [this, output]() {
            if (m_generatingWidget && m_generatingItem) {
                auto content = QString::fromStdString(output.output.content);
                m_generatingWidget->setContent(content);
                m_generatingWidget->setVersionInfo(0, 1);
                m_generatingWidget->finish();

                auto* chat = chatManager->getCurrentChat();
                uint64_t parentId = m_generatingWidget->parentId();
                if (parentId == 0 && chat) {
                    auto allMsgs = chat->getMessages();
                    if (allMsgs.size() >= 2) {
                        parentId = allMsgs[allMsgs.size() - 2].id;
                        m_generatingWidget->setParentId(parentId);
                    }
                }
                if (parentId != 0) {
                    m_slotItems[parentId] = m_generatingItem;

                    connect(m_generatingWidget, &MessageWidget::prevRequested, this, [this, parentId]() {
                        onVersionPrev(parentId);
                    });
                    connect(m_generatingWidget, &MessageWidget::nextRequested, this, [this, parentId]() {
                        onVersionNext(parentId);
                    });
                    connect(m_generatingWidget, &MessageWidget::regenerateRequested, this, [this, parentId]() {
                        onRegenerateRequested(parentId);
                    });
                    connect(m_generatingWidget, &MessageWidget::editRequested, this, [this, parentId]() {
                        onEditRequested(parentId);
                    });
                    connect(m_generatingWidget, &MessageWidget::continueRequested, this, [this, parentId]() {
                        onContinueRequested(parentId);
                    });
                }
                m_generatingItem->setSizeHint(m_generatingWidget->minimumSizeHint());
            }

            ui->tokensCachedDisplay->display((int)output.tokensCached);
            ui->tokensGeneratedDisplay->display((int)output.tokensGenerated);
            ui->tokensEvaluatedDisplay->display((int)output.tokensEvaluated);

            m_generating = false;
            m_stopRequested = false;
            m_generatingItem = nullptr;
            m_generatingWidget = nullptr;

            ui->sendButton->setText("Send");
            ui->messageInput->setEnabled(true);
            ui->inputEvalProgressBar->setIndeterminate(false);
            ui->inputEvalProgressBar->hide();

            onSendDone();
        });
    };

    options.onInputEval = progressFor(ui->inputEvalProgressBar);

    chatManager->sendAsync(message, options);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui->messageInput && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = (QKeyEvent*)event;
        if ((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) && !m_generating) {
            send();
            return true;
        }
    }
    return false;
}

void MainWindow::showImage(QImage image, bool smoorthTransform) {
    QMetaObject::invokeMethod(ui->imagePreview, &ImagePreview::setImageWithTransform, Qt::QueuedConnection, image, smoorthTransform);
}

void MainWindow::onPreviewGenerated(int step, const QImage& preview, bool isNoisy) {
    showImage(preview);

    auto bar = ui->generationProgressBar;
    bar->setValue(step);
    if (bar->full()) {
        bar->setIndeterminate();
        ui->statusbar->showMessage("VAE Decoding...");
    }
}

MainWindow::~MainWindow()
{
    // Save before closing
    if (chatManager) {
        chatManager->saveCurrentChatMetadata();
    }
    m_settings.lastChatFolder = "";
    if (chatManager && chatManager->getCurrentChat()) {
        std::string folder = chatManager->getCurrentChat()->folder();
        auto pos = folder.rfind('/');
        if (pos != std::string::npos) folder = folder.substr(pos + 1);
        pos = folder.rfind('\\');
        if (pos != std::string::npos) folder = folder.substr(pos + 1);
        m_settings.lastChatFolder = folder;
    }
    saveSettings();
    delete ui;
}
