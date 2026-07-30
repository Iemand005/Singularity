#ifndef MESSAGEWIDGET_H
#define MESSAGEWIDGET_H

#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPropertyAnimation>

class MessageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MessageWidget(QWidget *parent = nullptr);

    void appendToken(const QString &token);
    void setThinking(bool thinking);
    void finish();

    void setContent(const QString &text);
    void setVersionInfo(size_t current, size_t total);
    void setParentId(uint64_t id) { m_parentId = id; }
    uint64_t parentId() const { return m_parentId; }

signals:
    void sizeChanged();
    void prevRequested();
    void nextRequested();
    void regenerateRequested();

private:
    void startTextSegment();
    void ensureTextSegment();
    void startThinkSegment();
    void ensureThinkSegment();
    void processBuffer();

    QVBoxLayout *m_layout;
    bool m_isThinking = false;
    bool m_everHadContent = false;
    bool m_hasStreamedContent = false;
    QString m_pending;

    QLabel *m_textLabel = nullptr;
    QWidget *m_thinkContainer = nullptr;
    QToolButton *m_thinkToggle = nullptr;
    QScrollArea *m_thinkScroll = nullptr;
    QLabel *m_thinkContent = nullptr;
    QPropertyAnimation *m_thinkAnim = nullptr;
    int m_thinkCollapsedHeight = 0;

    uint64_t m_parentId = 0;
    QWidget *m_versionBar = nullptr;
    QPushButton *m_prevBtn = nullptr;
    QLabel *m_versionLabel = nullptr;
    QPushButton *m_nextBtn = nullptr;
    QPushButton *m_regenerateBtn = nullptr;
};

#endif // MESSAGEWIDGET_H
