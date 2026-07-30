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
    explicit MessageWidget(const QString &text, QWidget *parent = nullptr);

signals:
    void toggleChanged();

private:
    void parseAndBuild(const QString &text);
    QWidget* createTextSection(const QString &text);
    QWidget* createThinkSection(const QString &content);

    QVBoxLayout *m_layout;
};

#endif // MESSAGEWIDGET_H
