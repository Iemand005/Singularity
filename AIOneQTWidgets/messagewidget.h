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

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void sizeChanged();
    void prevRequested();
    void nextRequested();
    void regenerateRequested();

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

    Ui::MessageWidget *ui;
    QToolButton *m_thinkToggle = nullptr;
    QScrollArea *m_thinkScroll = nullptr;
    QLabel *m_thinkContent = nullptr;
    QPropertyAnimation *m_thinkAnimMax = nullptr;
    QPropertyAnimation *m_thinkAnimMin = nullptr;
    bool m_isThinking = false;
    bool m_thinkExpanded = false;
    bool m_everHadContent = false;
    bool m_hasStreamedContent = false;
    uint64_t m_parentId = 0;
    QString m_tagBuffer;
};

#endif // MESSAGEWIDGET_H