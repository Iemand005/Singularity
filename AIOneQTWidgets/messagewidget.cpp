#include "messagewidget.h"
#include <QRegularExpression>

MessageWidget::MessageWidget(const QString &text, QWidget *parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout(this))
{
    m_layout->setContentsMargins(0, 2, 0, 2);
    m_layout->setSpacing(2);
    parseAndBuild(text);
}

void MessageWidget::parseAndBuild(const QString &text)
{
    QRegularExpression thinkRegex(R"(<think>([\s\S]*?)</think>)");

    int lastEnd = 0;
    auto it = thinkRegex.globalMatch(text);
    while (it.hasNext()) {
        auto match = it.next();
        if (match.capturedStart() > lastEnd)
            m_layout->addWidget(createTextSection(text.mid(lastEnd, match.capturedStart() - lastEnd)));
        m_layout->addWidget(createThinkSection(match.captured(1)));
        lastEnd = match.capturedEnd();
    }
    if (lastEnd < text.length())
        m_layout->addWidget(createTextSection(text.mid(lastEnd)));
}

QWidget* MessageWidget::createTextSection(const QString &text)
{
    auto *label = new QLabel(text, this);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setContentsMargins(0, 0, 0, 0);
    return label;
}

QWidget* MessageWidget::createThinkSection(const QString &content)
{
    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *toggle = new QToolButton(container);
    toggle->setText(QStringLiteral("\u25B6 Show thinking"));
    toggle->setCheckable(true);
    toggle->setChecked(false);
    toggle->setStyleSheet(
        "QToolButton { border: none; color: #4a9eff; font-weight: bold; }"
        "QToolButton:hover { color: #6ab4ff; }"
    );
    toggle->setCursor(Qt::PointingHandCursor);
    toggle->setToolButtonStyle(Qt::ToolButtonTextOnly);

    auto *contentLabel = new QLabel(content, container);
    contentLabel->setWordWrap(true);
    contentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    contentLabel->setContentsMargins(20, 2, 0, 2);
    contentLabel->hide();

    connect(toggle, &QToolButton::toggled, this, [this, toggle, contentLabel](bool checked) {
        contentLabel->setVisible(checked);
        toggle->setText(checked ? QStringLiteral("\u25BC Hide thinking") : QStringLiteral("\u25B6 Show thinking"));
        emit toggleChanged();
    });

    layout->addWidget(toggle);
    layout->addWidget(contentLabel);
    return container;
}
