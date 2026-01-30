#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->listWidget->addItem("hey");
    ui->listWidget->addItem("hey");

    connect(ui->sendButton, &QPushButton::clicked, [this]() {
        qDebug() << "I felt that!";

        QString message = ui->messageInput->toPlainText();

        ui->listWidget->addItem("Response: ");

        llm->generateAsync(message.toStdString(), [this](std::string token) {
            QString response(token.c_str());
            qDebug() << response;
            // ui->listWidget->addItem(response);
            QString newText = ui->listWidget->currentItem()->text() + response;
            auto lastItemIndex = ui->listWidget->count() - 1;
            // auto lastItem =
                ui->listWidget->item(lastItemIndex)->setText(newText);
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

        std::string path = fileName.toStdString();
        this->llm = factory->loadLLM(path);
        this->messages = llm->createContext();
        qDebug() << "Loaded da modeellaaa";
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}
