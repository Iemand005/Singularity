#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <ModelFactory.hpp>

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

    std::unique_ptr<ModelFactory> factory = std::make_unique<ModelFactory>();
    std::unique_ptr<LLModel> llm = nullptr;
    std::shared_ptr<MessageContext> messages = nullptr;


private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
