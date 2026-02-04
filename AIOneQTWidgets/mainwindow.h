#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>

#include <ModelFactory.hpp>

#include "../AIOneQTCore/QModelFactory.hpp"

#include "progressbar.h"

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

    QString vaePath = "";
    QString taePath = "";

    SDImageOptions sdImageOptions;
    SDModelOptions sdModelOptions;

    QString quantModelPath = "";

    ProgressCallback progressFor(ProgressBar *bar);

    void showImage(QImage image, bool smooth = false);

    // void reloadSeed() {
    //     if (ui->randomizeSeedBox->isChecked()) ui->seedInput->setValue(sdm->newSeed());
    // }
    void updateSeed();

    QString lastPath = QDir::homePath(); // TOOD:: load dis from a file or somthng yaya
    QString openFileDialog(const QString &title, QString fileTypes = tr("SafeTensors files (*.safetensors);"));
};
#endif // MAINWINDOW_H
