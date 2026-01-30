#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <ModelFactory.hpp>

#include "../AIOneQTCore/QModelFactory.hpp"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    std::unique_ptr<QModelFactory> factory = std::make_unique<QModelFactory>();
    QLLModelPtr llm = nullptr;
    QSDModelPtr sdm = nullptr;

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
