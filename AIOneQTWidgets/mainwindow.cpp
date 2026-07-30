#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "messagewidget.h"

#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include <QThread>
#include <QVBoxLayout>
#include <QTimer>
#include <QMessageBox>
#include <QKeyEvent>

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
        if (chatManager)
            chatManager->currentChatOptions()->maxTokens = ui->maxTokensInput->value();
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

    // Restore API key from settings
    if (!m_settings.apiKey.empty()) {
        ui->openAIKey->setText(QString::fromStdString(m_settings.apiKey));
    }

    connect(ui->openAIButton, &QPushButton::clicked, [this]() {
        QString apiKey = ui->openAIKey->text().trimmed();
        if (apiKey.isEmpty()) return;

        // Save API key
        m_settings.apiKey = apiKey.toStdString();
        saveSettings();

        ui->openAIButton->setEnabled(false);
        ui->openAIButton->setText("Fetching...");

        static const std::string baseUrl = "api.groq.com/openai";
        openAIProvider = std::make_unique<AIOne::OpenAIProvider>(baseUrl, apiKey.toStdString());

        std::thread([this]() {
            auto models = openAIProvider->getModels();
            QMetaObject::invokeMethod(this, [this, models]() {
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

void MainWindow::setupCloudChatManager(const QString &modelId, const QString &apiKey) {
    if (!openAIProvider) {
        openAIProvider = std::make_unique<AIOne::OpenAIProvider>(
            "api.groq.com/openai", apiKey.toStdString());
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

    // Save current chat before switching
    if (chatManager) {
        chatManager->saveCurrentChatMetadata();
    }

    m_loadingChat = true;

    // Load the selected chat
    auto meta = ChatStorage::loadMetadata(folder.toStdString());

    if (!chatManager) {
        // No model loaded, create a dummy chat manager for browsing history
        auto dummyChat = std::make_unique<QChatManager>(
            std::make_unique<AIOne::OpenAIProvider>("api.groq.com/openai", "").get());
        chatManager = std::move(dummyChat);
    }

    chatManager->loadChat(folder.toStdString());
    m_currentModelName = QString::fromStdString(meta.model);

    // Restore UI state
    ui->systemPromptInput->setPlainText(QString::fromStdString(meta.systemPrompt));
    ui->maxTokensInput->setValue(meta.params.maxTokens);

    // Reload messages into listWidget
    ui->listWidget->clear();
    auto msgs = chatManager->getCurrentChat()->getMessages();
    for (const auto& msg : msgs) {
        if (msg.role == "system") continue;
        auto *listItem = new QListWidgetItem(ui->listWidget);
        auto *w = new MessageWidget(ui->listWidget);
        w->setContent(QString::fromStdString(msg.content));
        w->finish();
        ui->listWidget->setItemWidget(listItem, w);
        listItem->setSizeHint(w->minimumSizeHint());
    }

    ui->llmInputFrame->setEnabled(true);

    m_loadingChat = false;
}

void MainWindow::onNewChat() {
    if (!chatManager) {
        // Need at least a chat manager - create a stub provider-based one if nothing loaded
        ui->statusbar->showMessage("Load a model first");
        return;
    }

    if (!chatManager->getCurrentChat())
        return;

    m_loadingChat = true;

    // Save current chat first
    chatManager->saveCurrentChatMetadata();

    // Get current system prompt and params from UI
    std::string systemPrompt = ui->systemPromptInput->toPlainText().toStdString();
    TextGenOptionsBase params;
    params.maxTokens = ui->maxTokensInput->value();

    chatManager->createNewChat("Untitled", m_currentModelName.toStdString(),
                                systemPrompt, params);

    // Clear message display
    ui->listWidget->clear();
    ui->llmInputFrame->setEnabled(true);

    m_loadingChat = false;

    // Refresh sidebar
    refreshChatList();
    // Select the new chat (last item)
    m_chatList->setCurrentRow(m_chatList->count() - 1);
}

void MainWindow::syncChatToUI() {
    if (!chatManager || !chatManager->getCurrentChat()) return;

    ui->listWidget->clear();
    auto msgs = chatManager->getCurrentChat()->getMessages();
    for (const auto& msg : msgs) {
        if (msg.role == "system") continue;
        auto *item = new QListWidgetItem(ui->listWidget);
        auto *w = new MessageWidget(ui->listWidget);
        w->setContent(QString::fromStdString(msg.content));
        w->finish();
        ui->listWidget->setItemWidget(item, w);
        item->setSizeHint(w->minimumSizeHint());
    }
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
}

void MainWindow::saveSettings() {
    ChatStorage::saveSettings(m_settings);
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
        // Update sidebar - the title may have changed
        refreshChatList();
    }
}

void MainWindow::send() {
    if (m_generating) {
        m_stopRequested = true;
        return;
    }

    QString message = ui->messageInput->toPlainText();
    if (message.isEmpty() || !chatManager) return;

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

    connect(m_generatingWidget, &MessageWidget::sizeChanged, this, [this]() {
        if (m_generatingItem && m_generatingWidget) {
            m_generatingItem->setSizeHint(m_generatingWidget->minimumSizeHint());
            ui->listWidget->doItemsLayout();
        }
    });

    QAsyncTextGenOptions options;

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
            m_generatingWidget->appendToken(token);
            ui->listWidget->scrollToBottom();
            auto display = ui->tokensGeneratedDisplay;
            display->display(display->intValue() + 1);
        });
    };

    options.onDone = [this](const TextGenResult &output) {
        QMetaObject::invokeMethod(this, [this, output]() {
            if (m_generatingWidget) {
                auto content = QString::fromStdString(output.output.content);
                m_generatingWidget->setContent(content);
                m_generatingWidget->finish();
                if (m_generatingItem)
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
            ui->inputEvalProgressBar->setRange(0, 100);
            ui->inputEvalProgressBar->setValue(100);

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
