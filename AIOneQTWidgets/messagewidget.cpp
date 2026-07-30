#include "messagewidget.h"

MessageWidget::MessageWidget(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout(this))
{
    m_layout->setContentsMargins(0, 2, 0, 2);
    m_layout->setSpacing(2);
}

void MessageWidget::appendToken(const QString &token)
{
    if (m_isThinking) {
        if (!m_thinkContent)
            startThinkSegment();
        m_thinkContent->setText(m_thinkContent->text() + token);
    } else {
        if (!m_textLabel)
            startTextSegment();
        m_textLabel->setText(m_textLabel->text() + token);
    }
}

void MessageWidget::setThinking(bool thinking)
{
    if (m_isThinking == thinking)
        return;

    m_textLabel = nullptr;
    m_thinkContainer = nullptr;
    m_thinkToggle = nullptr;
    m_thinkContent = nullptr;

    m_isThinking = thinking;

    if (thinking)
        startThinkSegment();
    else
        startTextSegment();

    emit sizeChanged();
}

void MessageWidget::finish()
{
    m_textLabel = nullptr;
    m_thinkContainer = nullptr;
    m_thinkToggle = nullptr;
    m_thinkContent = nullptr;
    emit sizeChanged();
}

void MessageWidget::startTextSegment()
{
    auto *label = new QLabel(this);
    label->setWordWrap(true);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setContentsMargins(0, 0, 0, 0);
    m_layout->addWidget(label);
    m_textLabel = label;
}

void MessageWidget::startThinkSegment()
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

    auto *content = new QLabel(container);
    content->setWordWrap(true);
    content->setTextInteractionFlags(Qt::TextSelectableByMouse);
    content->setContentsMargins(20, 2, 0, 2);
    content->hide();

    connect(toggle, &QToolButton::toggled, this, [this, toggle, content](bool checked) {
        content->setVisible(checked);
        toggle->setText(checked ? QStringLiteral("\u25BC Hide thinking") : QStringLiteral("\u25B6 Show thinking"));
        emit sizeChanged();
    });

    layout->addWidget(toggle);
    layout->addWidget(content);
    m_layout->addWidget(container);

    m_thinkContainer = container;
    m_thinkToggle = toggle;
    m_thinkContent = content;
}
