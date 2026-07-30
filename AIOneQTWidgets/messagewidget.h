#ifndef MESSAGEWIDGET_H
#define MESSAGEWIDGET_H

#include <QWidget>
#include <QLabel>
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

signals:
    void sizeChanged();

private:
    void startTextSegment();
    void ensureTextSegment();
    void startThinkSegment();
    void ensureThinkSegment();
    void processBuffer();

    QVBoxLayout *m_layout;
    bool m_isThinking = false;
    bool m_everHadContent = false;
    QString m_pending;

    QLabel *m_textLabel = nullptr;
    QWidget *m_thinkContainer = nullptr;
    QToolButton *m_thinkToggle = nullptr;
    QScrollArea *m_thinkScroll = nullptr;
    QLabel *m_thinkContent = nullptr;
    QPropertyAnimation *m_thinkAnim = nullptr;
    int m_thinkCollapsedHeight = 0;
};

#endif // MESSAGEWIDGET_H
