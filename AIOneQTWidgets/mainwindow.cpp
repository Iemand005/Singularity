#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QGraphicsPixmapItem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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
        // AsyncGenerationCallbacks callbacks;
        // callbacks.onToken
        TextGenOptions options;
        chatManager->completeAsync(message.toStdString(), options, [this](const std::string &token) {
            QMetaObject::invokeMethod(ui->llmInputFrame, [this, token]() {
                ui->messageInput->insertPlainText(QString(token.c_str()));
            });
        });
    });

    ui->messageInput->installEventFilter(this);

    connect(ui->loadSDButton, &QPushButton::clicked, [this]() {
        qDebug() << "I need that als too!";

        QString fileName = QFileDialog::getOpenFileName(
            this,                    // Parent widget
            tr("Open SafeTensors file"),         // Dialog title
            QDir::homePath(),        // Starting directory
            tr("SafeTensors files (*.safetensors);")
            );
        qDebug() << "and this is the file" << fileName;

        if (this->sdm) this->sdm = nullptr;

        this->sdm = factory->loadSDM(fileName);

        connect(this->sdm.get(), &QSDModel::previewGenerated, this, &MainWindow::onPreviewGenerated);

        qDebug() << "Loaded da SD modelk";
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

    QGraphicsScene *scene = new QGraphicsScene();
    ui->imageView->setScene(scene);

}

void MainWindow::send() {
    QString message = ui->messageInput->toPlainText();

    ui->messageInput->setPlainText("");
    ui->listWidget->addItem(message);

    ui->listWidget->addItem("");
    auto lastItemIndex = ui->listWidget->count() - 1;
    auto lastItem = ui->listWidget->item(lastItemIndex);
    ui->tokensGeneratedDisplay->display(0);

    chatManager->sendAsync(message, [this](const TextGenerationResult &output) {
        ui->tokensCachedDisplay->display((int)output.tokensCached);
        ui->tokensGeneratedDisplay->display((int)output.tokensGenerated);
        ui->tokensEvaluatedDisplay->display((int)output.tokensEvaluated);
    }, [this, lastItem](const QString &token) {

QMetaObject::invokeMethod(ui->listWidget, [this, lastItem, token]() {
        
       lastItem->setText(lastItem->text() + token);

       ui->listWidget->scrollToBottom();

       auto display = ui->tokensGeneratedDisplay;
       display->display(display->intValue() + 1);
                                                                                                                            });
   }, [this](const float &progress) {
       QMetaObject::invokeMethod(ui->inputEvalProgressBar, [this, progress]() {
           ui->inputEvalProgressBar->setValue(progress * 100);
       });
   });
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
    QMetaObject::invokeMethod(ui->imageView, [this, image]() {
        QGraphicsPixmapItem *item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
        ui->imageView->scene()->addItem(item);
        ui->imageView->fitInView(item, Qt::KeepAspectRatio);
    });
}


void MainWindow::onPreviewGenerated(int step, const QImage& preview, bool isNoisy) {
    showImage(preview);
}

MainWindow::~MainWindow()
{
    delete ui;
}
