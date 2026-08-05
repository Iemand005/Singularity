#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QListWidget>
#include <QPushButton>
#include <QSplitter>
#include <map>

#include <ModelFactory.hpp>

#include "../AIOneQTCore/QModelFactory.hpp"
#include "../AIOne/src/Providers/OpenAIProvider.hpp"
#include "../AIOne/src/ChatStorage.hpp"

#include "progressbar.h"

class MessageWidget;

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
    std::unique_ptr<AIOne::OpenAIProvider> openAIProvider;

    void send();

private slots:
    void onPreviewGenerated(int step, const QImage& preview, bool isNoisy);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Ui::MainWindow *ui;

    // Sidebar
    QSplitter *m_chatSplitter = nullptr;
    QWidget *m_sidebar = nullptr;
    QListWidget *m_chatList = nullptr;
    QPushButton *m_newChatBtn = nullptr;

    // Storage
    QString m_chatsRoot;
    AppSettings m_settings;
    bool m_loadingChat = false;

    // Current model tracking
    QString m_currentModelName;

    // Generation state
    bool m_generating = false;
    bool m_stopRequested = false;
    QListWidgetItem *m_generatingItem = nullptr;
    MessageWidget *m_generatingWidget = nullptr;

    QString vaePath = "";
    QString taePath = "";

    SDImageOptions sdImageOptions;
    SDModelOptions sdModelOptions;

    QString quantModelPath = "";

    ProgressCallback progressFor(ProgressBar *bar);
    void showImage(QImage image, bool smooth = false);
    void updateSeed();

    QString lastPath = QDir::homePath();
    QString openFileDialog(const QString &title, QString fileTypes = tr("SafeTensors files (*.safetensors);"));

    void setupCloudChatManager(const QString &modelId, const QString &apiKey);
    void connectOpenAI(const QString &apiKey);
    QString currentApiBaseUrl() const;
    QString apiKeyForUrl(const QString &url) const;

    // Chat storage
    void initChatStorage();
    void refreshChatList();
    void onChatSelected(int row);
    void onNewChat();
    void syncChatToUI();
    void saveChatMetaDelayed();
    void loadSettings();
    void saveSettings();
    void loadLastChat();
    void onSendDone();

    // Generation helpers
    void forceStopGeneration();

    // Version/branching
    void rebuildConversationDisplay();
    void onVersionPrev(uint64_t parentId);
    void onVersionNext(uint64_t parentId);
    void onRegenerateRequested(uint64_t parentId);
    void onEditRequested(uint64_t parentId);
    void onContinueRequested(uint64_t parentId);
    void onGenerateLastResponse(uint64_t parentId);
    void onForkRequested(uint64_t messageId);
    void cancelEditMode();

    // Edit mode: the next send creates a new version of the message being edited
    bool m_editing = false;
    uint64_t m_editingParentId = 0;

    std::map<uint64_t, QListWidgetItem*> m_slotItems;
};

#endif // MAINWINDOW_H
