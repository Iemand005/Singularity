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

        llm->completeAny(message.toStdString(), [this](std::string token) {
            QString response(token.c_str());
            qDebug() << token;
            ui->listWidget->addItem(token.c_str());
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
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}
