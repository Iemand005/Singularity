#ifndef MESSAGEWIDGET_H
#define MESSAGEWIDGET_H

#include <QWidget>
#include <QToolButton>
#include <QScrollArea>
#include <QLabel>
#include <QPropertyAnimation>
#include <QEvent>

namespace Ui {
class MessageWidget;
}

class MessageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MessageWidget(QWidget *parent = nullptr);
    ~MessageWidget();

    void appendToken(const QString &token);
    void appendTokenReasoning(const QString &token, bool thinking);
    void setThinking(bool thinking);
    void finish();

    void setContent(const QString &text);
    void setVersionInfo(size_t current, size_t total);
    void setParentId(uint64_t id) { m_parentId = id; }
    uint64_t parentId() const { return m_parentId; }
    void setMessageId(uint64_t id) { m_messageId = id; }
    uint64_t messageId() const { return m_messageId; }
    void setTimestamp(qint64 millis);
    void setAssistantMessage(bool isAssistant);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void sizeChanged();
    void prevRequested();
    void nextRequested();
    void regenerateRequested();
    void editRequested();
    void continueRequested();
    void forkRequested(uint64_t messageId);

private:
    void hideThinking();
    void showThinking();
    void hideVersionBar();
    void showVersionBar();
    void appendToThinking(const QString &token);
    void processToken(const QString &token);
    void routeText(const QString &text);
    void animateThinking(bool expand);
    void stopThinkAnimation();
    int thinkTargetHeight() const;
    QSize contentSize() const;
    int contentHeightForWidth(int width) const;

    Ui::MessageWidget *ui;
    QToolButton *m_thinkToggle = nullptr;
    QScrollArea *m_thinkScroll = nullptr;
    QLabel *m_thinkContent = nullptr;
    QPropertyAnimation *m_thinkAnimMax = nullptr;
    QObject *m_eventWatchParent = nullptr;
    bool m_isThinking = false;
    bool m_thinkExpanded = false;
    bool m_everHadContent = false;
    bool m_hasStreamedContent = false;
    uint64_t m_parentId = 0;
    uint64_t m_messageId = 0;
    qint64 m_timestamp = 0;
    QString m_tagBuffer;
};

#endif // MESSAGEWIDGET_H