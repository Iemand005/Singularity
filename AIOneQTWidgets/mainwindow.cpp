#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QGraphicsPixmapItem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->sendButton, &QPushButton::clicked, [this]() {
        qDebug() << "I felt that!";

        QString message = ui->messageInput->toPlainText();
        ui->messageInput->setPlainText("");

        ui->listWidget->addItem(message);

        llm->generateAsync(message.toStdString(), [this](std::string token) {
            QString response(token.c_str());
            qDebug() << response;
            // ui->listWidget->addItem(response);
            // QString newText = ui->listWidget->currentItem()->text() + response;
            auto lastItemIndex = ui->listWidget->count() - 1;
            auto lastItem = ui->listWidget->item(lastItemIndex);
            QString newText = lastItem->text() + response;
            lastItem->setText(newText);
        });
    });

    connect(ui->loadLLMButton, &QPushButton::clicked, [this]() {
        qDebug() << "I need that!";

        QString fileName = QFileDialog::getOpenFileName(
            this,                    // Parent widget
            tr("Open GGUF file"),         // Dialog title
            QDir::homePath(),        // Starting directory
            tr("GGUF files (*.gguf);")
            );
        qDebug() << "and this is the file" << fileName;

        this->llm = factory->loadLLM(fileName);
        qDebug() << "Loaded da modeellaaa";
    });


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

void MainWindow::showImage(QImage image) {
    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(QPixmap::fromImage(image));
    ui->imageView->scene()->addItem(item);
    ui->imageView->fitInView(item, Qt::KeepAspectRatio);
}

void MainWindow::onPreviewGenerated(int step, const QImage& preview, bool isNoisy) {
    showImage(preview);
}

MainWindow::~MainWindow()
{
    delete ui;
}
