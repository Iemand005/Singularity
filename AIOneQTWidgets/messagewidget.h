#ifndef MESSAGEWIDGET_H
#define MESSAGEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QToolButton>
#include <QVBoxLayout>

class MessageWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MessageWidget(QWidget *parent = nullptr);

    void appendToken(const QString &token);
    void setThinking(bool thinking);
    void finish();

signals:
    void sizeChanged();

private:
    void startTextSegment();
    void startThinkSegment();

    QVBoxLayout *m_layout;
    bool m_isThinking = false;

    QLabel *m_textLabel = nullptr;
    QWidget *m_thinkContainer = nullptr;
    QToolButton *m_thinkToggle = nullptr;
    QLabel *m_thinkContent = nullptr;
};

#endif // MESSAGEWIDGET_H
