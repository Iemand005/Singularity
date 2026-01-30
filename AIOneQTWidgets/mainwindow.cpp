#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("AIOne");
    ui->listWidget->addItem("hey");
    ui->listWidget->addItem("hey");
}

MainWindow::~MainWindow()
{
    delete ui;
}
