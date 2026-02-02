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

    connect(ui->loadLLMButton, &QPushButton::clicked, [this]() {
        qDebug() << "I need that!";

        QString fileName = QFileDialog::getOpenFileName(
            this,                    // Parent widget
            tr("Open GGUF file"),         // Dialog title
            QDir::homePath(),        // Starting directory
            tr("GGUF files (*.gguf);")
            );
        qDebug() << "and this is the file" << fileName;

        llm = nullptr; // Unload the old before reload TODO: check if the path is valid (exists) before unloading!

        LLModelOptions options;

        factory->loadLLMAsync(fileName, options, [this](QLLModelPtr model) {
            if (!model) {
                qDebug() << "Umm this isn't normal ain't normal the model is empty bruh bro?!?!?!";
                return;
            }
            this->llm = std::move(model);

            qDebug() << "Loaded LLM";

            QMetaObject::invokeMethod(ui->llmInputFrame, [this]() {
                QString systemPrompt = ui->systemPromptInput->toPlainText();
                this->chatManager = llm->createChatManager(systemPrompt);

                connect(ui->systemPromptInput, &QPlainTextEdit::textChanged, [this]() {
                    QString systemPrompt = ui->systemPromptInput->toPlainText();
                    this->chatManager->setSystemPrompt(systemPrompt);
                });

                ui->llmInputFrame->setEnabled(true);
            });
        }, [this](const float &progress) {
            QMetaObject::invokeMethod(ui->llmLoadProgressBar, [this, progress]() {
                ui->llmLoadProgressBar->setValue(progress * 100);
            });
        });
    });

    connect(ui->sendButton, &QPushButton::clicked, [this]() {
        send();
    });

    connect(ui->continueButton, &QPushButton::clicked, [this]() {
        QString message = ui->messageInput->toPlainText();
        // std::shared_ptr<Message> draft = std::make_shared<Message>(Role::User, message);
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

    connect(ui->loadSDButton, &QPushButton::clicked, [this]() {
        qDebug() << "I need that als too!";

        QString fileName = QFileDialog::getOpenFileName(
            this,                    // Parent widget
            tr("Open SafeTensors file"),         // Dialog title
            QDir::homePath(),        // Starting directory
            tr("SafeTensors files (*.safetensors);")
            );
        qDebug() << "and this is the file" << fileName;

        ui->sdmLoadProgressBar->show();

        if (this->sdm) this->sdm = nullptr;

        factory->loadSDMAsync(fileName, [this](QSDModelPtr model) {
            this->sdm = std::move(model);
            connect(this->sdm.get(), &QSDModel::previewGenerated, this, &MainWindow::onPreviewGenerated);

            QMetaObject::invokeMethod(ui->sdmLoadProgressBar, [this]() {
                ui->sdmLoadProgressBar->hide();
            });
        });
    });

    connect(ui->vaeButton, &QPushButton::clicked, [this]() {
        QString fileName = QFileDialog::getOpenFileName(
            this,                    // Parent widget
            tr("Open SafeTensors file"),         // Dialog title
            QDir::homePath(),        // Starting directory
            tr("SafeTensors files (*.safetensors);")
            );
        qDebug() << "and this is the file" << fileName;

        vaePath = fileName;
    });

    connect(ui->generateButton, &QPushButton::clicked, [this]() {
        qDebug() << "I need to generat ya image!";

        QString positive = ui->positiveInput->toPlainText();
        QString negative = ui->negativeInput->toPlainText();

        SDImageOptions options;

        options.cfgScale = ui->cfgSlider->value();
        options.width = ui->widthBox->value();
        options.height = ui->heightBox->value();
        options.stepCount = ui->stepCountSlider->value();

        sdm->generateAsync(positive, negative, options, [this](QImage image) {
            showImage(image);
        });

        qDebug() << "Loaded da SD modelk";
    });


    // QGraphicsScene *scene = new QGraphicsScene();
    // ui->imageView->setScene(scene);

}

void MainWindow::send() {
    QString message = ui->messageInput->toPlainText();

    ui->messageInput->setPlainText("");
    ui->listWidget->addItem(message);

    ui->listWidget->addItem("");
    auto lastItemIndex = ui->listWidget->count() - 1;
    auto lastItem = ui->listWidget->item(lastItemIndex);
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
    QMetaObject::invokeMethod(ui->previewImage, [this, image]() {
        // QGraphicsPixmapItem *item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
        // ui->imageView->scene()->addItem(item);
        // ui->imageView->fitInView(item, Qt::KeepAspectRatio);
        QPixmap pix = QPixmap::fromImage(image).scaled(ui->previewImage->size(), Qt::KeepAspectRatio);
        ui->previewImage->setPixmap(pix);
    });
}


void MainWindow::onPreviewGenerated(int step, const QImage& preview, bool isNoisy) {
    showImage(preview);
}

MainWindow::~MainWindow()
{
    delete ui;
}
