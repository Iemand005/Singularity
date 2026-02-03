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

        ui->llmLoadProgressBar->show();

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
                QString systemPrompt = ui->systemPromptInput->toPlainText();

                connect(ui->systemPromptInput, &QPlainTextEdit::textChanged, [this]() {
                    QString systemPrompt = ui->systemPromptInput->toPlainText();
                    this->chatManager->setSystemPrompt(systemPrompt);
                });
                ui->llmLoadProgressBar->hide();
                ui->llmInputFrame->setEnabled(true);
            });
        }, [this](const float &progress) {
            QMetaObject::invokeMethod(ui->llmLoadProgressBar, [this, progress]() {
                ui->llmLoadProgressBar->setValue(progress * 100);
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

        QString fileName = QFileDialog::getOpenFileName(
            this,                    // Parent widget
            tr("Open .safetensors file"),         // Dialog title
            nullptr,        // Starting directory
            tr("SafeTensors files (*.safetensors);")
            );
        qDebug() << "and this is the file" << fileName;

        // ui->loadSDButton->
        // TODO: disable lod button

        // sdm->setProgressCallback();

        ui->sdmLoadProgressBar->show();
        ui->statusbar->showMessage("Loading model...");

        if (this->sdm) this->sdm = nullptr;

        SDModelOptions options;
        options.flashAttention = ui->flashAttentionBox->isChecked();
        options.freeParamsImmediately = ui->freeParamsBox->isChecked();
        options.keepClipOnCpu = ui->clipOnCpuBox->isChecked();
        options.keepControlNetOnCpu = ui->controlNetOnCpuBox->isChecked();
        options.keepVaeOnCpu = ui->vaeOnCpuBox->isChecked();
        if (ui->customVaeBox->isEnabled()) options.vaePath = vaePath.toStdString();

        options.onProgress = [this](float progress) {
            QMetaObject::invokeMethod(ui->sdmLoadProgressBar, [this, progress]() {
                ui->sdmLoadProgressBar->setValue(progress * 100);

                ui->sdmLoadProgressBar->setMaximum(100);
                ui->sdmLoadProgressBar->setTextVisible(true);
            });
        };

        factory->loadSDMAsync(fileName, options, [this](QSDModelPtr model) {
            this->sdm = std::move(model);
            connect(this->sdm.get(), &QSDModel::previewGenerated, this, &MainWindow::onPreviewGenerated);

            QMetaObject::invokeMethod(ui->sdmLoadProgressBar, [this]() {
                ui->sdmLoadProgressBar->hide();
                ui->sdmLoadProgressBar->setMaximum(0);
                ui->sdmLoadProgressBar->setTextVisible(false);

                ui->statusbar->showMessage("Done!");
            });
        });
    });

    connect(ui->vaeButton, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getOpenFileName(this, tr("Open .safetensors file"), QDir::homePath(), tr("SafeTensors files (*.safetensors);"));
        qDebug() << "and this is the file" << fileName;
        vaePath = fileName;
    });

    connect(ui->generateButton, &QPushButton::clicked, [this]() {
        qDebug() << "Generating image...";

        ui->statusbar->showMessage("Generating...");

        SDImageOptions options;

        options.cfgScale = ui->cfgInput->value();
        options.width = ui->widthBox->value();
        options.height = ui->heightBox->value();
        options.stepCount = ui->stepCountInput->value();
        options.clipSkip = ui->clipSkipInput->value();

        options.tiling.enabled = ui->vaeTilingBox->isChecked();
        options.tiling.overlap = ui->tilingOverlapInput->value();
        options.tiling.tileHeight = ui->tilingHeightInput->value();
        options.tiling.tileWidth = ui->tilingWidthInput->value();

        options.seed = ui->seedInput->value();

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


    // QGraphicsScene *scene = new QGraphicsScene();
    // ui->imageView->setScene(scene);

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
        QMetaObject::invokeMethod(ui->inputEvalProgressBar, [this, progress]() {
            ui->inputEvalProgressBar->setValue(progress * 100);
        });
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
