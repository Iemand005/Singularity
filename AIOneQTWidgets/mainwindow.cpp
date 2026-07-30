#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "messagewidget.h"

#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include <QThread>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // LLM

    ui->llmLoadProgressBar->hide();
    connect(ui->loadLLMButton, &QPushButton::clicked, [this]() {
        qDebug() << "I need that!";

        QString fileName = QFileDialog::getOpenFileName(
            this,                    // Parent widget
            tr("Open GGUF file"),         // Dialog title
            nullptr,        // Starting directory
            tr("GGUF files (*.gguf);")
            );
        qDebug() << "and this is the file" << fileName;

        ui->llmLoadProgressBar->showIntermediate();

        llm = nullptr; // Unload the old before reload TODO: check if the path is valid (exists) before unloading!

        LLModelOptions options;

        options.onProgress = progressFor(ui->llmLoadProgressBar);

        factory->loadLLMAsync(fileName, options, [this](QLLModelPtr model) {
            if (!model) {
                qDebug() << "Umm this isn't normal ain't normal the model is empty bruh bro?!?!?!";
                return;
            }
            this->llm = std::move(model);
            this->chatManager = llm->createChatManager();

            qDebug() << "Loaded LLM";

            QMetaObject::invokeMethod(ui->llmInputFrame, [this]() {
                connect(ui->systemPromptInput, &QPlainTextEdit::textChanged, [this]() {
                    this->chatManager->setSystemPrompt(ui->systemPromptInput->toPlainText());
                });
                ui->llmLoadProgressBar->hide();
                ui->llmInputFrame->setEnabled(true);
            });
        });
    });

    connect(ui->sendButton, &QPushButton::clicked, [this](){send();});

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
        chatManager->currentChatOptions()->maxTokens = ui->maxTokensInput->value();
    });


    // Stable Diffusion

    ui->sdmLoadProgressBar->hide();
    ui->generationProgressBar->hide();

    connect(ui->loadSDButton, &QPushButton::clicked, [this]() {
        qDebug() << "I need that als too!";

        QString fileName = openFileDialog(tr("Open Stable Diffusion model file"), tr("Stable Diffusion models (*.safetensors *.gguf);;All Files (*)"));

        // ui->loadSDButton->
        // TODO: disable lod button

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

        // SDImageOptions options;
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

    // SD Quantization o tpions

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

    // OpenAI Provider

    connect(ui->openAIButton, &QPushButton::clicked, [this]() {
        QString apiKey = ui->openAIKey->text();
        if (apiKey.isEmpty()) return;

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
                    if (index >= 0)
                        setupCloudChatManager(ui->modelBox->currentText(), ui->openAIKey->text());
                });

                ui->openAIButton->setText("Switch Model");
                ui->openAIButton->setEnabled(true);
            });
        }).detach();
    });
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

    ui->llmLoadProgressBar->hide();
    ui->llmInputFrame->setEnabled(true);

    disconnect(ui->systemPromptInput, &QPlainTextEdit::textChanged, nullptr, nullptr);
    connect(ui->systemPromptInput, &QPlainTextEdit::textChanged, this, [this]() {
        if (chatManager) chatManager->setSystemPrompt(ui->systemPromptInput->toPlainText());
    });

    ui->statusbar->showMessage("Using OpenAI model: " + modelId);
}

void MainWindow::send() {
    QString message = ui->messageInput->toPlainText();

    ui->messageInput->setPlainText("");
    ui->listWidget->addItem(message);

    ui->listWidget->addItem("");
    auto lastItem = ui->listWidget->item(ui->listWidget->count() - 1);
    ui->tokensGeneratedDisplay->display(0);

    auto *widget = new MessageWidget(ui->listWidget);
    ui->listWidget->setItemWidget(lastItem, widget);
    lastItem->setSizeHint(widget->minimumSizeHint());

    connect(widget, &MessageWidget::sizeChanged, this, [this, lastItem, widget]() {
        lastItem->setSizeHint(widget->minimumSizeHint());
        ui->listWidget->doItemsLayout();
    });

    QAsyncTextGenOptions options;

    options.onThinkStateChange = [widget](bool thinking) {
        QMetaObject::invokeMethod(widget, [widget, thinking]() {
            widget->setThinking(thinking);
        });
    };

    options.onToken = [this, lastItem, widget](const QString &token) {
        QMetaObject::invokeMethod(ui->listWidget, [this, lastItem, widget, token]() {
            widget->appendToken(token);

            ui->listWidget->scrollToBottom();

            auto display = ui->tokensGeneratedDisplay;
            display->display(display->intValue() + 1);
        });
    };

    options.onDone = [this, lastItem, widget](const TextGenResult &output) {
        QMetaObject::invokeMethod(this, [this, lastItem, widget, output]() {
            widget->finish();
            lastItem->setSizeHint(widget->minimumSizeHint());

            ui->tokensCachedDisplay->display((int)output.tokensCached);
            ui->tokensGeneratedDisplay->display((int)output.tokensGenerated);
            ui->tokensEvaluatedDisplay->display((int)output.tokensEvaluated);
        });
    };

    options.onInputEval = progressFor(ui->inputEvalProgressBar);

    chatManager->sendAsync(message, options);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui->messageInput && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = (QKeyEvent*)event;
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
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
    delete ui;
}
