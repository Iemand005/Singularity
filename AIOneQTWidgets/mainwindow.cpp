#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QGraphicsPixmapItem>

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
        }, [this](const float &progress) {
            QMetaObject::invokeMethod(ui->llmLoadProgressBar, &ProgressBar::setPercentage, Qt::QueuedConnection, progress);
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

        QString fileName = QFileDialog::getOpenFileName(
            this,
            tr("Open Stable Diffusion model file"),
            nullptr,
            tr("Stable Diffusion models (*.safetensors *.gguf);;All Files (*)")
            );
        qDebug() << "and this is the file" << fileName;

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

        options.onProgress = [this](float progress) {
            QMetaObject::invokeMethod(ui->sdmLoadProgressBar, &ProgressBar::setPercentage, Qt::QueuedConnection, progress);
        };

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
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open VAE .safetensors file"), QDir::homePath(), tr("SafeTensors files (*.safetensors);"));
        qDebug() << "and this is the file" << fileName;
        this->vaePath = fileName;
    });

    connect(ui->chooseTaeButton, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open TAE .safetensors file"), QDir::homePath(), tr("SafeTensors files (*.safetensors);"));
        qDebug() << "and this is the file" << fileName;
        this->taePath = fileName;
    });

    connect(ui->clipGButton, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open CLIP G .safetensors file"), QDir::homePath(), tr("SafeTensors files (*.safetensors);"));
        qDebug() << "and this is the file" << fileName;
        this->sdModelOptions.clipGPath = fileName.toStdString();
    });

    connect(ui->clipLButton, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open CLIP L .safetensors file"), QDir::homePath(), tr("SafeTensors files (*.safetensors);"));
        qDebug() << "and this is the file" << fileName;
        this->sdModelOptions.clipLPath = fileName.toStdString();
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
            showImage(image);
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
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open CLIP L .safetensors file"), QDir::homePath(), tr("SafeTensors files (*.safetensors);"));
        qDebug() << "and this is the file" << fileName;
        this->quantModelPath = fileName;
    });

    connect(ui->quantizeButton, &QPushButton::clicked, [this]() {
        qDebug() << "Quantizing...";
        QString fileName = QFileDialog::getSaveFileName(this, tr("Open L SAVE TO RAARRAwwawawa .gguf file"), QDir::homePath(), tr("SafeTensors shit files (*.gguf);"));
        qDebug() << "and this is the file" << fileName;

        auto type = QuantTypes(ui->quantInputBox->currentIndex());
        factory->convertSDModelAsync(this->quantModelPath, type, fileName, [this](const float &progress) {
            QMetaObject::invokeMethod(ui->quantProgressBar, &ProgressBar::setPercentage, Qt::QueuedConnection, progress);
        });
        // auto ee = progressFor(ui->quantProgressBar);
        // factory->convertSDModelAsync(this->quantModelPath, type, fileName, ee);
    });
}

ProgressCallback MainWindow::progressFor(ProgressBar *bar) {
    return [bar](const float &progress) {
        QMetaObject::invokeMethod(bar, &ProgressBar::setPercentage, Qt::QueuedConnection, progress);
    };
}

void MainWindow::send() {
    QString message = ui->messageInput->toPlainText();

    ui->messageInput->setPlainText("");
    ui->listWidget->addItem(message);

    ui->listWidget->addItem("");
    auto lastItem = ui->listWidget->item(ui->listWidget->count() - 1);
    ui->tokensGeneratedDisplay->display(0);

    QAsyncTextGenOptions options;

    options.onDone = [this](const TextGenResult &output) {
        ui->tokensCachedDisplay->display((int)output.tokensCached);
        ui->tokensGeneratedDisplay->display((int)output.tokensGenerated);
        ui->tokensEvaluatedDisplay->display((int)output.tokensEvaluated);
    };

    options.onToken = [this, lastItem](const QString &token) {

        QMetaObject::invokeMethod(ui->listWidget, [this, lastItem, token]() {

            lastItem->setText(lastItem->text() + token);

            ui->listWidget->scrollToBottom();

            auto display = ui->tokensGeneratedDisplay;
            display->display(display->intValue() + 1);
        });

    };

    options.onInputEval = [this](const float &progress) {
        QMetaObject::invokeMethod(ui->inputEvalProgressBar, &ProgressBar::setPercentage, Qt::QueuedConnection, progress);
    };

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

void MainWindow::showImage(QImage image) {
    QMetaObject::invokeMethod(ui->imagePreview, [this, image]() {
        ui->imagePreview->setImage(image);
    });
}

void MainWindow::onPreviewGenerated(int step, const QImage& preview, bool isNoisy) {
    showImage(preview);
    auto bar = ui->generationProgressBar;
    bar->setValue(step);
    if (step == bar->maximum()) {
        bar->setIndeterminate();
        ui->statusbar->showMessage("VAE Decoding...");
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
