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

    QModelFactoryPtr factory = std::make_unique<QModelFactory>();
    QLLModelPtr llm = nullptr;
    QSDModelPtr sdm = nullptr;
    QChatManagerPtr chatManager = nullptr;

    void send();

private slots:
    void onPreviewGenerated(int step, const QImage& preview, bool isNoisy);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::MainWindow *ui;

    void showImage(QImage image);
};
#endif // MAINWINDOW_H
