#ifndef MESSAGEWIDGET_H
#define MESSAGEWIDGET_H

#include <QWidget>

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
    Ui::MessageWidget *ui;
    bool m_isThinking = false;
    bool m_everHadContent = false;
    bool m_hasStreamedContent = false;
    uint64_t m_parentId = 0;
};

#endif // MESSAGEWIDGET_H