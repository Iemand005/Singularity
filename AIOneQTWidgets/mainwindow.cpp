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
    });
}

MainWindow::~MainWindow()
{
    delete ui;
}
